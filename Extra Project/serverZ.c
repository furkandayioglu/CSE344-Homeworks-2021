/**
 * @file serverZ.c
 * @author Furkan Sergen Dayioglu
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

/*  Data Structures */

typedef struct threadPool_t{
    int id;
    int status;
    int socketfd;
    pthread_mutex_t mutex;
}threadPool_t;


/* Thread Function */
void *pool_func(void* arg);




/* Functions */

/* These functions are taken from geeksforgeeks 
   https://www.geeksforgeeks.org/determinant-of-a-matrix/ */

void cofactors(int** matrix, int** temp, int size, int i, int j);
int determinant(int** matrix,int size);
void signal_handler(int signo);


/* Helpers */ 
void print_ts(char* msg);
void print_usage();
void threadPool_init();
void check_instance();

/* Variables */

pthread_t* pool;
threadPool_t* threadParams; /* Threads parameters */ 
pthread_mutex_t main_mutex = PTHREAD_MUTEX_INITIALIZER; /* In order to create critical region to write into file*/
pthread_mutex_t full_mutex = PTHREAD_MUTEX_INITIALIZER; /* find available thread */ 


char* logFile;
int pool_size;
int port;
int sleep_dur;

int sig_int_flag=0;
/* counter semaphores */
sem_t pool_full;
sem_t invertible_counter;
sem_t non_invertible_counter;

int logFD;
int socketfd;

int main(int argc, char** argv){

    int opt;
    int i=0;
    struct sigaction sa;
    
    memset(&sa,0,sizeof(sa));
    sa.sa_handler=&signal_handler;

    sigaction(SIGINT,&sa,NULL);


    check_instance();
    if (argc != 9)
    {
        print_usage();
        print_ts("Terminating...\n");
        exit(-1);
    }

    while ((opt = getopt(argc, argv, ":p:o:m:t:")) != -1)
    {
        switch (opt)
        {
        case 'p':
            port = atoi(optarg);
            break;
        case 'o':
            logFile = optarg;
            break;
        case 'm':
            pool_size = atoi(optarg);
            break;
        case 't':
            sleep_dur = atoi(optarg);
            break;
        case ':':
            print_usage();
            return -1;
        case '?':
            print_usage();
            return -1;
        }
    }

    if (port <= 1000)
    {
        print_ts("PORT must be greater than 1000.\n");
        print_ts("PORTs that less than 1000 are reserved for kernel processes\n");
        print_ts("Terminating...\n");
        exit(-1);
    }

    if(pool_size<2){
        print_ts("Pool size must be greater than 1000.\n");
        print_ts("Terminating...\n");
        exit(-1);
    }


    /* Deamonize */



   
    /* server creation */ 
    struct sockaddr_in serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));
    if((socketfd = socket(AF_INET, SOCK_STREAM, 0))<0){		// socket initialize
	    print_ts("socket error!\n");
        print_ts("Terminating...\n");
	    exit(-1);
    }
    serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serv_addr.sin_port = htons(port); 

	if((bind(socketfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0){	// bind socket
	    print_ts("bind error!\n");
        print_ts("Terminating...\n");	
	    exit(-1);
	}

    if((listen(socketfd, 20)) < 0){			// listen socket
	    print_ts("listen error!\n");
        print_ts("Terminating...\n");
	    exit(-1);
	}

    char msg_srv_init[513];
    sprintf(msg_srv_init,"Z:Server Z (127.0.0.1:%d, %s, t=%d, m=%d) started\n",port,logFile,sleep_dur,pool_size);
    print_ts(msg_srv_init);




    /* Create threads */
    pool = (pthread_t *)calloc(pool_size, sizeof(pthread_t));
    threadParams = (threadPool_t *)calloc(pool_size, sizeof(threadPool_t));
    for(i=0;i<pool_size;i++){
        if (pthread_create(&pool[i], NULL, pool_func, (void *)i) != 0)
            {
                print_ts("Thread Creation failed\n");
                exit(-1);
            }
    }
    threadPool_init();

    /* Main Thread */
    while (1){

        socklen_t socket_len = sizeof(serv_addr);
        int acceptfd = accept(socketfd,(struct sockaddr*)&serv_addr,&socket_len);




    }


    /* Clean the mess */

    for(i=0; i<pool_size;i++){
        void * ptr = NULL;
        int stat;
		stat=pthread_join(pool[i],&ptr); 						
		if(stat!=0) 
			print_ts(" Error p_thread Join");
    }

    close(socketfd);
    close(logFD);
    free(threadParams);

    return 0;
}

void print_ts(char* msg){
    char buffer[500];

    struct tm *currentTime;
    time_t timeCurrent = time(0);
    currentTime = gmtime(&timeCurrent);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S ", currentTime);
    strcat(buffer, msg);
    write(2, buffer, strlen(buffer));

}

void print_usage(){
    print_ts("##USAGE##\n");
    print_ts("Invalid Amount of parameter\n");
    print_ts("./serverZ -p PORT -o PathLogFile -m poolsize -t sleepduration\n");
}

void check_instance(){
    int pid_file = open("/tmp/serverZ.pid", O_CREAT | O_RDWR,0666);
    int res = flock(pid_file, LOCK_EX | LOCK_NB);

    if(res){
        if(EAGAIN == errno){
            print_ts("Z: Server Z already running...\n");
            print_ts("Z: Terminating...\n");
            exit(-1);
        }
    }
}
void threadPool_init(){
    int i=0;

    for(i=0;i<pool_size;i++){
        threadParams[i].id=i;
        threadParams[i].status=0;
        threadParams[i].socketfd=0;
        if(pthread_mutex_init(&threadParams[i].mutex,NULL)!=0){								// initialize mutex and condition variables
			print_ts("mutex initialize failed!!!. Program will finish.");
			exit(-1);
		}
    }

}

void cofactor(int** matrix,int** temp,int size,int i, int j){
    int temp_i = 0, temp_j=0;
    int row=0, col=0;

    for(row=0; row<size; row++){
        for(col=0; col<size; col++){
            if (row !=i && col !=j){
                temp[temp_i][temp_j++]=matrix[row][col];

                if(temp_j == size-1){
                    temp_j = 0;
                    temp_i++;
                }
            }
        }
    }

}

int determinant(int**matrix, int size){
    int det = 0;
    int sign = 1;
    int** temp;
    int i=0;
    
    
    if(size==1){
        return matrix[0][0];        
    }

    temp = (int**)calloc(size,sizeof(int*));

    for(i=0;i<size;i++){
        temp[i]= (int*)calloc(size,sizeof(int));
    }

    
    for(i=0;i<size;i++){
        cofactor(matrix,temp,size,0,i);
        det += sign * matrix[0][i] * determinant(temp,size-1);
        sign = -sign;
    }


    for(i=0;i<size;i++)
        free(temp[i]);
    free(temp);


    return det;

}

void *pool_func(void* arg){
    int size;
    int i=0,j=0;
    int** matrix;
    int id = (int) arg;
    
    char msg[100];
	sprintf(msg,"Z: Thread #%d: waiting for connection\n",id);
    print_ts(msg);

    while(1){
        print_ts(msg);
        if(sig_int_flag == 1){
            break;
        }


    }

}