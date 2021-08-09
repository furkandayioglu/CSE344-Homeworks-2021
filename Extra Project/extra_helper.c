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
    int row=0, col=0;

    for(row=0; row<n; row++){
        for(col=0; col<n; col++){
            if (row !=i && col !=j){
                temp[temp_i][temp_j++]=matrix[row][col];

                if(temp_j == n-1){
                    temp_j = 0;
                    temp_i++;
                }
            }
        }
    }
}


int determinant(int** matrix, int n){
    int det = 0;
    int sign = 1;
    int** temp;
    int i=0;
    
    
    if(n==1){
        return matrix[0][0];        
    }

    temp = (int**)calloc(n,sizeof(int*));

    for(i=0;i<n;i++){
        temp[i]= (int*)calloc(n,sizeof(int));
    }

    
    for(i=0;i<n;i++){
        getCofactor(matrix,temp,n,0,i);
        det += sign * matrix[0][i] * determinant(temp,n-1);
        sign = -sign;
    }


    for(i=0;i<n;i++)
        free(temp[i]);
    free(temp);


    return det;    
}

int dim_size(char* filename){
    int count=0;
    char temp,temp_pr;
    int stat;
    FILE* input;

    input = fopen(filename,"r");

    if(input == NULL){
        fprintf(stderr,"Input file could not be opened.\n");
        fprintf(stderr,"Terminating...\n");
        exit(-1);
    }

    stat=fscanf(input,"%c",&temp);
    while(stat != EOF){
        if(temp == '\n')
            count++;
        temp_pr=temp;
        stat=fscanf(input,"%c",&temp);
    }

    if(temp_pr!='\n')
        count++;

    fclose(input);
    
    return count;
}

void fill_matrix(int**matrix,int dim,char* filename){
    FILE* input;
    int num;
    char temp;
    int i=0,j=0;
    input = fopen(filename,"r");

    if(input == NULL){
        fprintf(stderr,"Input file could not be opened.\n");
        fprintf(stderr,"Terminating...\n");
        exit(-1);
    }

    for(i=0;i<dim;i++){
        for(j=0;j<dim;j++){
            fscanf(input,"%d%c",&num,&temp);
            matrix[i][j]=num;
        }
    }


    fclose(input);
}
int main(int argc, char**argv){
    int opt;
    char* filepath;
    int dim=0;
    int det=0;
    int **matrix;
    int i,j;

    dim = dim_size("../matrix1.csv");

    fprintf(stderr,"line count : %d\n",dim);

    matrix = (int**)calloc(dim,sizeof(int*));

    for(i=0;i<dim;i++){
        matrix[i]= (int*)calloc(dim,sizeof(int));
    }

    fill_matrix(matrix,dim,"../matrix1.csv");

    for(i=0;i<dim;i++){
        for(j=0;j<dim;j++){
            fprintf(stderr,"%d ",matrix[i][j]);
            
        }

         fprintf(stderr,"\n");
    }


    det=determinant(matrix,dim);

    fprintf(stderr,"Determinant of Matrix : %d\n",det);

    for(i=0;i<dim;i++){
        free(matrix[i]);
    }
    free(matrix);
    return 0;
}