/**
 * @file client.c
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

void print_ts(char *msg)
{
    char buffer[500];

    struct tm *currentTime;
    time_t timeCurrent = time(0);
    currentTime = gmtime(&timeCurrent);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S ", currentTime);
    strcat(buffer, msg);
    write(stderr, buffer, strlen(buffer));
}
void print_usage(){
    print_ts("##USAGE##\n");
    print_ts("Invalid Amount of parameter\n");
    print_ts("./client -i ID -a 127.0.0.1 -p PORT -o pathToQueryFile\n");
}

char* id;
int id_int;
char* ipAdrr;
int port;
char* pathToQuery;
int query_count;
char** query_arr;

void line_count(char* filename);
void read_queries_from_file(char* filename);

int main(int argc,char** argv){

    int opt;
    int i=0,j=0;
    

    if(argc != 9){
        print_usage();
        print_ts("Terminating...\n");
        exit(-1);
    }
   
    while((opt= getopt(argc,argv,":i:a:p:o"))){
        switch (opt)
        {
        case 'i':
            id = optarg;
            id_int = atoi(optarg);
            break;
        case 'a':
            ipAdrr=optarg;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 'o':
            pathToQuery = optarg;
            break;       
        default:
            print_usage();
            exit(-1);
            break;
        }

    }

    if(port<=1000){
        print_ts("PORT must be greater than 1000.\n");
        print_ts("Terminating...\n");
        exit(-1);
    }

    if(id_int<1){
        print_ts("ID must be greater than 1\n");
        print_ts("Terminating\n");
        exit(-1);
    }

    line_count(pathToQuery);

    query_arr =(char**) calloc(query_count,sizeof(char*));
    for(i=0;i<query_count;i++)
        query_arr[i] = (char*) calloc(500,sizeof(char));

    read_queries_from_file(pathToQuery);

    int sockfd = 0;
    int readedByte = 0;
    char responseBuff[1024];
    struct sockaddr_in serv_addr; 




    for(i=0;i<query_count;i++)
        free(query_arr[i]);
    free(query_arr);    
    return 0;

}

void line_count(char* filename){

    FILE* input = fopen(filename,"r");
    char line[500];
    int count=0;
    while(fgets(line,sizeof(line),input)!=NULL){
        char* temp=strtok_r(line," ",&line);
        if(strcmp(temp,id)==0)
            count++;

        memset(line,'0',strlen(line));    
    }

    query_count = count;
    fclose(input);
}


void read_queries_from_file(char* filename){
    FILE* input = fopen(filename,"r");
    char* temp="";
    char line[500];
    int count=0;

    while(fgets(line,sizeof(line),input)!=NULL){
        char* temp=strtok_r(line," ",&line);
        if(strcmp(temp,id)==0){
            strcpy(query_arr[count],line);
            count++;
        }
            

        memset(line,'0',strlen(line));    
    }

    fclose(input);
}