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
    write(2, buffer, strlen(buffer));
}
void print_usage()
{
    print_ts("##USAGE##\n");
    print_ts("Invalid Amount of parameter\n");
    print_ts("./client -i ID -a 127.0.0.1 -p PORT -o pathToQueryFile\n");
}

char *id;
int id_int;
char *ipAdrr;
int port;
char *pathToQuery;
int query_count;
char **query_arr;

void line_count(char *filename);
void read_queries_from_file(char *filename);

int main(int argc, char **argv)
{

    int opt;
    int i = 0, j = 0;

    /*fprintf(stderr,"%s\n",argv[0]);
    fprintf(stderr,"%s\n",argv[1]);
    fprintf(stderr,"%s\n",argv[2]);
    fprintf(stderr,"%s\n",argv[3]);
    fprintf(stderr,"%s\n",argv[4]);
    fprintf(stderr,"%s\n",argv[5]);
    fprintf(stderr,"%s\n",argv[6]);
    fprintf(stderr,"%s\n",argv[7]);
    fprintf(stderr,"%s\n",argv[8]);
    fprintf(stderr,"%d\n",argc);*/

    if (argc != 9)
    {
        print_usage();
        print_ts("Terminating...\n");
        exit(-1);
    }

    while ((opt = getopt(argc, argv, ":i:a:p:o:")) != -1)
    {
        switch (opt)
        {
        case 'i':
            id = optarg;
            id_int = atoi(optarg);
            break;
        case 'a':
            ipAdrr = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 'o':
            pathToQuery = optarg;
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
        print_ts("Terminating...\n");
        exit(-1);
    }

    if (id_int < 1)
    {
        print_ts("ID must be greater than 1\n");
        print_ts("Terminating\n");
        exit(-1);
    }

    line_count(pathToQuery);

    query_arr = (char **)calloc(query_count, sizeof(char *));
    for (i = 0; i < query_count; i++)
        query_arr[i] = (char *)calloc(500, sizeof(char));

    read_queries_from_file(pathToQuery);

    int sockfd = 0;
    int readedByte = 0;
    char responseBuff[1024];
    struct sockaddr_in serv_addr;

    memset(responseBuff, '0', sizeof(responseBuff));
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        print_ts("\n Socker Createion failed \n");
        print_ts("\n Terminating... \n");
        exit(-1);
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(ipAdrr);
    if (inet_pton(AF_INET, ipAdrr, &serv_addr.sin_addr) <= 0)
    {
        print_ts("\n inet_pton error occured\n");
        exit(-1);
    }

    char msg1[500];
    sprintf(msg1, "Client (%d) connecting to %s:%d\n", id_int, ipAdrr, port);
    print_ts(msg1);

    int connectfd = 0;

    if ((connectfd = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
    {
        print_ts("\n Connection failed \n");
        exit(-1);
    }

    for (j = 0; j < query_count; j++)
    {
        struct timeval tm1, tm2;
        int line_c = 0, k = 0;
        gettimeofday(&tm1, NULL);

        printf("Client (%d) connected and requesting query  %s \n", id_int, query_arr[i]);

        write(sockfd, query_arr[i], strlen(query_arr[i]));

        readedByte = read(sockfd, responseBuff, sizeof(responseBuff) - 1);
        responseBuff[readedByte] = '\0';

        if (readedByte < 0)
        {
            print_ts("\n Read error! Program finished. \n");
            exit(-1);
        }

        gettimeofday(&tm2, NULL);
        double totalTime = (double)(tm2.tv_usec - tm1.tv_usec) / 1000000 + (double)(tm2.tv_sec - tm1.tv_sec);
        char msg2[1500];
        sprintf(msg2, "Server’s response to (%d): %s, arrived in %.1fseconds. \n\n", id_int, responseBuff, totalTime);
        line_c = atoi(responseBuff);

        for (k = 0; k < line_c; k++)
        {
            memset(responseBuff, 0, strlen(responseBuff));
            readedByte = read(sockfd, responseBuff, sizeof(responseBuff) - 1);
            responseBuff[readedByte] = '\0';

            print_ts(responseBuff);
        }
    }

    close(connectfd);

    for (i = 0; i < query_count; i++)
        free(query_arr[i]);
    free(query_arr);
    return 0;
}

void line_count(char *filename)
{

    FILE *input = fopen(filename, "r");
    char line[500]="";
    int count = 0;

    while (fgets(line, sizeof(line), input) != NULL)
    {       
        char *temp = (char*)calloc(500,sizeof(char));
        char* temp_t;

        strcpy(temp,line);         
        temp_t=strtok_r(temp," ",&temp);

        
        fprintf(stderr,"%s\n",temp_t);
        fprintf(stderr,"%s\n",temp);

        if (strcmp(temp_t,id)==0)
            count++;

        memset(line, 0, strlen(line));
        free(temp);
    }

    query_count = count;

    
    fclose(input);
}

void read_queries_from_file(char *filename)
{

    FILE *input = fopen(filename, "r");

    char line [500];
    int count = 0;

    while (fgets(line, sizeof(line), input) != NULL)
    {    
        char *temp = (char*)calloc(500,sizeof(char));
        char* temp_t;

        strcpy(temp,line);         
        temp_t=strtok_r(temp," ",&temp);

        
        fprintf(stderr,"%s\n",temp_t);
        fprintf(stderr,"%s\n",temp);

        if (strcmp(temp_t,id)==0){

            strcpy(query_arr[count],temp);
            count++;

        }

        memset(line, 0, strlen(line));
        free(temp);      
       
    }

  

    fclose(input);
}