/**
 * @file clientX.c
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

void signal_handler(int signo);

/*HELPER FUNCTIONS */
int dim_size(char*filename);
void fill_matrix(char* filename);
void print_ts(char*msg);
void print_usage();

/* Varibles*/
int** matrix;       /* Matrix that will be sent*/
int size;           /* Size of matrix*/

char* id;           /* id of client */
int id_int;         /* id of client */
char* filePath;     /* data file */
int port;           /* port to connect */
int response;       /* 0 = non-invertible 1=invertible*/            
char* ipAddr;       /* ip address to connet */ 

int sockfd = 0;
int connectfd = 0;

int main(int argc, char**argv){

    int opt;
    int i=0;
    struct sigaction sa;
    char rspMsg[513];
  

    memset(&sa,0,sizeof(sa));
    sa.sa_handler=&signal_handler;

    sigaction(SIGINT,&sa,NULL);

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
            ipAddr = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 'o':
            filePath = optarg;
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

    if (id_int < 1)
    {
        print_ts("ID must be greater than 1\n");
        print_ts("Terminating\n");
        exit(-1);
    }


    /* initialize matrix */ 
    size = dim_size(filePath);    
    matrix = (int**) calloc(size,sizeof(int*));
    for(i=0;i<size;i++)
        matrix[i] = (int*) calloc(size,sizeof(int));

    fill_matrix(filePath);


    /* connection part */
   
    int readedByte = 0;
    char responseBuff[1024];
    struct sockaddr_in serv_addr;

    memset(responseBuff, '0', sizeof(responseBuff));
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        print_ts("\n Socket Createion failed \n");
        print_ts("\n Terminating... \n");
        exit(-1);
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(ipAddr);
    if (inet_pton(AF_INET, ipAddr, &serv_addr.sin_addr) <= 0)
    {
        print_ts("\n inet_pton error occured\n");
        print_ts("\n Terminating... \n");
        exit(-1);
    }

    char msg1[1024];
    sprintf(msg1, "Client #%d (%s:%d, %s) is submitting %dx%d matrix\n", id_int, ipAddr, port,filePath,size,size);
    print_ts(msg1);

    // connect the client socket to server socket
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) != 0) {
        print_ts("connection with the server failed...\n");
        print_ts("\n Terminating... \n");
        exit(-1);
    }

    /*time initialize */
    struct timeval  tm1, tm2;
    gettimeofday(&tm1, NULL);

    /* waiting queue status*/



   
    /* send id*/
    if( send(sockfd, &id_int , sizeof(id_int),0) < 0)
    {
        print_ts("ID Send failed\n");
        print_ts("\n Terminating... \n");
        exit(-1);
    }

    /* send size*/
    if( send(sockfd, &size , sizeof(size),0) < 0)
    {
        print_ts("ID Send failed\n");
        print_ts("\n Terminating... \n");
        exit(-1);
    }

  
    /* send matrix*/

    for(int i=0;i<size;i++){
        for(int j=0;j<size;j++){
            int temp = matrix[i][j];
            if( send(sockfd,&temp, sizeof(temp),0) < 0)
            {
                print_ts(" Matrix Send failed\n");
                print_ts("\n Terminating... \n");
                exit(-1);
            }

        }
    }

   

    /* invertible */

    if((readedByte = recv(sockfd,&response,sizeof(response),0)) < 0){
        print_ts("Recieve Failed\nTerminating...\n");
        exit(-1);
    }

    gettimeofday(&tm2,NULL);
    double totaltime = (double) (tm2.tv_usec - tm1.tv_usec) / 1000000 +(double) (tm2.tv_sec - tm1.tv_sec);
    
    if(response==0){
        sprintf(rspMsg,"Client #%d : the matrix is non-invertible, total time %.1f seconds, exiting...\n",id_int,totaltime);
    }else{
        sprintf(rspMsg,"Client #%d : the matrix is invertible, total time %.1f seconds, exiting...\n",id_int,totaltime);
    }

    print_ts(rspMsg);

    /* Free matrix */
    for(i=0;i<size;i++){
        free(matrix[i]);
    }
    free(matrix);

    close(sockfd);
    close(connectfd);
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
    print_ts("./clientX -i ID -a 127.0.0.1 -p PORT -o pathToDataFile\n");
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

void fill_matrix(char* filename){

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

    for(i=0;i<size;i++){
        for(j=0;j<size;j++){
            fscanf(input,"%d%c",&num,&temp);
            matrix[i][j]=num;
        }
    }


    fclose(input);
}

void signal_handler(int signo){
    int i=0;
    char msg1[256];
    switch (signo)
    {
        case SIGINT:            
            sprintf(msg1,"ClientX #%d caught SIGINT\nTerminating...",id_int);
            print_ts(msg1);
            close(sockfd);
            close(connectfd);
            /* Free matrix */
             for(i=0;i<size;i++){
                free(matrix[i]);
            }
            free(matrix);
            break;
    
        default:
            break;
    }
}
