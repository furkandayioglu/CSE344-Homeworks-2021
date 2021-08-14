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
    pthread_cond_t cond;
    
}threadPoolY_t;

typedef struct threadPoolZ_t{
    int id;
    int status;
    int full;
    int socketfd;
    //int zsocketfd;
    int connectionPort;
    pthread_mutex_t mutex;
    pthread_cond_t cond;

}threadPoolZ_t;


node_t *head;
node_t* tail;
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

void enqueue(int client_socket);
int dequeue();




/* Helpers */ 
void print_ts(char *msg,int fd);
void print_usage();
void threadParams_init();



/* Variables */

pthread_t* pool1;
pthread_t* pool2;
pthread_mutex_t main_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pool1_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pool2_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_var = PTHREAD_COND_INITIALIZER;

threadPoolY_t* threadParamsY;
threadPoolZ_t* threadParamsZ;

/* counter  */
atomic_int pool1_full=0;
atomic_int pool2_full=0;
atomic_int invertible_counter=0;
atomic_int non_invertible_counter=0;
atomic_int total_rec=0;
static volatile sig_atomic_t sig_int_flag=0;

char* ipAddr;
char* logFile;
int pool1_size;
int pool2_size;
int port;
int sleep_dur;

int socketfd;
int logFD;
pid_t temp_pid;
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


    char port_a[10];
    char time_a[10];
    char pool_a[10];
    sprintf(port_a,"%d",port+1);
    sprintf(time_a,"%d",sleep_dur);
    sprintf(pool_a,"%d",pool2_size);

    char * args[10] = {"./serverZ","-p",port_a,"-t",time_a,"-m",pool_a,"-o",logFile,NULL};
    int pid;
    /* Daemonize*/

    //becomedeamon();    

    logFD = open(logFile,O_CREAT|O_WRONLY|O_TRUNC,S_IRUSR|S_IWUSR);

    /*Start Server Z */
    char msg_srv_init[513];
    sprintf(msg_srv_init,"Server Y (127.0.0.1:%d, %s, l=%d t=%d, m=%d) started\nInstantiating Server Z\n",port,logFile,pool1_size,sleep_dur,pool2_size);
    print_ts(msg_srv_init,logFD);

    pid = fork();
    if(pid == 0){
        serverZ_pid = getpid();
        execvp("./serverZ",args);
        
    }else if(pid<0){
        /* fork Failed */ 
        print_ts("Fork for serverZ failed\nTerminating....\n",logFD);
        exit(-1);
    }

    
   
    /* ClientX connection Sockets*/
    struct sockaddr_in serv_addr;
    memset(&serv_addr,'0',sizeof(serv_addr));
    if((socketfd = socket(AF_INET, SOCK_STREAM, 0))<0){		// socket initialize
	    print_ts("socket error!\n",logFD);
        print_ts("Terminating...\n",logFD);
	    exit(-1);
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(port);

    if((bind(socketfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0){	// bind socket
	    print_ts("bind error!\n",logFD);
        print_ts("Terminating...\n",logFD);	
	    exit(-1);
	}

    if((listen(socketfd, SERVERBACKLOG)) < 0){			// listen socket
	    print_ts("listen error!\n",logFD);
        print_ts("Terminating...\n",logFD);
	    exit(-1);
	}

    /* Create Threads and initiliaze Thread Pool */  
   threadParamsY = (threadPoolY_t *)calloc(pool1_size, sizeof(threadPoolY_t));
   threadParamsZ = (threadPoolZ_t *)calloc(pool2_size, sizeof(threadPoolZ_t));

   threadParams_init();

    pool1 = (pthread_t *)calloc(pool1_size, sizeof(pthread_t));    
    for(int i=0;i<pool1_size;i++){
        if (pthread_create(&pool1[i], NULL, pool1_func, &threadParamsY[i]) != 0)
            {
                print_ts("Thread Creation failed\n",logFD);
                exit(-1);
            }
    }

    pool2 = (pthread_t *)calloc(pool2_size,sizeof(pthread_t));    
    for(int i=0;i<pool2_size;i++){
        if (pthread_create(&pool2[i], NULL, pool2_func, &threadParamsZ[i]) != 0)
            {
                print_ts("Thread Creation failed\n",2);
                exit(-1);
            }
    }

    




    /* Main thread */


    while(1){

        socklen_t socket_len = sizeof(serv_addr);
        int acceptfd = accept(socketfd,(struct sockaddr*)&serv_addr,&socket_len);

        

        phtread_mutex_lock(&main_mutex);
        enqueue(acceptfd);


       if(pool1_full < pool1_size){
           /* signal pool1*/

       }else{
           /* signal pool2*/
       }

        pthread_mutex_unlock(&main_mutex);
        


        

        if(sig_int_flag==1)
            break;
            
    }







    /* clean the mess */
    free(pool1);
    free(pool2);
    free(threadParamsY);
    free(threadParamsZ);
    free(tail);
    free(head);
    close(logFD);
    close(socketfd);



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

void enqueue(int client_socket){
    node_t *newnode = (node_t*) calloc(1,sizeof(node_t));
    
    
    newnode->client_socket = client_socket;
    newnode->next = NULL;

    if(tail == NULL){
        head = newnode;
    }else{
        tail->next= newnode;
    }

    tail=newnode;

}

int dequeue(){

    if(head==NULL){
        return 0;
    }
        
    else{

        int res = head->client_socket;
        node_t* temp = head;
        head=head->next;
        if(head == NULL){
            tail=NULL;
        }
        free(temp);
        return res;
    } 

}

void threadParams_init(){
    for(int i=0;i<pool1_size;i++){
        threadParamsY[i].id=i;
        threadParamsY[i].status=0;
        threadParamsY[i].full = 0;
        threadParamsY[i].socketfd=0;
        if(pthread_mutex_init(&threadParamsY[i].mutex,NULL)!=0){								// initialize mutex and condition variables
			print_ts("Mutex initialize failed!!!. Terminating...",logFD);
			exit(-1);
		}
        if(pthread_cond_init(&threadParamsY[i].cond,NULL)!=0){								// initialize mutex and condition variables
			print_ts("Condition Variable initialize failed!!!. Terminating...",logFD);
			exit(-1);
		}
    }

    for(int i=0;i<pool2_size;i++){
        threadParamsZ[i].id=i;
        threadParamsZ[i].status=0;
        threadParamsZ[i].full = 0;
        threadParamsZ[i].socketfd=0;
        threadParamsZ[i].connectionPort = port+1;
        if(pthread_mutex_init(&threadParamsZ[i].mutex,NULL)!=0){								// initialize mutex and condition variables
			print_ts("Mutex initialize failed!!!. Terminating...",logFD);
			exit(-1);
		}
         if(pthread_cond_init(&threadParamsZ[i].cond,NULL)!=0){								// initialize mutex and condition variables
			print_ts("Condition Variable initialize failed!!!. Terminating...",logFD);
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
    char msg[513];
    switch (signo)
    {
        case SIGINT:
            sig_int_flag = 1;
            break;
    
        default:
            break;
    }
}

void* pool1_func(void* arg){
    /* pool 1 */

    
    int bytes = 0;
    threadPoolY_t threadParams = *((threadPoolY_t*)arg);
    while(1){
        
        if(sig_int_flag==1){
            break;
        }

        // Change condition varible 
        /* @TODO Change cond variable    */
        pthread_mutex_lock(&main_mutex);
        if( (threadParams.socketfd = dequeue()) == 0){
            pthread_cond_wait(&condition_var,&main_mutex);
            threadParams.socketfd = dequeue();
        }
        pool1_full++;
        pthread_mutex_unlock(&main_mutex);


        /* handle calculation */

        if(sig_int_flag == 0){

            int i=0,j=0;
            int client_id=0;
            int matrix_size=0;
            int** matrix;
            int det=0;
            int response=0;


            if(threadParams.socketfd != 0){
                if((bytes = read(threadParams.socketfd,&client_id,sizeof(client_id)))<0){
                    print_ts("Z: Recieve error Client_id\nTerminating...\n",logFD);
                }

                if((bytes = read(threadParams.socketfd,&matrix_size,sizeof(matrix_size)))<0){
                    print_ts("Z: Recieve error Matrix_size\nTerminating...\n",logFD);
                }

                matrix = (int**) calloc(matrix_size,sizeof(int*));
                for( i=0;i< matrix_size;i++){
                    matrix[i] = (int*) calloc(matrix_size,sizeof(int));
                }


                for( i=0;i<matrix_size;i++){        
                    for( j=0;j<matrix_size;j++){
                        int temp;
                        if((bytes = read(threadParams.socketfd,&temp,sizeof(temp)))<0){
                            print_ts("Z: Recieve error Matrix\nTerminating...\n",logFD);
                            exit(-1);
                        }
                        matrix[i][j]=temp;
                   
                    }
            
                }

                char clHndMsg[513];
                sprintf(clHndMsg,"Z: Thread #%d of pool1 is handling Client #%d: matrix size %dx%d, pool busy %d/%d\n",threadParams.id,client_id,matrix_size,matrix_size,pool1_full,pool1_size);
                print_ts(clHndMsg,logFD);


                det = determinant(matrix,matrix_size);

                if(det == 0){
                    response = 0;
                    ++non_invertible_counter;
                }else{
                    response = 1;
                    ++invertible_counter;
                }


                total_rec++;
                sleep(sleep_dur);    

                char clRspMsg[513];
                if(response == 0)
                    sprintf(clRspMsg,"Thread #%d of pool1 is responding Client #%d, matrix IS non-invertible\n",threadParams.id,client_id);
                else
                    sprintf(clRspMsg,"Thread #%d of pool1 is responding Cliend #%d, matrix IS invertible\n",threadParams.id,client_id);

                print_ts(clRspMsg,logFD);


                if( write(threadParams.socketfd, &response , sizeof(response)) < 0)
                {
                    print_ts("Response Send Fail\n",logFD);
                    print_ts("\n Terminating... \n",logFD);
                    exit(-1);
                }

                pthread_mutex_lock(&main_mutex);
                pool1_full--;
                pthread_mutex_unlock(&main_mutex);

                for( i=0;i<matrix_size;i++){
                    free(matrix[i]);
                }
                free(matrix);   
            }

            close(threadParams.socketfd);
            pthread_mutex_lock(&main_mutex);
            pool1_full--;
            pthread_mutex_unlock(&main_mutex);
        }


    }
    
    return NULL;
}

void* pool2_func(void* arg){

    int bytes = 0;
    threadPoolZ_t threadParams = *((threadPoolZ_t*)arg);

    while(1){

        if(sig_int_flag==1)
            break;


        // change condition varible
         pthread_mutex_lock(&main_mutex);
        if( (threadParams.socketfd = dequeue()) == 0){
            pthread_cond_wait(&condition_var,&main_mutex);
            threadParams.socketfd = dequeue();
        }
        pool2_full++;        
        pthread_mutex_unlock(&main_mutex);

        if(sig_int_flag == 0){
            int i=0,j=0;
            int client_id;
            int matrix_size;
            int** matrix;
            int det=0;
            int response=0;
             
            if( threadParams.socketfd !=0 ){
                
                if((bytes = read(threadParams.socketfd,&client_id,sizeof(client_id)))<0){
                    print_ts("Z: Recieve error Client_id\nTerminating...\n",logFD);
                }

                if((bytes = read(threadParams.socketfd,&matrix_size,sizeof(matrix_size)))<0){
                    print_ts("Z: Recieve error Matrix_size\nTerminating...\n",logFD);
                }

                matrix = (int**) calloc(matrix_size,sizeof(int*));
                for( i=0;i< matrix_size;i++){
                    matrix[i] = (int*) calloc(matrix_size,sizeof(int));
                }

                for( i=0;i<matrix_size;i++){        
                    for( j=0;j<matrix_size;j++){
                        int temp;
                        if((bytes = read(threadParams.socketfd,&temp,sizeof(temp)))<0){
                            print_ts("Z: Recieve error Matrix\nTerminating...\n",logFD);
                            exit(-1);
                        }
                        matrix[i][j]=temp;
                   
                    }

                /* create connection to z */

                struct sockaddr_in server_addr;
                int zsocketfd;

                if((zsocketfd = socket(AF_INET,SOCK_STREAM,0))<0){
                    print_ts("Server Y, Pool2 has an issue to connect to ServerZ\nTerminating...\n",logFD);
                    exit(-1);
                }
                memset(&server_addr, '0', sizeof(server_addr));
                server_addr.sin_family = AF_INET;
                server_addr.sin_port = htons(threadParams.connectionPort);
                server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

               
                if (connect(zsocketfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
                    print_ts("connection with the server failed...\n",logFD);
                    print_ts("\n Terminating... \n",logFD);
                    exit(-1);
                    }

                /* Send Server Z */
                if(write(zsocketfd, &client_id , sizeof(client_id)) < 0){
                    print_ts("ID Send failed\n",logFD);
                    print_ts("\n Terminating... \n",logFD);
                    exit(-1);
                }

                if(write(zsocketfd, &matrix_size , sizeof(matrix_size)) < 0){
                    print_ts("ID Send failed\n",logFD);
                    print_ts("\n Terminating... \n",logFD);
                    exit(-1);
                }

                for(i=0;i<matrix_size;i++){
                    for(j=0;j<matrix_size;j++){
                        int temp=matrix[i][j];
                        if(write(zsocketfd,&temp,sizeof(temp))<0){
                            print_ts("Matrix send from Y to Z failed\n",logFD);
                            print_ts("Terminating\n",logFD);
                            exit(-1);
                        }
                    }
                }
                /* get server z response */ 
                if((bytes = read(zsocketfd,&response,sizeof(response))) < 0){
                    print_ts("Recieve Failed\nTerminating...\n",logFD);
                    exit(-1);
                }

                /*return clientX response */    
                if(write(threadParams.socketfd, &response , sizeof(response)) < 0){
                    print_ts("ID Send failed\n",logFD);
                    print_ts("\n Terminating... \n",logFD);
                    exit(-1);
                }
                
                pthread_mutex_lock(&main_mutex);
                pool2_full--;
                pthread_mutex_unlock(&main_mutex);
                
                for( i=0;i<matrix_size;i++){
                    free(matrix[i]);
                }
                free(matrix);

                close(zsocketfd);
            }   

            close(threadParams.socketfd);
            pthread_mutex_lock(&main_mutex);
            pool2_full--;
            pthread_mutex_unlock(&main_mutex);

        }
       
    }


    return 0;
}