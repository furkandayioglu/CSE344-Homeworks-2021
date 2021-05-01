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

struct syncronizer_t{
    int * buffer;
    atomic_int file_offset;

    sem_t buffer_full;
    sem_t buffer_empty;

    sem_t vac1;
    sem_t vac2;
    

}



void nurse(int fd,int* buffer,int t, int c, );
void get_vacc_storage();
void fill_vacc_buffer();
void vaccinator();

void citizen();


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
int main(int argc, char** argv){
    int opt;
    char* inputfilePath;
    int nurse,vaccinator,citizen,dose,buffer_size;





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




















}