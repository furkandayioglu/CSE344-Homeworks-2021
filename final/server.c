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




/* SQL FUNCTIONALITIES*/

/*THREAD FUNCTIONS*/

pthread_t *pool_threads;
threadP_t *threadParams; /* thread_params */



/* select*/
char** reader();
/* Update*/
int writer();

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
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;          /* initialize mutex */
pthread_mutex_t mainMutex = PTHREAD_MUTEX_INITIALIZER;      /* initialize main mutex controls threads job assignment loop */
pthread_mutex_t m_pool_full = PTHREAD_MUTEX_INITIALIZER;    /* check if all threads are busy */
pthread_cond_t c_pool_full = PTHREAD_COND_INITIALIZER;      /* signal if any thread gets available */


/* COMMANDLINE ARGUMENTS */
int port;
char *pathToLogFile;
int poolSize;
char *datasetPath;

int output_fd;

/* Some Essential*/
void print_usage();
void print_log(char *msg);

void check_instance();
int create_daemon();

void sigIntHandle(int sig);
static volatile sig_atomic_t sig_int = 0;

char *instance;
key_t key;
int shmid;

int main(int argc, char **argv)
{
    int opt;
   
    pid_t pid = 0, snw = 0;

    /*LOG PATH*/
    output_fd = open("logfile.log", O_WRONLY | O_CREAT | O_EXCL | O_APPEND, S_IRUSR | S_IWUSR);

    /*Shared memory usage to prevent multiple instantion*/
    key = ftok("serverNoSecInstance", 123);
    shmid = shmget(key, 1024, 0666 | IPC_CREAT);
    instance = (char *)shmat(shmid, (void *)0, 0);

    if (strcmp(instance, "CW") == 0)
    {
        print_log("already working\n");
        exit(-1);
    }

    /* Determine that Currently Working */
    instance[0] = 'C';
    instance[1] = 'W';

    /*Daemon Creatation */
    pid = fork(); // Create child process
    if (pid < 0)
    { // Check Fail
        print_log("Fork failed!\n");
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

        print_log("setsid() error!\n");
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
        char socBuf[1025]="";
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
                print_log("ınvalid Parameter\n");
                print_usage();
                exit(-1);
                break;
            }
        }

        if (port <= 1000)
        {
            print_log("First 1000 ports are being used by kernel.\n");
            print_log("That means, you entered invalid port.\n");
            print_log("Terminating...\n");
            exit(-1);
        }
        close(STDIN_FILENO); // close inherited files
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        close(close_fd);

        /* Main thread execution params to log file */
        print_log("Execution with parametes:\n");
        sprintf(param1, "-p %d\n", port);
        print_log(param1);
        sprintf(param2, "-o %s\n", pathToLogFile);
        print_log(param2);
        sprintf(param3, "-l %d\n", poolSize);
        print_log(param3);
        sprintf(param4, "-d %s\n", datasetPath);
        print_log(param4);

        signal(SIGINT,sigIntHandle); 

        print_log("Loading dataset...\n");

        gettimeofday(&tm1, NULL);
        column_number = number_of_columns(datasetPath);
        table_lenght = line_number(datasetPath);

        /*initialize table*/
        table = (char ***)calloc(column_number ,sizeof(char **));

        for (i = 0; i < column_number; i++)
        {
            table[i] = (char **)calloc(table_lenght , sizeof(char *));
            for (j = 0; j < table_lenght; j++)
                table[i][j] = (char *)calloc(150 , sizeof(char));
        }

        read_csv_file(datasetPath);

        pool_threads = (pthread_t *)calloc(poolSize , sizeof(pthread_t));
        threadParams = (threadP_t*) calloc(poolSize,sizeof(threadP_t));
        threadP_Array();

        gettimeofday(&tm2, NULL);

        time_interval = (double)(tm2.tv_sec - tm1.tv_sec) + (double)(tm2.tv_usec - tm1.tv_usec) / 1000000;
        sprintf(loadMsg, "Dataset loaded in %.2f seconds with %d entries", time_interval, table_lenght);
        print_log(loadMsg);

        sprintf(tcMsg, "A pool of %d threads has been created.\n", poolSize);
        print_log(tcMsg);

        for (i = 0; i < poolSize; i++)
        {
            if (pthread_create(&pool_threads[i], NULL, thread_func, (void *)i) != 0)
            {
                print_log("Thread Creation failed\n");
                eixt(-1);
            }
        }

        /* Socket \m/ */

        socketFd = 0;
        memset(&serv_adr,'0',sizeof(serv_adr));
        memset(socBuf,'0',sizeof(socBuf));


        if((socketFd = socket(AF_INET,SOCK_STREAM,0)<0)){
            print_log("Socket Initialization failed \n");
            print_log("Terminating....\n");
            exit(-1);
        }

        serv_adr.sin_family = AF_INET;
        serv_adr.sin_addr.s_addr = inet_addr(socket_ip);
        serv_adr.sin_port = htons(port);



        if( (bind(socketFd,(struct sockaddr*)&serv_adr,sizeof(serv_adr))) <0 ){
            print_log("Socket Binding error \n");
            print_log("Terminating...");
            exit(-1);

        }

        if((listen(socketFD,20))<0){
            print_log("Listen Error\n");
            print_log("Terminating...\n");
            exit(-1);
        }



        while(1){

            socklen_t soclen = sizeof(serv_adr);
            int accept_fd = accept(sockerFd,(struct sockaddr*)&serv_adr,&len);

            pthread_mutex_lock(&mainMutex);
            int busy_Flag = 0;

            for(i=0;i<poolSize;i++){
                if(threadParams[i].status == 0){
                    char threadMsg[500];
                    threadParams[i].status=1;
                    threadParams[i].socketFD = accept_fd;
                    pthread_cond_signal(threadParams[i].cond);


                }

            }



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
void print_log(char *msg)
{
    char buffer[500];

    struct tm *currentTime;
    time_t timeCurrent = time(0);
    currentTime = gmtime(&timeCurrent);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S ", currentTime);
    strcat(buffer, msg);
    write(output_fd, buffer, strlen(buffer));
}

void print_usage()
{
    print_log("#USAGE#\n");
    print_log("Invalid amount of parameters\n");
    print_log("server -p PORT -o PathToLogFile -l poolSize -d datasetPath\n");
    print_log("Terminating...\n");
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
        print("Dataset File could not be opened\n");
        print("Terminating...\n");
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
        print_log("Dataset File could not be opened\n");
        print_log("Terminating...\n");
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
        print_log("DatasetFile could not be opened\n");
        print_log("Terminating....\n");
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

void threadP_Array(){
    int i = 0;

    for(i=0;i<poolSize;i++){
        threadParams[i].id = i;
        threadParams[i].status = 0;
        threadParams[i].socketFD=0;
        if(pthread_mutex_init(&threadParams[i].mutex,NULL)!=0){
            print_log("Thread Params Mutex Initialization failed\n");
            print_log("Terminating...\n");
            exit(-1);
        }
           
        if(pthread_cond_init(&threadParams[i].cond,NULL)!=0){
            print_log("Thread Params Condition Variable Initialization failed\n");
            print_log("Terminating...\n");
            exit(-1);
        }
    }   
}
