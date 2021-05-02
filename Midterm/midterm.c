#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdatomic.h>
#include <string.h>

typedef struct syncronizer_t{
    /*atomic_int* buffer;*/
    /*atomic_int file_offset;*/
    sem_t vac1_count;
    sem_t vac2_count;
    atomic_int total_vax;

    /*semaphore for buffer */
    sem_t buffer_full;
    sem_t buffer_empty;
    sem_t storage_mutex;

    /*  vax shot semaphore */
    sem_t vac1_mutex;
    sem_t vac2_mutex;
    
}syncronizer_t;

static struct syncronize_t* sync;


void init_sync(int b){

    if(sem_init(&sync->vac1_count,1,0)<0){
        fprintf(stderr,"Vaccine 1 counter sem could not be initialized\n")
        exit(-1);
    }

    if(sem_init(&sync->vac1_coun2,1,0)<0){
        fprintf(stderr,"Vaccine 2 counter sem could not be initialized\n")
        exit(-1);
    }

    if(sem_init(&sync->buffer_full,1,0)<0){
        fprintf(stderr,"Buffer_full sem could not be initialized\n")
        exit(-1);
    }

    if(sem_init(&sync->buffer_empty,1,b)<0){
        fprintf(stderr,"Buffer_empty sem could not be initialized\n")
        exit(-1);
    }

    if(sem_init(&sync->storage_mutex,1,1)<0){
        fprintf(stderr,"Storage mutex could not be initialized\n")
        exit(-1);
    }

    if(sem_init(&sync->vac1_mutex,1,1)<0){
        fprintf(stderr,"Vaccine 1 mutex could not be initialized\n")
        exit(-1);
    }

    if(sem_init(&sync->vac2_mutex,1,1)<0){
        fprintf(stderr,"Vaccine 2 mutex could not be initialized\n")
        exit(-1);
    }

    sync->total_vax=0;
    

}



void destroy_sync(){

   f(sem_destroy(&sync->vac1_count)<0){
        fprintf(stderr,"Vaccine 1 counter sem could not be destroyed\n")
        exit(-1);
    }

    if(sem_destroy(&sync->vac1_coun2)<0){
        fprintf(stderr,"Vaccine 2 counter sem could not be destroyed\n")
        exit(-1);
    }

    if(sem_destroy(&sync->buffer_full)<0){
        fprintf(stderr,"Buffer_full sem could not be destroyed\n")
        exit(-1);
    }

    if(sem_destroy(&sync->buffer_empty)<0){
        fprintf(stderr,"Buffer_empty sem could not be destroyed\n")
        exit(-1);
    }

    if(sem_destroy(&sync->storage_mutex)<0){
        fprintf(stderr,"Storage mutex could not be destroyed\n")
        exit(-1);
    }

    if(sem_destroy(&sync->vac1_mutex)<0){
        fprintf(stderr,"Vaccine 1 mutex could not be destroyed\n")
        exit(-1);
    }

    if(sem_destroy(&sync->vac2_mutex)<0){
        fprintf(stderr,"Vaccine 2 mutex could not be destroyed\n")
        exit(-1);
    }

   
}

/* read with lock
    it will lock file while one of the nurses is reading*/
int read_with_lock(int fd, void*buffer, int buffer_len){

    struct flock lock;
    memset(&lock,0,sizeof(lock));
    int read_size;

    lock.l_type=F_RDLCK;
    if(fcntl(fd,F_SETLKW,&lock)<0){
        fprintf(stderr,"Read lock could not be set.\n");
        close(fd); 
        exit(-1);
    }
    
    read_size=read(fd,buffer,count);

    lock.l_type=F_UNLCK;
    if(fcntl(fd,F_SETLKW,&lock)<0){
        fprintf(stderr,"Read unlock could not be set.\n");
        close(fd); 
        exit(-1);
    }

    return read_size;
}


/* sem_wait with error check*/
void semwait(sem_t* sem){

    if(sem_wait(sem)<0){
        fprintf(stderr,"Sem Wait Failed.\n");
        exit(-1)
    }
}

/* sem_post with error checking */
void sempost(sem_t* sem){

    if(sem_post(sem)<0){
        fprintf(stderr,"Sem Post Failed.\n");
        exit(-1);
    }
}


/* sem_getvalue wşth error checking */
int semvalue(sem_t* sem){
    int val;
    if(sem_getvalue(sem,&val)<0){
        fprintf(stderr,"Sem Get Value Failed.\n");
        exit(-1);
    }

    return val;
}




void get_vacc_storage();
void fill_vacc_buffer();
void add_vax1();
void add_vax2();
void vaccinate_dose1_citizen();
void vaccinate_dose2_citizen();

void nurse(int i,int fd,int t, int c, int buffer_size){

    /*int filesize = 2*t*c;*/
    char vax;
    int size;
    char message[200];

    size=read_with_lock(fd,&vax,1);
    if(size<0){
        fprintf(stderr,"Read error\n");
        exit(-1);
    }
    while(size>0){
        
        if(vax == '1'){
            sprintf(message,"Nurse %d (pid=%d) has brought vaccine 1: clinic has %d vaccine1 %d vaccine 2",i+1,getpid(),semvalue(&sync->vac_1))
            /*vax1 to storage*/
            add_vax1();

        }
        
        if(vax=='2'){
            /*vax1 to storage*/
            add_vax2();
        }

        size=read_with_lock(fd,&vax,1);
        if(size<0){
            fprintf(stderr,"Read error\n");
            exit(-1);
        }
    }
}


void vaccinator(int i,int t, int c, int buffer_size);

void citizen(int i,int t,int c);


void sig_handler(int signum){

    if (signum == SIGINT){
        fprintf(stderr,"SIGINT signal occured.\nExiting.\n");


        /*delete semaphores */
        destroy_sync();


        /* Uninitialize the shared memory */
        if(munmap(sync,sizeof(syncronizer_t))<0){
            fprintf(stderr,"Shared memory could not be unmapped.\n");
            exit(-1);
        }

        /* Unlinks the shared memory */
        if(shm_unlink("sync")<0){
            fprintf(stderr,"Shared memory could not be unlinked.\n");
            exit(-1);
        }

        exit(0);
    
    }else if(signum== SIGUSR1){

    }
}

void print_usage();

int main(int argc, char** argv){
    int opt;
    char* inputfilePath;
    int nurse,vaccinator,citizen,dose,buffer_size;
    struct sigaction sact;
    int input_fd, shm_fd;
    int i=0;

    sigemptyset(&sact.sa_mask); 
    sact.sa_flags = SA_RESTART;
    sact.sa_handler = sig_handler;


    /* sigaction for SIGINT */
    if (sigaction(SIGINT, &sact, NULL) != 0){
        fprintf(stderr,"SIGINT could not be added sigaction()\n");
        exit(-1);
    }

    /* sigaction for SIGINT */
    if (sigaction(SIGUSR1, &sact, NULL) != 0){
        fprintf(stderr,"SIGUSR1 ould not be added sigaction()\n");
        exit(-1);
    }

    if(argc<13){
        fprintf(stderr,"Invalid Amount Of Parameter\n");
        exit(-1);
    }


    while ((opt = getopt (argc, argv, ":n:v:c:b:t:i:")) != -1){
        switch (opt)
        {
            case 'n':
                nurse=atoi(optarg);
                if(nurse<2){
                    fprintf(stderr,"Nurse count must bu greater or equal to 2\n");
                    exit(-1);
                }
                break;
            case 'v':
                vaccinator = atoi(optarg); 
                if(vaccinator<2){
                    fprintf(stderr,"Vaccinator count must bu greater or equal to 2\n");
                    exit(-1);
                }   
                break;
            case 'c':
                citizen = atoi(optarg);
                if(citizen<3){
                    fprintf(stderr,"citizen count must bu greater or equal to 3\n");
                    exit(-1);
                }                
                break;
            case 'b':
                buffer_size=atoi(optarg);               
                break;
            case 't':
                dose = atoi(optarg); 
                break;
            case '-i':
                inputfilePath = optarg;                
                break;
            case ':':  
                errno=EINVAL;
                print_usage();
                exit(-1);
                break;  
            case '?':  
                errno=EINVAL;
                print_usage();
                exit(-1);
                break; 
            default:
                exit(-1);
        }
    }


    if(buffer_size < (citizen*dose+1)){
        fprintf(stderr,"Invalid Buffer size\nBuffer size must be greater or equal to TxC+1\n");
        exit(-1);
    }

    input_fd = open(inputfilePath,O_RDONLY);
    if(input_fd < 0){
        fprintf(stderr,"Input file could not be opened\n");
        exit(-1);
    }

    shm_fd=shm_open("sync",O_CREAT | O_RDWR | O_TRUNC,S_IRWXU | S_IRWXO );
    if(shm_fd<0){
        fprintf(stderr,"Shared memory coould not be opened\n");
        exit(-1);
    }

    if(ftruncate(shm_fd,sizeof(shared_mem))<0){
        fprintf(stderr,"Shared memory could not be initiliazed by truncate");
        exit(-1);
    }


    sync=(syncronizer_t*)mmap(NULL,sizeof(syncronizer_t),PROT_READ|PROT_WRITE,MAP_SHARED,shm_fd,0);
    if((void*)sync==MAP_FAILED){
        fprintf(stderr,"Shared memory mmap error.\n");
        exit(-1);
    }

    /* TODO Initialize semaphores */

    init_sync(buffer_size);

    fprintf(stderr,"Welcome to the CSE344 clinic. Number of citizen to vaccinate c=%d with t=%d doses.\n",c,t);

    
    for(i=0;i<nurse;i++){
        switch (fork())
        {
            case -1:
                fprintf(stderr,"Nurse could not be created.\n");
                exit(-1);
                break;
            case 0:
                nurse(i,input_fd,dose,citizen,buffer_size);
                exit(EXIT_SUCCESS);
            default:
                break;
        }

    }

    for(i=0;i<citizen;i++){
        switch (fork())
        {
            case -1:
                fprintf(stderr,"Citizen could not be created.\n");
                exit(-1);
                break;
            case 0:
                citizen(i,dose,citizen);
                exit(EXIT_SUCCESS);
            default:
                break;
        }

    }




    for(i=0;i<citizen;i++){
        switch (fork())
        {
            case -1:
                fprintf(stderr,"Vaccinator could not be created.\n");
                exit(-1);
                break;
            case 0:
                vacctinator(i,dose,citizen,buffer_size);
                exit(-1);
            default:
                break;
        }

    }


    destroy_sync();

    /* Uninitialize the shared memory */
    if(munmap(sync,sizeof(syncronizer_t))<0){
        fprintf(stderr,"Shared memory could not be unmapped.\n");
        exit(-1);
    }

    /* Unlinks the shared memory */
    if(shm_unlink("sync")<0){
        fprintf(stderr,"Shared memory could not be unlinked.\n");
        exit(-1);
    }


    close(input_fd);
    return 0;
}




void print_usage(){
    fprintf(stderr,"###Usage###\n");
    fprintf(stderr,"Program has 13 parameters\n");
    fprintf(stderr,"./program -n NURSE_COUNT -v VACCINATOR_COUNT -c CITIZEN_COUNT -b BUFFER_SIZE -t 2shots_DOSE -i InputFile\n");
    fprintf(stderr,"n >= 2 \n");
    fprintf(stderr,"v >= 2\n");
    fprintf(stderr,"c >= 3 \n");
    fprintf(stderr,"b >= tc+1\n");
    fprintf(stderr,"t >=1 \n");
    
}

