/**
 * @file server.c
 * @author Furkan Sergen Dayioglu
 * @brief CSE344 System Programming
 * 
 * 121044015
 * Final Project
 *    
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/file.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct threadP_t
{
    int id;
    int status; /* if it works or sits idle */
    int socketFD;
    int busy;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} threadP_t;

char ***table;
int column_number;
int table_lenght;

/* TABLE INITIALIZATION & CSV PARSER FUNCTIONS*/
int number_of_columns(char *filename);
int line_number(char *filename);
void read_csv_file(char *filename);
void line_to_table(char *line, int line_number);
void threadP_Array();

/*THREAD FUNCTIONS*/

pthread_t *pool_threads;
threadP_t *threadParams; /* thread_params */

/* SQL Functions */
/* select  ---- > reader */
void select_Q(int tid, int distinct, char *column_name);
/* Update ---- > writer */
void update_Q(int tid, char *column_name, char *where_column);

void sql_parser(int tid, char *command);
void select_parser(int tid, char *command);
void update_parser(int tid, char *command);

void *thread_func(void *);

/*READER WRITER SYNCRONIZERS*/
/* Inspired from week 10 slides*/
/* state variables */
int AR = 0; /* Active Reader*/
int WR = 0; /* Waiting Reader*/
int AW = 0; /* Active Writer */
int WW = 0; /* Waiting Writer*/
/* condition variables */
pthread_cond_t okToRead = PTHREAD_COND_INITIALIZER;  /* reader condition variable*/
pthread_cond_t okToWrite = PTHREAD_COND_INITIALIZER; /* writer condition variable*/

/* mutex */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; /* initialize mutex */

pthread_mutex_t mainMutex = PTHREAD_MUTEX_INITIALIZER;   /* initialize main mutex controls threads job assignment loop */
pthread_mutex_t m_pool_full = PTHREAD_MUTEX_INITIALIZER; /* check if all threads are busy */
pthread_cond_t c_pool_full = PTHREAD_COND_INITIALIZER;   /* signal if any thread gets available */

/* COMMANDLINE ARGUMENTS */
int port;
char *pathToLogFile;
int poolSize;
char *datasetPath;

int output_fd;

/* Some Essential*/
void print_usage();
void print_log(char *msg,int fd);

void check_instance();
int create_daemon();

void sigIntHandle(int sig);
static volatile sig_atomic_t sig_int = 0;

char *instance;
key_t key;
int shmid;

int main(int argc, char **argv)
{
   

    pid_t pid = 0, snw = 0;

    /*LOG PATH*/
  

    /*Shared memory usage to prevent multiple instantion*/
    key = ftok("serverNoSecInstance", 123);
    shmid = shmget(key, 1024, 0666 | IPC_CREAT);
    instance = (char *)shmat(shmid, (void *)0, 0);

    if (strcmp(instance, "CW") == 0)
    {
        fprintf(stderr,"already working\n");
        exit(-1);
    }

    /* Determine that Currently Working */
    instance[0] = 'C';
    instance[1] = 'W';

    /*Daemon Creatation */
    pid = fork(); // Create child process
    if (pid < 0)
    { // Check Fail
        fprintf(stderr,"Fork failed!\n");
        exit(EXIT_FAILURE);
    }
    if (pid > 0)
    {
        exit(EXIT_SUCCESS); // parent process exit
    }
    umask(0);       // unmask file mode
    snw = setsid(); // set new session
    if (snw < 0)
    {

        fprintf(stderr,"setsid() error!\n");
        exit(EXIT_FAILURE);
    }

    /* We are daemon now Hallelujah \m/ */
    if (getpid() == syscall(SYS_gettid))
    {

        /* variables declaration area */
        int opt;
        char param1[100];
        char param2[100];
        char param3[100];
        char param4[100];
        char loadMsg[100];
        char tcMsg[100];
        int close_fd = 0;
        int i, j;
        char socket_ip[] = "127.0.0.1"; /* Run on Local */
        double time_interval;
        struct timeval tm1, tm2;
        struct sockaddr_in serv_adr;
        char socBuf[1025] = "";
        int socketFd;

        if (argc != 9)
        {
            print_usage();
            exit(-1);
        }

        while ((opt = getopt(argc, argv, ":p:o:l:d:")))
        {

            switch (opt)
            {
            case 'p':
                port = atoi(optarg);
                break;
            case 'o':
                pathToLogFile = optarg;
                break;
            case 'l':
                poolSize = atoi(optarg);

                break;
            case 'd':
                datasetPath = optarg;
                break;
            default:
                fprintf(stderr,"invalid Parameter\n");
                print_usage();
                exit(-1);
                break;
            }
        }

        output_fd = open("logfile.log", O_WRONLY | O_CREAT | O_EXCL | O_APPEND, S_IRUSR | S_IWUSR);
       
        if (port <= 1000)
        {
            print_log("First 1000 ports are being used by kernel.\n",output_fd);
            print_log("That means, you entered invalid port.\n",output_fd);
            print_log("Terminating...\n",output_fd);
            exit(-1);
        }
        close(STDIN_FILENO); // close inherited files
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        close(close_fd);

        /* Main thread execution params to log file */
        print_log("Execution with parametes:\n",output_fd);
        sprintf(param1, "-p %d\n", port);
        print_log(param1,output_fd);
        sprintf(param2, "-o %s\n", pathToLogFile);
        print_log(param2,output_fd);
        sprintf(param3, "-l %d\n", poolSize);
        print_log(param3,output_fd);
        sprintf(param4, "-d %s\n", datasetPath);
        print_log(param4,output_fd);

        signal(SIGINT, sigIntHandle);

        print_log("Loading dataset...\n",output_fd);

        gettimeofday(&tm1, NULL);
        column_number = number_of_columns(datasetPath);
        table_lenght = line_number(datasetPath);

        /*initialize table*/
        table = (char ***)calloc(column_number, sizeof(char **));

        for (i = 0; i < column_number; i++)
        {
            table[i] = (char **)calloc(table_lenght, sizeof(char *));
            for (j = 0; j < table_lenght; j++)
                table[i][j] = (char *)calloc(150, sizeof(char));
        }

        read_csv_file(datasetPath);

        pool_threads = (pthread_t *)calloc(poolSize, sizeof(pthread_t));
        threadParams = (threadP_t *)calloc(poolSize, sizeof(threadP_t));
        threadP_Array();

        gettimeofday(&tm2, NULL);

        time_interval = (double)(tm2.tv_sec - tm1.tv_sec) + (double)(tm2.tv_usec - tm1.tv_usec) / 1000000;
        sprintf(loadMsg, "Dataset loaded in %.2f seconds with %d entries", time_interval, table_lenght);
        print_log(loadMsg,output_fd);

        sprintf(tcMsg, "A pool of %d threads has been created.\n", poolSize);
        print_log(tcMsg,output_fd);

        for (i = 0; i < poolSize; i++)
        {
            if (pthread_create(&pool_threads[i], NULL, thread_func, (void *)i) != 0)
            {
                print_log("Thread Creation failed\n",output_fd);
                exit(-1);
            }
        }

        /* Socket \m/ */

        socketFd = 0;
        memset(&serv_adr, '0', sizeof(serv_adr));
        memset(socBuf, '0', sizeof(socBuf));

        if ((socketFd = socket(AF_INET, SOCK_STREAM, 0) < 0))
        {
            print_log("Socket Initialization failed \n",output_fd);
            print_log("Terminating....\n",output_fd);
            exit(-1);
        }

        serv_adr.sin_family = AF_INET;
        serv_adr.sin_addr.s_addr = inet_addr(socket_ip);
        serv_adr.sin_port = htons(port);

        if ((bind(socketFd, (struct sockaddr *)&serv_adr, sizeof(serv_adr))) < 0)
        {
            print_log("Socket Binding error \n",output_fd);
            print_log("Terminating...",output_fd);
            exit(-1);
        }

        if ((listen(socketFd, 20)) < 0)
        {
            print_log("Listen Error\n",output_fd);
            print_log("Terminating...\n",output_fd);
            exit(-1);
        }

        while (1)
        {

            socklen_t soclen = sizeof(serv_adr);
            int accept_fd = accept(socketFd, (struct sockaddr *)&serv_adr, &soclen);

            pthread_mutex_lock(&mainMutex);
            int threadFound = 0;

            for (i = 0; i < poolSize; i++)
            {
                if (threadParams[i].status == 0)
                {
                    char threadMsg[500];
                    threadParams[i].status = 1;
                    threadParams[i].busy = 1;
                    threadParams[i].socketFD = accept_fd;
                    pthread_cond_signal(&threadParams[i].cond);

                    sprintf(threadMsg, "A connection has been delegated to thread_id #%d", i);
                    print_log(threadMsg,output_fd);

                    threadFound = 1;
                }
            }

            pthread_mutex_unlock(&mainMutex);
            if (threadFound == 0)
            {
                char noTMSG[] = "No thread is available! waiting...\n";
                print_log(noTMSG,output_fd);
            }

            if (sig_int == 1)
                break;
        }

        for (i = 0; i < poolSize; i++)
        {
            pthread_join(pool_threads[i], NULL);
        }
        /* free thread pool */
        free(pool_threads);
        free(threadParams);
        /* Free DB Table */
        for (i = 0; i < column_number; i++)
        {
            for (j = 0; j < table_lenght; j++)
            {
                free(table[i][j]);
            }
        }
        for (i = 0; i < column_number; i++)
        {
            free(table[i]);
        }
        free(table);

        /*Close Log File*/
        close(output_fd);
    }

    /* Destroy Shared Memory */
    shmdt(instance);
    return 0;
}

/* initialization functions*/
void print_log(char *msg,int fd)
{
    char buffer[500];

    struct tm *currentTime;
    time_t timeCurrent = time(0);
    currentTime = gmtime(&timeCurrent);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S ", currentTime);
    strcat(buffer, msg);
    write(fd, buffer, strlen(buffer));
}

void print_usage()
{
    fprintf(stderr,"#USAGE#\n");
    fprintf(stderr,"Invalid amount of parameters\n");
    fprintf(stderr,"server -p PORT -o PathToLogFile -l poolSize -d datasetPath\n");
    fprintf(stderr,"Terminating...\n");
}

int number_of_columns(char *filename)
{

    FILE *input = fopen(filename, "r");
    char line[500];
    char *strtok_ptr = NULL;
    char *temp;
    int i = 0;

    if (input == NULL)
    {
        print_log("Dataset File could not be opened\n",output_fd);
        print_log("Terminating...\n",output_fd);
        exit(-1);
    }

    fgets(line, sizeof(line), input);
    line[strlen(line) - 1] = '\0';

    strtok_ptr = line;

    while ((temp = strtok_r(strtok_ptr, ",", &strtok_ptr)))
    {
        i++;
    }

    fclose(input);
    return i;
}

int line_number(char *filename)
{

    FILE *input = fopen(filename, "r");
    int i = 0;
    char line[500];

    if (input == NULL)
    {
        print_log("Dataset File could not be opened\n",output_fd);
        print_log("Terminating...\n",output_fd);
        exit(-1);
    }

    while (fgets(line, sizeof(line), input) != NULL)
    {
        i++;
    }

    fclose(input);

    return i;
}

void read_csv_file(char *filename)
{
    int i = 0;
    FILE *input = fopen(filename, "r");
    char line[500];

    if (input == NULL)
    {
        print_log("DatasetFile could not be opened\n",output_fd);
        print_log("Terminating....\n",output_fd);
        exit(-1);
    }

    while (fgets(line, sizeof(line), input) != NULL)
    {

        line[strlen(line) - 1] = '\0';
        line_to_table(line, i);
        i++;
        memset(line, 0, strlen(line));
    }

    fclose(input);
}

void line_to_table(char *line, int line_number)
{
    int i = 0;
    char *temp;

    for (i = 0; i < column_number; i++)
    {

        temp = strtok_r(line, ",", &line);
        if (temp[0] == '"')
        {

            temp++;
            temp[strlen(temp) - 1] = '\0';
            fprintf(stderr, "Char p_pointer : %s\n", temp);
        }
        strcpy(table[i][line_number], temp);
        fprintf(stderr, "TABLE[%d][%d]: %s\n", i, line_number, table[i][line_number]);
    }
}

void threadP_Array()
{
    int i = 0;

    for (i = 0; i < poolSize; i++)
    {
        threadParams[i].id = i;
        threadParams[i].status = 0;
        threadParams[i].socketFD = 0;
        threadParams[i].busy = 0;
        if (pthread_mutex_init(&threadParams[i].mutex, NULL) != 0)
        {
            print_log("Thread Params Mutex Initialization failed\n",output_fd);
            print_log("Terminating...\n",output_fd);
            exit(-1);
        }

        if (pthread_cond_init(&threadParams[i].cond, NULL) != 0)
        {
            print_log("Thread Params Condition Variable Initialization failed\n",output_fd);
            print_log("Terminating...\n",output_fd);
            exit(-1);
        }
    }
}

void sigIntHandle(int sigint)
{
    int i = 0, j = 0;
    switch (sigint)
    {
    case SIGINT:
        sig_int = 1;

        print_log("Termination Signal Recieved. waiting for ongoing threads to complete.\n",output_fd);
        for (i = 0; i < poolSize; i++)
        {
            int join;
            if (threadParams[i].busy == 0)
            {
                threadParams[i].busy = 1;
                pthread_cond_wait(&threadParams[i].cond,&threadParams[i].mutex);
            }

            join = pthread_join(pool_threads[i], NULL);
            if (join != 0)
            {
                print_log("Join Error\n",output_fd);
            }
        }
        print_log("ALL threads joined\n",output_fd);
        shmdt(instance);
        free(threadParams);
        free(pool_threads);

        for (i = 0; i < column_number; i++)
        {
            for (j = 0; j < table_lenght; j++)
            {
                free(table[i][j]);
            }
        }
        for (i = 0; i < column_number; i++)
        {
            free(table[i]);
        }
        free(table);

        kill(getpid(),SIGKILL);
        break;

    default:
        print_log("UNDEFINED SIGNAL\n JUST IGNORING\n",output_fd);
        break;
    }
}

void *thread_func(void *args)
{
    int id = (int)args;
    //int i = 0;
   // int row_effected = 0;
   

    char wfcMsg[500];
    sprintf(wfcMsg, "Thead %d is waiting for connection\n", id);

    while (1)
    {
        print_log(wfcMsg,output_fd);

        
            
        if (sig_int == 1)
            break;
       

        pthread_mutex_lock(&threadParams[id].mutex);

            if (threadParams[id].busy == 0)
        {
            while (threadParams[id].busy == 0)
            {
                pthread_cond_wait(&threadParams[id].cond, &threadParams[id].mutex);
                threadParams[id].status = 1;
            }
        }

        if (sig_int == 0)
        {   
            usleep(500000);
            int q_len = 0;
            char query[500];
            char query_rec_msg[1024];
            if ((q_len = recv(threadParams[id].socketFD, query, 1024, 0)) < 0)
            {
                print_log("Recieve Error\n",output_fd);
                print_log("Terminating...\n",output_fd);
                exit(-1);
            }

            query[q_len] = '\0';
            sprintf(query_rec_msg, "Thread#%d : recieved query \"%s\"", id, query);
            print_log(query_rec_msg,output_fd);

            sql_parser(id, query);

            pthread_mutex_lock(&mainMutex);
            threadParams[id].busy = 0;
            threadParams[id].status = 0;
            pthread_mutex_unlock(&mainMutex);
            pthread_mutex_unlock(&threadParams[id].mutex);
        }
    }

    return 0;
}

void sql_parser(int tid, char *command)
{
    char *temp = "";
    //int row_eff;

    temp = strtok_r(command, " ", &command);

    /* READER PART*/
    if (strcmp("SELECT", temp) == 0)
    {
        select_parser(tid, command);

    } /* WRITER PART */
    else if (strcmp("UPDATE", temp) == 0)
    {
        update_parser(tid, command);
    }
}

void select_parser(int tid, char *command)
{
    char *temp;
    char column_name[500] = "";
    char where_column[100];
    int distinct_f = 0;
    //int i = 0;
    while ((temp = strtok_r(command, " ;", &command)))
    {
        if (strcmp("DISTINCT", temp) == 0)
        {
            distinct_f = 1;
        }
        else if (strcmp("*", temp) == 0)
        {
            strcpy(column_name, temp);
        }
        else if (strcmp("TABLE", temp) == 0)
        {
            // fprintf(stderr,"Table Token\n");
        }
        else if (strcmp("WHERE", temp) == 0)
        {
            memset(temp, 0, strlen(temp));
            temp = strtok_r(command, ";", &command);

            strcpy(where_column, temp);
        }
        else if (strcmp("FROM", temp) == 0)
        {
            // fprintf(stderr,"From Token\n");
        }
        else
        {
            strcat(column_name, temp);
            // fprintf(stderr,"Column names : %s\n",column_name);
        }

        memset(temp, 0, strlen(temp));
    }

    select_Q(tid, distinct_f, column_name);
}

void select_Q(int tid, int distinct, char *column_name)
{
    pthread_mutex_lock(&mutex);

    while ((AW + WW) > 0)
    {
        WR++;
        pthread_cond_wait(&okToRead, &mutex);
        WR--;
    }
    AR++;

    pthread_mutex_unlock(&mutex);
    char **result_table = (char **)calloc(table_lenght, sizeof(char *));
    char **distinct_table = (char **)calloc(table_lenght, sizeof(char *));
    int i = 0, j = 0;//k = 0;

    for (i = 0; i < table_lenght; i++)
    {
        result_table[i] = (char *)calloc(1024, sizeof(char));
        distinct_table[i] = (char *)calloc(1024, sizeof(char));
    }

    if (strcmp(column_name, "*") == 0)
    {

        for (i = 0; i < table_lenght; i++)
        {
            for (j = 0; j < column_number; j++)
            {
                strcat(result_table[i], "\t");
                strcat(result_table[i], table[j][i]);
            }

            //fprintf(stderr, "%s\n", result_table[i]);
        }
    }
    else if (distinct == 0)
    {

        char *temp;
        while ((temp = strtok_r(column_name, ",\n", &column_name)))
        {
            int index = 0;
            /*fprintf(stderr,"TEMP : %s\n",temp);
            fprintf(stderr,"SAVEPTR : %s\n",column_name);*/

            while (strcmp(temp, table[index][0]) != 0)
            {
                /* fprintf(stderr,"Index :%d temp :%s   %s\n",index,temp,table[index][0]);  */
                index++;
            }

            for (i = 0; i < table_lenght; i++)
            {
                strcat(result_table[i], "\t");
                strcat(result_table[i], table[index][i]);
            }

            memset(temp, 0, strlen(temp));
        }
    }
    else if (distinct == 1)
    {
        char *temp;
        //int contains_f = 0;
        while ((temp = strtok_r(column_name, ",\n", &column_name)))
        {
            int index = 0;
            /*fprintf(stderr,"TEMP : %s\n",temp);
            fprintf(stderr,"SAVEPTR : %s\n",column_name);*/

            while (strcmp(temp, table[index][0]) != 0)
            {
                /* fprintf(stderr,"Index :%d temp :%s   %s\n",index,temp,table[index][0]);  */
                index++;
            }

            for (i = 0; i < table_lenght; i++)
            {
                strcat(distinct_table[i], "\t");
                strcat(distinct_table[i], table[index][i]);
            }

            memset(temp, 0, strlen(temp));
        }

        strcpy(result_table[0], distinct_table[0]);
        strcpy(result_table[1], distinct_table[1]);

        for (i = 2; i < table_lenght; i++)
        {
            for (j = 0; j < i; j++)
            {
                if (strcmp(result_table[i], distinct_table[j]) == 0)
                {
                    break;
                }
            }

            if (i == j)
            {
                //fprintf(stderr,"%s\n",result_table[i]);
                strcpy(result_table[i], distinct_table[j]);
            }
        }
    }

    char selectmsg[500];
    sprintf(selectmsg, "Thread#%d : query completed %d records have been returned\n", tid, table_lenght);
    print_log(selectmsg,output_fd);

    

    memset(selectmsg, 0, strlen(selectmsg));
    sprintf(selectmsg, "%d", table_lenght);
    write(threadParams[tid].socketFD, selectmsg, strlen(selectmsg));

    for (i = 0; i < table_lenght; i++)
    {
        memset(selectmsg, 0, strlen(selectmsg));
        sprintf(selectmsg, "%s", result_table[i]);
        write(threadParams[tid].socketFD, selectmsg, strlen(selectmsg));
    }

    pthread_mutex_lock(&mutex);
    AR--;
    if (AR == 0 && WW > 0)
        pthread_cond_signal(&okToWrite); // prioriy writers
    pthread_mutex_unlock(&mutex);

    for (i = 0; i < table_lenght; i++)
    {
        free(result_table[i]);
        free(distinct_table[i]);
    }
    free(result_table);
    free(distinct_table);
}

void update_parser(int tid, char *command)
{
    char *temp;
    char column_name[500] = "";
    char where_column[100];
    //int distinc_f = 0;
   // int howManyEntryEffected = 0;

    while ((temp = strtok_r(command, " ;", &command)))
    {
        if (strcmp("TABLE", temp) == 0)
        {
            //fprintf(stderr,"Table Token\n");
        }
        else if (strcmp("SET", temp) == 0)
        {
            //fprintf(stderr, "Set Token\n");
        }
        else if (strcmp("WHERE", temp) == 0)
        {
            memset(temp, 0, strlen(temp));
            temp = strtok_r(command, ";", &command);

            //fprintf(stderr,"Where Columns : %s\n",temp);
            strcpy(where_column, temp);
        }
        else
        {
            strcat(column_name, temp);
            //fprintf(stderr,"Column names : %s\n",column_name);
        }

        memset(temp, 0, strlen(temp));
    }

    update_Q(tid,column_name, where_column);

    //return howManyEntryEffected;
}

void update_Q(int tid, char *column_name, char *where_column)
{

    pthread_mutex_lock(&mutex);
    while ((AW + AR) > 0)
    {         // if any readers or writers, wait
        WW++; // waiting writer
        pthread_cond_wait(&okToWrite, &mutex);
        WW--;
    }
    AW++; // active writer
    pthread_mutex_unlock(&mutex);

    int effected_row = 0;
    char *temp = "";
    int where_column_index = 0;

    //fprintf(stderr,"Update_Q Where columns: %s\n",where_column);
    temp = strtok_r(where_column, "=", &where_column);
    //fprintf(stderr,"Temp columns: %s\n",temp);

    if (where_column[0] == '\'' || where_column[0] == 39)
    {
        where_column++;
        where_column[strlen(where_column) - 1] = '\0';
    }
    //fprintf(stderr,"Where columns: %s\n",where_column);
    while (strcmp(table[where_column_index][0], temp) != 0)
    {
        where_column_index++;
    }

    //fprintf(stderr,"Where columns index: %d\n",where_column_index);

    while ((temp = strtok_r(column_name, ",", &column_name)))
    {
        char *temp_t = "";
        int temp_index = 0;
        int row_count = 0;
        int i = 0;// j = 0;

        //fprintf(stderr,"TEMP columns: %s\n",temp);
        temp_t = strtok_r(temp, "=", &temp);
        //fprintf(stderr,"TEMP_T: %s\n",temp_t);

        if (temp[0] == '\'' || temp[0] == 39)
        {

            temp++;
            temp[strlen(temp) - 1] = '\0';
        }

        //fprintf(stderr,"TEMP columns: %s\n",temp);

        while (strcmp(table[temp_index][0], temp_t) != 0 && temp_index < column_number)
        {
            temp_index++;
        }
        //fprintf(stderr,"Temp columns index: %d\n",where_column_index);

        for (i = 0; i < table_lenght; i++)
        {
            if (strcmp(table[where_column_index][i], where_column) == 0)
            {
                strcpy(table[temp_index][i], temp);
                row_count++;
            }
        }

        memset(temp, 0, strlen(temp));
        effected_row = row_count;
    }

    char updatemsg[500];
    sprintf(updatemsg, "Thread#%d: query completed. %d records updated\n", tid, effected_row);
    print_log(updatemsg,output_fd);

    write(threadParams[tid].socketFD, "1", 2);
    write(threadParams[tid].socketFD, updatemsg, strlen(updatemsg));

    AW--;
    if (WW > 0) // give priority to other writers
        pthread_cond_signal(&okToWrite);
    else if (WR > 0)
        pthread_cond_broadcast(&okToRead);
    pthread_mutex_unlock(&mutex);
}