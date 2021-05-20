#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdbool.h>

/* student budget*/
int budget;

atomic_int low_budget_flag=0;

/* syncronizer for producer consumer btw thread_h and main_thread */ 
sem_t hw_empty;
sem_t hw_full;
sem_t hw_mutex;

/* define critical region btw worker_threads and main_thread*/
/*check and modify availablity, assigned */
sem_t *student_hire_mutex;



/* homework queue to write and read from */
/* buffer of the producer consumer btw main and h */
typedef struct queue_s{

    char* array;
    atomic_int front;
    atomic_int rear;
    atomic_int size;
    atomic_int capacity;

}queue_s;

static struct queue_s* hw_queue;

/* define each students*/
/*student threads will read the index of themselves from student array*/
/* it will define sleeping time and cost*/
/* According to student array there will be created index arrays to find the fastest/cheapest/ most qualified one*/
/* ex: HW : S
    check speed_index array, find avalible one. ex index speed[0]=3 ->  student[3]. 
    Modify assigned bit of student[3] and let it sleep for 6-S time
 */ 

typedef struct student_s{
    char name[20];
    atomic_int cost;
    atomic_int quality;
    atomic_int speed;
    atomic_int avaliable;   /* 0 available, 1 busy, thread make it 1 before sleep, then make it 0 */
    atomic_int assigned;    /* main thread makes it 1 if it assign any job for thread */
    atomic_int alive;       /* thread loop checker, main thread makes it -1 if program has to stop */
    atomic_int hw_solved;
    atomic_int money_made;

}student_s;

struct student_s *students;  

/* threads */
void* thread_h_work(void* params);
void* worker_thread(void* params);


/* semaphore operations with error checks */

void semwait(sem_t* sem){

    if(sem_wait(sem)<0){
        fprintf(stderr,"Sem Wait Failed.\n");
        exit(-1);
    }
}

void sempost(sem_t* sem){

    if(sem_post(sem)<0){
        fprintf(stderr,"Sem Post Failed.\n");
        exit(-1);
    }
}

/* essential functions to prepare enviroment */
int student_count(char* filepath);
void fill_students(struct student_s * s, int student_count, char*filename);
void sort_speed(struct student_s *s, int*,int);
void sort_quality(struct student_s *s, int*,int);
void sort_cost(struct student_s *s, int*,int);
void print_usage();


/* queue functions */

void init_queue(int capacity);
char poll();
void offer(char hw);

void signal_handler(int signo);


int student_cnt;
char *hwfile, *student_for_hire;
pthread_t thrd_h;
pthread_t *worker_threads;

int main(int argc, char**argv){

    
  
    int *speed_index;
    int *quality_index;
    int *cost_index;
    int i=0;
    int s;

    int hw_fd;
    int main_avaliale_thread_flag = 0;

    struct sigaction sa;
  

    memset(&sa,0,sizeof(sa));
    sa.sa_handler=&signal_handler;
    sigaction(SIGINT,&sa,NULL);


    if(argc<4){
        print_usage();
        exit(-1);
    }


    hwfile = argv[1];
    student_for_hire = argv[2];
    budget= atoi(argv[3]);
    student_cnt=student_count(student_for_hire);

    students = (struct student_s*) calloc (student_cnt,sizeof(struct student_s));
    hw_queue = (struct queue_s*) calloc(1,sizeof(struct queue_s));    
    student_hire_mutex = (sem_t*) calloc(student_cnt, sizeof(sem_t));
    worker_threads = (pthread_t*) calloc(student_cnt,sizeof(pthread_t));

    speed_index = (int*) calloc(student_cnt,sizeof(int));
    quality_index = (int*) calloc(student_cnt,sizeof(int));
    cost_index = (int*) calloc(student_cnt,sizeof(int));

    fill_students(students,student_cnt,student_for_hire);
    init_queue(student_cnt+1);

    sort_speed(students,speed_index,student_cnt);
    sort_quality(students,quality_index,student_cnt);
    sort_cost(students,cost_index,student_cnt);


    hw_fd = open(hwfile,O_RDONLY);
    if(hw_fd <0){
        fprintf(stderr,"HW_file could not be opened\n");
        exit(-1);
    }

    /*Initialize semaphores */

     if(sem_init(&hw_full,1,0)<0){
        fprintf(stderr,"hw_full sem could not be initialized\n");
        exit(-1);
    }

    if(sem_init(&hw_empty,1,student_cnt+1)<0){
        fprintf(stderr,"hw_empty sem could not be initialized\n");
        exit(-1);
    }

    if(sem_init(&hw_mutex,1,1)<0){
        fprintf(stderr,"hw_mutex could not be initialized\n");
        exit(-1);
    }


    for(i=0;i<student_cnt;i++){
        if(sem_init(&student_hire_mutex[i],1,1)<0){
            fprintf(stderr,"student_hire_mutex[%d] could not be initialized\n",i);
            exit(-1);
        }
    }


    /* Create Threads */
    fprintf(stderr,"%10s    %2c    %3c    %3c\n","Name", 'Q','S','C');
    for(i = 0;i<student_cnt;i++){
        
        fprintf(stderr,"%10s   %3d    %3d    %3d  \n",students[i].name,students[i].quality,students[i].speed,students[i].cost);
        s=pthread_create(&worker_threads[i],NULL,worker_thread,&i);
        if(s != 0){
            fprintf(stderr,"Student For hire Thread creation failed\n");
            exit(-1);
        }
    }


    s=pthread_create(&thrd_h,NULL,thread_h_work,&hw_fd);
    if(s != 0){
        fprintf(stderr,"H Thread creation failed\n");
        exit(-1);
    }


   
    /* main thread Loop*/


    while(true){
        char c;
        int any_assinged=-1;
        semwait(&hw_full);
        semwait(&hw_mutex);
        c=poll();
        sempost(&hw_mutex);
        sempost(&hw_empty);

        if(c=='Q'){
            int qflag=0;
            int index=0;
            while(qflag==0){
                semwait(&student_hire_mutex[index]);
                if(students[quality_index[index]].avaliable == 0 && budget > students[quality_index[index]].cost){
                    students[quality_index[index]].assigned==1;
                    budget -= students[quality_index[index]].cost;
                    qflag++;
                    any_assinged=1;
                }
                sempost(&student_hire_mutex[index]);
                index++;
            }
        }
        if(c=='S'){
            int sflag=0;
            int index=0;
            while(sflag==0){
                semwait(&student_hire_mutex[index]);
                if(students[speed_index[index]].avaliable == 0 && budget > students[speed_index[index]].cost){
                    students[speed_index[index]].assigned==1;
                    budget -= students[speed_index[index]].cost;
                    sflag++;
                    any_assinged=1;
                }
                sempost(&student_hire_mutex[index]);
                index++;
            }
        }
        if(c=='C'){
             int cflag=0;
            int index=0;
            while(cflag==0){
                semwait(&student_hire_mutex[index]);
                if(students[cost_index[index]].avaliable == 0 && budget > students[cost_index[index]].cost){
                    students[cost_index[index]].assigned==1;
                    budget -= students[cost_index[index]].cost;
                    cflag++;
                    any_assinged=1;
                }
                sempost(&student_hire_mutex[index]);
                index++;
            }
        }

        if(any_assinged==-1){
            low_budget_flag=-1;
            break;
        }

    }


    for(i=0;i<student_cnt;i++)
        students[i].alive=-1;



    /* join threads */

    s=pthread_join(thrd_h, NULL);
    if(s!=0){
        fprintf(stderr,"H thread join failed\n");
        exit(-1);
    }

    for(i = 0 ;i<student_cnt;i++){
       s=pthread_join(worker_threads[i],NULL);
       if(s!=0){
            fprintf(stderr,"Student for hire thread join failed\n");
            exit(-1);
        }
    }

    /* destroy semaphores */

    if(sem_destroy(&hw_full)<0){
        fprintf(stderr,"hw_full sem could not be destroyed\n");
        exit(-1);
    }

    if(sem_destroy(&hw_empty)<0){
        fprintf(stderr,"hw_empty sem could not be destroyed\n");
        exit(-1);
    }

    if(sem_destroy(&hw_mutex)<0){
        fprintf(stderr,"hw_mutex could not be destroyed\n");
        exit(-1);
    }


    for(i=0;i<student_cnt;i++){
        if(sem_destroy(&student_hire_mutex[i])<0){
            fprintf(stderr,"student_hire_mutex[%d] could not be destroyed\n",i);
            exit(-1);
        }
    }


    free(speed_index);
    free(cost_index);
    free(quality_index);
    free(students);
    free(student_hire_mutex);
    free(worker_threads);
    free(hw_queue->array);
    free(hw_queue);
    close(hw_fd);
    return 0;
}

void print_usage(){

    fprintf(stderr,"##USAGE##\n");
    fprintf(stderr,"Program must have 4 parameters\n");
    fprintf(stderr,"./program homeworkfilepath studentsfilepath budget \n");
}

int student_count(char *filepath){

    int fd = open(filepath, O_RDONLY);
    int count = 0 ;
    int size=0;
    char c;
    char p=' ';
    if(fd < 0){
        fprintf(stderr,"Student Count : Input file could not be opened\n");
        exit(-1);
    }

    size = read(fd,&c,1);
    if(size<0){
        fprintf(stderr,"StudentCount: File read error\n");
        perror("Err");
        exit(-1);
    }
    while(size>0){
     
        if(c == '\n'){
            count++;
            
        }
        p=c;
       
        size = read(fd,&c,1);
        if(size == 0 & p!= '\n')
            count++;
        if(size<0){
            fprintf(stderr,"StudentCount: File read error\n");
            perror("Err");
            exit(-1);
        }
    }

    close(fd);
    return count;
}

void fill_students(struct student_s *s , int student_count ,  char* filename){

    
    int i=0;
    FILE* inputfile;
  

    inputfile = fopen(filename,"r");
    if(inputfile == NULL){
        fprintf(stderr,"File could not be opened\n");
        exit(-1);
    }

    for(i=0;i<student_count;i++){
        char name[20];
        atomic_int quality;
        atomic_int speed;
        atomic_int cost;

        fscanf(inputfile,"%s %d %d %d ",name,&quality,&speed,&cost);

        strcpy(s[i].name,name);
        s[i].quality=quality;
        s[i].speed=speed;
        s[i].cost = cost;
        s[i].avaliable = 0;
        s[i].alive=1;
        s[i].assigned=0;
        s[i].hw_solved=0;
        s[i].money_made=0;
    }

    fclose(inputfile);
}

void init_queue(int capacity){
    hw_queue->capacity = capacity;
    hw_queue->front = 0;
    hw_queue->size = 0;
    hw_queue->rear = hw_queue->capacity;


    hw_queue->array = (char*) calloc(capacity,sizeof(char));

}

char poll(){
    
    char hw;

    if(hw_queue->size==0){
        return -1;
    }

    hw = hw_queue->array[hw_queue->front];

    hw_queue->front = (hw_queue->front +1)%hw_queue->capacity;
    hw_queue->size--;

    return hw;

}

void offer(char hw){
    
    if(hw_queue->size==hw_queue->capacity){
        fprintf(stderr,"Hw queue is full\n");
        return ;
    }

    hw_queue->rear = (hw_queue->rear+1)%hw_queue->capacity;
    hw_queue->array[hw_queue->rear]=hw;
    hw_queue->size++;
}

void sort_speed(struct student_s *s, int*arr,int count){

    int i=0;
    int j=0;
    
    fprintf(stderr,"%d count\n",count);

    for(i=0 ; i< count;i++)
        arr[i]=i;



    for(i=0 ; i<count-1;i++){        
        for(j=0; j<count-i-1;j++){           
            if(s[arr[j]].speed<s[arr[j+1]].speed){
                int temp = arr[j];
                arr[j] = arr[j+1];
                arr[j+1] = temp;
            }
        }       
    }

}


void sort_quality(struct student_s *s, int*arr,int count){

    int i=0;
    int j=0;
      
    for(i=0 ; i< count;i++)
        arr[i]=i;

    for(i=0 ; i< count-1;i++){        
        for(j=0; j<count-i-1;j++){          
            if(s[arr[j]].quality < s[arr[j+1]].quality){
                int temp = arr[j];
                arr[j] = arr[j+1];
                arr[j+1] = temp;
            }
        }      
    }


}


void sort_cost(struct student_s *s, int*arr,int count){

    int i=0;
    int j=0;
      
    for(i=0 ; i< count;i++ )
        arr[i]=i;

    for(i=0 ; i< count-1;i++){        
        for(j=0; j < count-i-1;j++){         
            if(s[arr[j]].cost>s[arr[j+1]].cost){
                int temp = arr[j];
                arr[j] = arr[j+1];
                arr[j+1] = temp;
            }
        }       
    }

}



void signal_handler(int signo){
    if(signo==SIGINT){
        int i=0;
        /* destory semaphores */
        
        if(sem_destroy(&hw_full)<0){
            fprintf(stderr,"hw_full sem could not be destroyed\n");
            exit(-1);
        }

        if(sem_destroy(&hw_empty)<0){
            fprintf(stderr,"hw_empty sem could not be destroyed\n");
            exit(-1);
        }

        if(sem_destroy(&hw_mutex)<0){
            fprintf(stderr,"hw_mutex could not be destroyed\n");
            exit(-1);
        }


        for(i=0;i<student_cnt;i++){
            if(sem_destroy(&student_hire_mutex[i])<0){
                fprintf(stderr,"student_hire_mutex[%d] could not be destroyed\n",i);
                exit(-1);
            }
        }
        /* make all threads alive params -1 */
        for (i=0; i< student_cnt;i++){
            students->alive=-1;
        }
        /* join thread*/

         pthread_join(thrd_h, NULL);

         for(i = 0 ;i<student_cnt;i++){
             pthread_join(worker_threads[i], NULL);
         }
        /* free blocks*/
        free(hw_queue->array);
        free(hw_queue);
    }
}


void* thread_h_work(void* params){
    int* tfd = (int*) params;
    int fd = *tfd;
    char c;
    int size=0;

    size = read(fd,&c,1);
    if(size<0){
        fprintf(stderr,"Thread H: File read error\n");
        exit(-1);
    }
    while(size>0 || low_budget_flag!=-1){
        
        semwait(&hw_empty);
        semwait(&hw_mutex);
        offer(c);
        fprintf(stderr,"H has a new homework  %c;  Remaining Money is %d\n",c,budget);
        sempost(&hw_mutex);
        sempost(&hw_full);
        
        size = read(fd,&c,1);
        if(size<0){
            fprintf(stderr,"Thread H: File read error\n");
            exit(-1);
        }
    }

    if(low_budget_flag==1){
        fprintf(stderr,"H has no more money for hw... Terminating....\n");
    }

    if(size<=0){
        fprintf(stderr,"H has no more new hw ... Terminating....\n");
    }

    return 0;
}


void* worker_thread(void* params){

    int* i = (int*) params;
    int index= *i;
    atomic_int hw_count=0;
    int sleep_duration = students[index].speed;
    
    /*int alive = students[index].alive;*/

    while(students[index].alive != -1){
        
        if(students[index].avaliable == 0 && students[index].assigned==1){

            semwait(&student_hire_mutex[index]);
            students[index].avaliable = 1;
            sleep(sleep_duration);
            students[index].hw_solved++;
            students[index].money_made += students[index].cost;
            students[index].avaliable = 0;
            students[index].assigned=0;
            semwait(&student_hire_mutex[index]);

        }

    }

   fprintf(stderr,"%s solved %u hw, made %u money\n",students[index].name,students[index].hw_solved,students[index].money_made);
   return 0 ;
}


