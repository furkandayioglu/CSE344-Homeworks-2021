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
  
// Function designed for chat between client and server.
void func(int sockfd)
{
    int matrix_size;
    int temp=0;
    int client_id;
    int ** matrix;
    int read_size=0;
   
    read(sockfd,&client_id,sizeof(client_id));
    printf("Client ID  : %d\n",client_id);

    read(sockfd,&matrix_size,sizeof(matrix_size));
    printf("Client Matrix Size : %d\n",matrix_size);

    matrix = (int**) calloc(matrix_size,sizeof(int*));
    for(int i=0;i<matrix_size;i++){
        matrix[i]=(int*)calloc(matrix_size,sizeof(int));

    }

    for(int i=0;i<matrix_size;i++){        
        for(int j=0;j<matrix_size;j++){
            read(sockfd,&temp,sizeof(temp));
            matrix[i][j]=temp;
            printf("%d ",matrix[i][j]);
        }
            
        printf("\n");    
    }
     

    for(int i=0;i<matrix_size;i++){
        free(matrix[i]);
    }
    free(matrix);
 }
  
// Driver function
int main()
{
    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;
  
    // socket create and verification
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
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
  
    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully binded..\n");
  
    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    }
    else
        printf("Server listening..\n");
    len = sizeof(cli);
  
    // Accept the data packet from client and verification
    connfd = accept(sockfd, (SA*)&cli, &len);
    if (connfd < 0) {
        printf("server acccept failed...\n");
        exit(0);
    }
    else
        printf("server acccept the client...\n");
  
    // Function for chatting between client and server
    func(connfd);
  
    // After chatting close the socket
    close(sockfd);
}