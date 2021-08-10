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

/* Data Structures */

typedef struct threadPoolY_t{
    int id;
    int status;
    int socketfd;
}threadPoolY_t;

typedef struct threadPoolZ_t{
    int id;
    int status;
    int socketfd;
    int zsocketfd;
}threadPoolZ_t;

/* Wait Queue */

typedef struct node_t{
    int client_pid;
    int matrix_size;
    node_t* next;
}node_t;

typedef struct waitqueue_t{
    node_t* head;
    node_t* tail;
}waitqueue_t;


/* Thread Function */

void pool1_func();
void pool2_func();



/* Functions */

/* These functions are taken from geeksforgeeks 
   https://www.geeksforgeeks.org/determinant-of-a-matrix/ */

void cofactors(int** matrix, int** temp, int size, int i, int j);
int determinant(int** matrix,int size);



/* Helpers */ 





/* Variables */

pthread_t* pool1;
pthread_t* pool2;


/* counter semaphores */
sem_t pool1_full;
sem_t pool2_full;
sem_t invertible_counter;
sem_t non_invertible_counter;

char* ipAddr;
char* logFile;
int pool1_size;
int pool2_size;
int port;
int sleep_dur;

int logFD;

int main(int argc, char** argv){



}


void cofactor(int** matrix,int** temp,int size,int i, int j){


}

int determinant(int**matrix, int size){

}