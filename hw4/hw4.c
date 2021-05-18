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


/* student budget*/
int budget;

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
}student_s;

struct student_s *students;  

/* threads */
void * thread_h_work(void* params);
void* worker_thread(void* params);


/* essential functions to prepare enviroment */
int student_count(char* filepath);
void fill_students(struct student_s * s, int student_count, char*filename);
void sort_speed(struct student_s *s, int*);
void sort_quality(struct student_s *s, int*);
void sort_cost(struct student_s *s, int*);
void print_usage();


/* queue functions */

void init_queue(int capacity);
char poll();
void offer(char hw);

void signal_handler(int signo);

int main(int argc, char**argv){

    char *hwfile, *student_for_hire;
    int student_cnt;
    int *speed_index;
    int *quality_index;
    int *cost_index;

    pthread_t thread_h;
    pthread_t *worker_threads;

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
    student_cnt=student_count(hwfile);
    students = (struct student_s*) calloc (student_cnt,sizeof(struct student_s));
    hw_queue = (struct queue_s*) calloc(1,sizeof(struct queue_s));
    worker_threads = (pthread_t*) calloc(student_cnt,sizeof(pthread_t));

    fill_students(students,student_cnt,student_for_hire);
    init_queue(student_cnt+1);


    /* Create Threads */







    /* main thread Loop*/












    /* join threads */



    free(speed_index);
    free(cost_index);
    free(quality_index);
    free(students);
    free(student_hire_mutex);
    free(worker_threads);
    free(hw_queue->array);
    free(hw_queue);

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
        printf("C : %c     P : %c\n",c,p);
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
  

    inputfile = fopen("filename","r");
    if(inputfile == NULL){
        fprintf(stderr,"File could not be opened\n");
        exit(-1);
    }

    for(i=0;i<student_count;i++){
        char name[20];
        atomic_int quality;
        atomic_int speed;
        atomic_int cost;

        fscanf(inputfile,"%s %d %d %d ",&name,&quality,&speed,&cost);

        strcpy(s[i].name,name);
        s[i].quality=quality;
        s[i].speed=speed;
        s[i].cost = cost;
        s[i].avaliable = 0;
        s[i].alive=1;
        s[i].assigned=0;

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

void signal_handler(int signo){
    if(signo==SIGINT){
        /* destory semaphores */

        /* make all threads alive params -1 */

        /* join thread*/

        /* free blocks*/
    }
}

void* thread_h_work(void* params){}
void* worker_thread(void* params){}

