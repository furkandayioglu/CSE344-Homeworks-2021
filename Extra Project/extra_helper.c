/**
 * @file extra_helper.c
 * @author Furkan Sergen Dayioglu
 * @brief 
 * @version 0.1
 * @date 2021-08-09
 * 
 * @copyright Copyright (c) 2021
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

void print_ts(char *msg)
{
    char buffer[500];

    struct tm *currentTime;
    time_t timeCurrent = time(0);
    currentTime = gmtime(&timeCurrent);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S ", currentTime);
    strcat(buffer, msg);
    write(2, buffer, strlen(buffer));
}

void print_usage()
{
    print_ts("##USAGE##\n");
    print_ts("Invalid Amount of parameter\n");
    print_ts("./client -i ID -a 127.0.0.1 -p PORT -o pathToQueryFile\n");
}



/**
 * This part, getCofactor and Determinanat, is inspired from geeksforgeeks.
 * https://www.geeksforgeeks.org/determinant-of-a-matrix/
 * 
 */
void getCofactor(int** matrix,int** temp, int n, int i,int j){
    
    int temp_i = 0, temp_j=0;

}


int determinant(int** matrix, int n){
    int det = 0;
    int sign = 1;
    int** temp;

    
    if(n==1){
        return matrix[0][0];        
    }


}