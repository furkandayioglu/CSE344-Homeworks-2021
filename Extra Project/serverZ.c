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
}threadPool_t;


/* Thread Function */
void pool1_func();




/* Functions */

/* These functions are taken from geeksforgeeks 
   https://www.geeksforgeeks.org/determinant-of-a-matrix/ */

void cofactors(int** matrix, int** temp, int size, int i, int j);
int determinant(int** matrix,int size);
void signal_handler(int signo);


/* Helpers */ 

void print_ts(char* msg);
void print_usage();
/* Variables */

pthread_t* pool1;
/*char* ipAddr;*/
char* logFile;
int pool_size;
int port;
int sleep_dur;

sem_t pool_full;
sem_t invertible_counter;
sem_t non_invertible_counter;

int logFD;


int main(int argc, char** argv){

    int opt;
    int i=0;
    struct sigaction sa;
    
    memset(&sa,0,sizeof(sa));
    sa.sa_handler=&signal_handler;

    sigaction(SIGINT,&sa,NULL);

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




    return 0;
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