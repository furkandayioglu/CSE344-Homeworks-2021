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


char***table;
int column_number;
int table_lenght;


/* TABLE INITIALIZATION & CSV PARSER FUNCTIONS*/
int number_of_columns(char*filename);
int line_number(char* filename);
void read_csv_file(char* filename);
void line_to_table(char* line, int line_number);



/* SQL FUNCTIONALITIES*/





/*THREAD FUNCTIONS*/




/*READER WRITER SYNCRONIZERS*/





/* COMMANDLINE ARGUMENTS */
int port;
char* pathToLogFile;
int poolSize;
char* datasetPath;

int output_fd;

/* Some Essential*/
void print_usage();
void print_log (char* msg);

void check_instance();
int create_daemon();


void sigIntHandle(int sig);
static volatile sig_atomic_t sig_int = 0;

int main(int argc,char**argv){
    int opt;
    char* instance;
    key_t key;
    int shmid;
    pid_t pid = 0,snw = 0;


    /*LOG PATH*/
    output_fd = open("logfile.log",O_WRONLY|O_CREAT |O_EXCL| O_APPEND,S_IRUSR | S_IWUSR);
    
    /*Shared memory usage to prevent multiple instantion*/
    key = ftok("serverNoSecInstance",123);
    shmid = shmget(key,1024,0666|IPC_CREAT);
    instance = (char*)shmat(shmid,(void*)0,0);


    if(strcmp(instance,"CW")==0){
        print_log("already working\n");
        exit(-1);
    }
 
    /* Determine that Currently Working */
    instance[0] = 'C';
    instance[1] = 'W';

    /*Daemon Creatation */
    pid = fork();				// Create child process
	if (pid < 0){				// Check Fail
		print_log("Fork failed!\n");
		exit(EXIT_FAILURE);
	}
	if (pid > 0){
		exit(EXIT_SUCCESS);		// parent process exit
	}	
	umask(0);					// unmask file mode
	snw = setsid();				// set new session
	if(snw < 0){
        
		print_log("setsid() error!\n");
		exit(EXIT_FAILURE);
	}

    /* We are daemon Hallelujah \m/ */
    if(getpid() == syscall(SYS_gettid)){

        /* variables declaration area */
        int opt;
        char param1[100];
        char param2[100];
        char param3[100];
        char param4[100];
        char loadMsg[100];
        int close_fd=0;
        int i,j;
        char socket_ip[]="127.0.0.1"; /* Run on Local */ 
        double time_interval;
        struct timeval  tm1, tm2;

        if(argc!=9){
            print_usage();
            exit(-1);
        }

        while((opt = getopt(argc,argv,":p:o:l:d:"))){
        
            switch(opt){
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



        if(port <=1000){
            print_log("First 1000 ports are being used by kernel.\n");
            print_log("That means, you entered invalid port.\n");
            printf_log("Terminating...\n");
            exit(-1);
        }
        close(STDIN_FILENO);			// close inherited files
		close(STDOUT_FILENO);			
		close(STDERR_FILENO);
		
		close(close_fd);
		

        /* Main thread execution params to log file */
        print_log("Execution with parametes:\n");        
        sprintf(param1,"-p %d\n",port);
        print_log(param1);        
        sprintf(param2,"-o %s\n",pathToLogFile);
        print_log(param2);        
        sprintf(param3,"-l %d\n",poolSize);
        print_log(param3);        
        sprintf(param4,"-d %s\n",datasetPath);
        print_log(param4);

        print_log("Loading dataset...\n");

        gettimeofday(&tm1,NULL);
        column_number= number_of_columns(datasetPath);
        table_lenght = line_number(datasetPath);

        /*initialize table*/
         table = (char***)malloc(column_number*sizeof(char**));

        for(i=0;i<column_number;i++){
            table[i] = (char**)malloc(table_lenght*sizeof(char*));
            for(j=0;j<table_lenght;j++)
                table[i][j] = (char*) malloc(150*sizeof(char));
        }

        read_csv_file(datasetPath);
        gettimeofday(&tm2,NULL);
        
        time_interval = (double) (tm2.tv_sec - tm1.tv_sec) + (double) (tm2.tv_usec - tm1.tv_usec) / 1000000 ;
        sprintf(loadMsg,"Dataset loaded in %.2f seconds with %d entries",time_interval,table_lenght);
        print_log(loadMsg);



        /* Free DB Table */
        for(i=0;i<column_number;i++){
            for(j=0;j<table_lenght;j++){
                free(table[i][j]);
            }
        }
        for(i=0;i<column_number;i++){
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

void print_log(char* msg){
    char buffer [500];

    struct tm *currentTime;
    time_t timeCurrent = time(0);
    currentTime = gmtime (&timeCurrent);
    
    strftime (buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S ", currentTime);
    strcat(buffer,msg);
    write(output_fd,buffer,strlen(buffer));


}


void print_usage(){
    print_log("#USAGE#\n");
    print_log("Invalid amount of parameters\n");
    print_log("server -p PORT -o PathToLogFile -l poolSize -d datasetPath\n");
    print_log("Terminating...\n");
}

int number_of_columns(char* filename){

    FILE* input = fopen(filename,"r");
    char line[500];
    char* strtok_ptr=NULL;
    char* temp;
    int i=0;
    
    

    if(input==NULL){
        print("Dataset File could not be opened\n");
        print("Terminating...\n");
        exit(-1);
    }
   
    fgets(line,sizeof(line),input);
    line[strlen(line)-1]='\0';
    
    strtok_ptr=line;

    while((temp=strtok_r(strtok_ptr,",",&strtok_ptr))){        
        i++;
    }  

   
    fclose(input);
    return i;
}

int line_number(char*filename){

    FILE*input=fopen(filename,"r");
    int i=0;
    char line[500];

    if(input==NULL){
        print_log("Dataset File could not be opened\n");
        print_log("Terminating...\n");
        exit(-1);
    }

    while(fgets(line,sizeof(line),input) != NULL){
        i++;
                
    }

    fclose(input);

    return i;
}

void read_csv_file(char*filename){
    int i=0;
    FILE*input=fopen(filename,"r");
    char line[500];   
   

    if(input==NULL){
        print_log("DatasetFile could not be opened\n");
        print_log("Terminating....\n");
        exit(-1);
        
    }

    while(fgets(line,sizeof(line),input)!=NULL){     
       
        line[strlen(line)-1]='\0';       
        line_to_table(line,i);
        i++;
        memset(line,0,strlen(line));

    }

    fclose(input);
}


void line_to_table(char* line,int line_number){
    int i=0;
    char* temp;   
    
    for(i=0;i<column_number;i++){
        temp = strtok_r(line,",",&line);
        strcpy(table[i][line_number],temp);       
    }
      
}
    
    