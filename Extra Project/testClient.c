#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include <arpa/inet.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include<pthread.h>

#define MAX 80
#define PORT 8080
#define SA struct sockaddr


void func(int sockfd,int** matrix)
{
    int client_id=1;
    int matrix_size=4;
    int i,j;
    int temp=0;
   for( i=0;i<matrix_size;i++){
        for( j =0 ; j<matrix_size;j++){
            printf("%d ",matrix[i][j]);
        }
        printf("\n");
    }

    printf("Client ID  : %d\n",client_id);
    if( write(sockfd, &client_id , sizeof(client_id)) < 0)
    {
            printf("ID Send failed\n");
    }

    printf("Matrix Size  : %d\n",matrix_size);
    if( write(sockfd,&matrix_size , sizeof(matrix_size)) < 0)
    {
            printf("Size Send failed\n");
    }





    for( i=0; i<4; i++){
        for( j=0; j<4; j++){
            printf("temp = %d   matrix[%d][%d]=%d\n",temp, i,j,matrix[i][j]);
            temp = matrix[i][j];
            printf("temp = %d   matrix[%d][%d] Sending \n",temp, i,j);

            if( write(sockfd,&temp, sizeof(temp)) < 0)
            {
                printf("temp = %d   matrix[%d][%d] Send failed\n",temp, i,j);
            }
        }
    }
   
}
  
int main()
{
    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;
    int matrix[4][4] = { { 1, 0, 2, -1 },
                      { 3, 0, 0, 5 },
                      { 2, 1, 4, -3 },
                      { 1, 0, 5, 0 } };
 
    // socket create and varification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));
  
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);
  
    // connect the client socket to server socket
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }
    else
        printf("connected to the server..\n");
  
    // function for chat
    //func(sockfd,mat);
    int client_id=1;
    int matrix_size=4;
    int i,j;
    int temp=0;
   for( i=0;i<matrix_size;i++){
        for( j =0 ; j<matrix_size;j++){
            printf("%d ",matrix[i][j]);
        }
        printf("\n");
    }

    printf("Client ID  : %d\n",client_id);
    if( send(sockfd, &client_id , sizeof(client_id),0) < 0)
    {
            printf("ID Send failed\n");
    }

    printf("Matrix Size  : %d\n",matrix_size);
    if( send(sockfd,&matrix_size , sizeof(matrix_size),0) < 0)
    {
            printf("Size Send failed\n");
    }





    for( i=0; i<4; i++){
        for( j=0; j<4; j++){
            printf("temp = %d   matrix[%d][%d]=%d\n",temp, i,j,matrix[i][j]);
            temp = matrix[i][j];
            printf("temp = %d   matrix[%d][%d] Sending \n",temp, i,j);

            if( send(sockfd,&temp, sizeof(temp),0) < 0)
            {
                printf("temp = %d   matrix[%d][%d] Send failed\n",temp, i,j);
            }
        }
    }
    // close the socket
    close(sockfd);
}