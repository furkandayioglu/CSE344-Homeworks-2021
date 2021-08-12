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
#include <stdatomic.h>

/*  Data Structures */

typedef struct threadPool_t{
    int id;
    int full;
    int status;
    int socketfd;
    pthread_mutex_t mutex;
}threadPool_t;


/* Thread Function */
void *pool_func(void* arg);


/* Functions */

/* These functions are taken from geeksforgeeks 
   https://www.geeksforgeeks.org/determinant-of-a-matrix/ */

void cofactors(int** matrix, int** temp, int size, int i, int j);
int determinant(int** matrix,int size);
void signal_handler(int signo);


/* Helpers */ 
void print_ts(char* msg);
void print_usage();
void threadPool_init();
void check_instance();

/* Variables */

pthread_t* pool;
threadPool_t* threadParams; /* Threads parameters */ 
pthread_mutex_t main_mutex = PTHREAD_MUTEX_INITIALIZER; /* In order to create critical region to write into file*/
pthread_mutex_t full_mutex = PTHREAD_MUTEX_INITIALIZER; /* find available thread */ 

/* condition variables */

char* logFile;
int pool_size;
int port;
int sleep_dur;

static volatile sig_atomic_t sig_int_flag=0;
/* counter  */
atomic_int pool_full=0;
atomic_int total_rec=0;
atomic_int invertible_counter=0;
atomic_int non_invertible_counter=0;

int logFD;
int socketfd;

int main(int argc, char** argv){

    int opt;
    int i=0;
    struct sigaction sa;
    
    memset(&sa,0,sizeof(sa));
    sa.sa_handler=&signal_handler;

    sigaction(SIGINT,&sa,NULL);


    check_instance();
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


    /* Deamonize */



   
    /* server creation */ 
    struct sockaddr_in serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));
    if((socketfd = socket(AF_INET, SOCK_STREAM, 0))<0){		// socket initialize
	    print_ts("socket error!\n");
        print_ts("Terminating...\n");
	    exit(-1);
    }
    serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serv_addr.sin_port = htons(port); 

	if((bind(socketfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0){	// bind socket
	    print_ts("bind error!\n");
        print_ts("Terminating...\n");	
	    exit(-1);
	}

    if((listen(socketfd, 20)) < 0){			// listen socket
	    print_ts("listen error!\n");
        print_ts("Terminating...\n");
	    exit(-1);
	}

    char msg_srv_init[513];
    sprintf(msg_srv_init,"Z:Server Z (127.0.0.1:%d, %s, t=%d, m=%d) started\n",port,logFile,sleep_dur,pool_size);
    print_ts(msg_srv_init);




    /* Create threads */
    pool = (pthread_t *)calloc(pool_size, sizeof(pthread_t));
    threadParams = (threadPool_t *)calloc(pool_size, sizeof(threadPool_t));
    for(i=0;i<pool_size;i++){
        if (pthread_create(&pool[i], NULL, pool_func, &i) != 0)
            {
                print_ts("Thread Creation failed\n");
                exit(-1);
            }
    }
    if(pthread_mutex_init(&main_mutex,NULL)!=0){								// initialize mutex and condition variables
		print_ts("main mutex initialize failed!!!. Program will finish.");
		exit(-1);
	}
    if(pthread_mutex_init(&full_mutex,NULL)!=0){								// initialize mutex and condition variables
		print_ts("full mutex initialize failed!!!. Program will finish.");
		exit(-1);
	}
    threadPool_init();

    /* Main Thread */
    while (1){

        socklen_t socket_len = sizeof(serv_addr);
        int acceptfd = accept(socketfd,(struct sockaddr*)&serv_addr,&socket_len);
    
        pthread_mutex_lock(&main_mutex);
        
        for(int i=0;i<pool_size;i++){
            if(threadParams[i].status == 0){
                threadParams[i].status = 1;
                threadParams[i].full = 1;
                threadParams[i].socketfd = acceptfd;
                
            }
        }
        pthread_mutex_unlock(&main_mutex);

       if(sig_int_flag==1){
           break;
       }
    }


    /* Clean the mess */

    for(i=0; i<pool_size;i++){
        void * ptr = NULL;
        int stat;
		stat=pthread_join(pool[i],&ptr); 						
		if(stat!=0) 
			print_ts(" Error p_thread Join");
    }

    close(socketfd);
    close(logFD);
    free(threadParams);

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
    print_ts("./serverZ -p PORT -o PathLogFile -m poolsize -t sleepduration\n");
}

void check_instance(){
    int pid_file = open("/tmp/serverZ.pid", O_CREAT | O_RDWR,0666);
    int res = flock(pid_file, LOCK_EX | LOCK_NB);

    if(res){
        if(EAGAIN == errno){
            print_ts("Z: Server Z already running...\n");
            print_ts("Z: Terminating...\n");
            exit(-1);
        }
    }
}

void threadPool_init(){
    int i=0;

    for(i=0;i<pool_size;i++){
        threadParams[i].id=i;
        threadParams[i].status=0;
        threadParams[i].full = 0;
        threadParams[i].socketfd=0;
        if(pthread_mutex_init(&threadParams[i].mutex,NULL)!=0){								// initialize mutex and condition variables
			print_ts("mutex initialize failed!!!. Program will finish.");
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
    char msg1[513];
    int j;
    switch(signo){
        case SIGINT:
            sig_int_flag =1;
            sprintf(msg1,"Z: SIGINT recieved, exiting serverZ. Total request:%d, invertible: %d, %d not\n",total_rec,invertible_counter,non_invertible_counter);
            print_ts(msg1);

            for(j=0; j< pool_size;j++){
                pthread_join(pool[j],NULL);
            }


            free(pool);
            free(threadParams);
            close(logFD);
            close(socketfd);
            break;
        default:
            break;
    }


}

void *pool_func(void* arg){
  
    int id = *((int *)arg);
    int bytes=0; 
    char msg[100];
	sprintf(msg,"Z: Thread #%d: waiting for connection\n",id);
   
    while(1){

        print_ts(msg);
        if(sig_int_flag == 1){
            break;
        }

        pthread_mutex_lock(&threadParams[id].mutex);
        /* change thread status*/
        if(threadParams[id].full == 0){

            while(threadParams[id].full == 0){}

            threadParams[id].status=1;
            
        }
            

        if(sig_int_flag==0){

            int i=0,j=0;
            int client_id;
            int matrix_size;
            int** matrix;
            int det=0;
            int response=0;

            if((bytes = recv(threadParams[id].socketfd,&client_id,sizeof(client_id),0))<0){
                print_ts("Z: Recieve error Client_id\nTerminating...\n");
            }

            if((bytes = recv(threadParams[id].socketfd,&matrix_size,sizeof(matrix_size),0))<0){
                print_ts("Z: Recieve error Matrix_size\nTerminating...\n");
            }

            matrix = (int**) calloc(matrix_size,sizeof(int*));
            for( i=0;i< matrix_size;i++){
                matrix[i] = (int*) calloc(matrix_size,sizeof(int));
            }



            for( i=0;i<matrix_size;i++){        
                for( j=0;j<matrix_size;j++){
                    int temp;
                    if((bytes = recv(threadParams[id].socketfd,&temp,sizeof(temp),0))<0){
                        print_ts("Z: Recieve error Matrix\nTerminating...\n");
                        exit(-1);
                    }
                    matrix[i][j]=temp;
                   
                }
            
            }

            char clHndMsg[513];
            sprintf(clHndMsg,"Z: Thread #%d is handling Client #%d: matrix size %dx%d, pool busy %d/%d\n",id,client_id,matrix_size,matrix_size,pool_full,pool_size);
            print_ts(clHndMsg);

            det = determinant(matrix,matrix_size);

            if(det == 0){
                response = 0;
                non_invertible_counter++;
            }else{
                response=1;
                invertible_counter++;
            }
            total_rec++;
            sleep(sleep_dur);

            char clRspMsg[513];
            if(response == 0)
                sprintf(clRspMsg,"Z: Thread #%d is responding Client #%d: matrix IS non-invertible\n",id,client_id);
            else
                sprintf(clRspMsg,"Z: Thread #%d is responding Client #%d: matrix IS invertible \n",id,client_id);
            
            print_ts(clRspMsg);

            if( send(threadParams[id].socketfd, &response , sizeof(response),0) < 0)
            {
                print_ts("Z: Response Send Fail\n");
                print_ts("\n Terminating... \n");
                exit(-1);
            }

            for( i=0;i<matrix_size;i++){
                free(matrix[i]);
            }
            free(matrix);

            close(threadParams[id].socketfd);

            pthread_mutex_lock(&main_mutex);
            threadParams[id].full=0;
            threadParams[id].status=0;
            pool_full--;
            pthread_mutex_unlock(&main_mutex);

            pthread_mutex_unlock(&threadParams[id].mutex);
        }

    }

    return 0;
}