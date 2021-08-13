/**
 * @file serverY.c
 * @author Furkan Sergen Dayioglu
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
#include <stdatomic.h>
#include "myqueue.h"

#define SERVERBACKLOG 100

/* Data Structures */

typedef struct threadPoolY_t{
    int id;
    int full;
    int status;
    int socketfd;
    pthread_mutex_t mutex;
    
}threadPoolY_t;

typedef struct threadPoolZ_t{
    int id;
    int status;
    int full;
    int socketfd;
    int zsocketfd;
    pthread_mutex_t mutex;
}threadPoolZ_t;



/* Thread Function */

void *pool1_func(void* arg);
void *pool2_func(void* arg);



/* Functions */

/* These functions are taken from geeksforgeeks 
   https://www.geeksforgeeks.org/determinant-of-a-matrix/ */

void cofactors(int** matrix, int** temp, int size, int i, int j);
int determinant(int** matrix,int size);
void signal_handler(int signo);
void check_instance();
static void becomedeamon();
/* Helpers */ 

void print_ts(char *msg,int fd);
void print_usage();
void threadParams_init();
threadPoolY_t* threadParamsY;
threadPoolZ_t* threadParamsZ;

/* Variables */

pthread_t* pool1;
pthread_t* pool2;
pthread_mutex_t main_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pool1_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pool2_mutex = PTHREAD_MUTEX_INITIALIZER;


/* counter  */
atomic_int pool1_full=0;
atomic_int pool2_full=0;
atomic_int invertible_counter;
atomic_int non_invertible_counter;

static volatile sig_atomic_t sig_int_flag=0;

char* ipAddr;
char* logFile;
int pool1_size;
int pool2_size;
int port;
int sleep_dur;

int logFD;

int serverZ_pid;

int main(int argc, char** argv){

    int opt;
    int i;
    struct sigaction sa;
    
    memset(&sa,0,sizeof(sa));
    sa.sa_handler=&signal_handler;

    sigaction(SIGINT,&sa,NULL);

    check_instance();    

     if(argc != 11){
        print_usage();
        print_ts("Terminating...\n",2);
        exit(-1);
    }

    while ((opt = getopt(argc, argv, ":p:o:l:m:t:")) != -1)
    {
        switch (opt)
        {
        case 'p':
            port = atoi(optarg);
            break;
        case 'o':
            logFile = optarg;
            break;
        case 'l':
            pool1_size = atoi(optarg);
            break;
        case 'm':
            pool2_size = atoi(optarg);
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
        print_ts("PORT must be greater than 1000.\n",2);
        print_ts("PORTs that less than 1000 are reserved for kernel processes\n",2);
        print_ts("Terminating...\n",2);
        exit(-1);
    }

    if(pool1_size<2 || pool2_size<2){
        print_ts("Pool size must be greater than 1000.\n",2);
        print_ts("Terminating...\n",2);
        exit(-1);
    }


  


    /* Daemonize*/

    //becomedeamon();    

    /* Sockets*/


    /*Start Server Z */
    
    /* get serverZ Pid*/


    /* Create Threads and initiliaze Thread Pool */  
    threadParams_init();

    pool1 = (pthread_t *)calloc(pool1_size, sizeof(pthread_t));
    threadParamsY = (threadPoolY_t *)calloc(pool1_size, sizeof(threadPoolY_t));
    for(int i=0;i<pool1_size;i++){
        if (pthread_create(&pool1[i], NULL, pool1_func, (void *)i) != 0)
            {
                print_ts("Thread Creation failed\n",2);
                exit(-1);
            }
    }

    pool2 = (pthread_t *)calloc(pool2_size, sizeof(pthread_t));
    threadParamsZ = (threadPoolZ_t *)calloc(pool2_size, sizeof(threadPoolZ_t));
    for(int i=0;i<pool2_size;i++){
        if (pthread_create(&pool2[i], NULL, pool2_func, (void *)i) != 0)
            {
                print_ts("Thread Creation failed\n",2);
                exit(-1);
            }
    }

    




    /* Main thread */










    /* clean mess */





    return 0;
}

void print_ts(char *msg,int fd)
{
    char buffer[500];
    struct flock lock;

    struct tm *currentTime;
    time_t timeCurrent = time(0);
    currentTime = gmtime(&timeCurrent);

    memset(&lock,0,sizeof(lock));
    lock.l_type = F_WRLCK;
    fcntl(fd,F_SETLKW,&lock);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S ", currentTime);
    strcat(buffer, msg);
    write(fd, buffer, strlen(buffer));

    lock.l_type= F_UNLCK;
    fcntl(fd,F_SETLKW,&lock);
}

void print_usage()
{
    print_ts("##USAGE##\n",2);
    print_ts("Invalid Amount of parameter\n",2);
    print_ts("./client -i ID -a 127.0.0.1 -p PORT -o pathToQueryFile\n",2);
}

static void becomedeamon(){
    pid_t pid;

    /* Forks the parent process */
    if ((pid = fork())<0){
        unlink("running");
        exit(-1);
    }

    /* Terminates the parent process */
    if (pid > 0){
        exit(0);
    }

    /*The forked process is session leader */
    if (setsid() < 0){
        unlink("running");
        exit(-1);
    }

    /* Second fork */
    if((pid = fork())<0){
        unlink("running");
        exit(-1);
    }

    /* Parent termination */
    if (pid > 0){
        exit(0);
    }

    /* Unmasks */
    umask(0);

    /* Appropriated directory changing */
    chdir(".");

    /* Close core  */
    close(STDERR_FILENO);
    close(STDOUT_FILENO);
    close(STDIN_FILENO);
}

void check_instance(){
    int pid_file = open("/tmp/serverY.pid", O_CREAT | O_RDWR,0666);
    int res = flock(pid_file, LOCK_EX | LOCK_NB);

    if(res){
        if(EAGAIN == errno){
            print_ts("Server Y already running...\n",2);
            print_ts("Terminating...\n",2);
            exit(-1);
        }
    }
}

void threadParams_init(){
    for(int i=0;i<pool1_size;i++){
        threadParamsY[i].id=i;
        threadParamsY[i].status=0;
        threadParamsY[i].full = 0;
        threadParamsY[i].socketfd=0;
        if(pthread_mutex_init(&threadParamsY[i].mutex,NULL)!=0){								// initialize mutex and condition variables
			print_ts("Mutex initialize failed!!!. Terminating...",2);
			exit(-1);
		}
    }

    for(int i=0;i<pool2_size;i++){
        threadParamsZ[i].id=i;
        threadParamsZ[i].status=0;
        threadParamsZ[i].full = 0;
        threadParamsZ[i].socketfd=0;
        if(pthread_mutex_init(&threadParamsZ[i].mutex,NULL)!=0){								// initialize mutex and condition variables
			print_ts("Mutex initialize failed!!!. Terminating...",2);
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

void signal_handler(int signo){

}

void* pool1_func(void* arg){}

void* pool2_func(void* arg){}