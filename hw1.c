#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

struct search_t{
    char* filename;
    int size;
    char type;
    char* permissions;
    int link_count;
};
/*usage function*/
void print_usage();

/* file control functions */ 
int filename_checker(char*filename,char*regex);
int size_checker(int filesize,int expected_size);
int type_checker(mode_t type, char expected_type);
int permission_checker(mode_t permissions,char*expected_permissions);
int link_count_checker(int link_count,int expected_link_count);
int isFileOK();

/* helper functions */

void print_file(char*path,int level,int count);



/* traversal function*/
int directory_traversal(char*path,struct search_t* file);


int main(int argc,char**argv){

    struct search_t file;
    int opt;
    char* path = NULL;
    int wFlag_status =-1;

    file.filename = " ";
    file.size=-1;
    file.type=' ';
    file.link_count=-1;
    file.permissions= " ";

    if(argc < 5){
        
        print_usage();
        exit(-1);
    }

    while ((opt = getopt (argc, argv, ":w:f:b:t:p:l:")) != -1){
        switch (opt)
        {
            case 'w':
                path=optarg;
                wFlag_status=0;
                break;
            case 'f':
                file.filename = optarg;    
                break;
            case 'b':
                file.size = atoi(optarg);                
                break;
            case 't':
                file.type=optarg[0];               
                break;
            case 'p':
                file.permissions = optarg; 
                break;
            case 'l':
                file.link_count = atoi(optarg);                
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
    
    if(wFlag_status==-1){
        fprintf(stderr,"-w parameter is NULL\n");
        print_usage();
        exit(-1);
    }

  
    return 0;
}

void print_usage(){

    fprintf(stderr,"###USAGE###\n");
    fprintf(stderr,"This program must be started with at least 5 arguments\n");
    fprintf(stderr,"Mandatory argument:\n");
    fprintf(stderr,"-w filepath \n");
    fprintf(stderr,"Optional Parameters, but at least one of them must be used\n");
    fprintf(stderr,"-f filename\n");
    fprintf(stderr,"-b file size\n");
    fprintf(stderr,"-t file type\n");
    fprintf(stderr,"-p permissions\n");
    fprintf(stderr,"-l link count\n");

}

int isFileOK(char*filename,char*path,struct search_t* properties){

    int filename_status = 1;
    int size_status=1;
    int type_status=1;
    int permission_status=1;
    int link_status=1;
    struct stat stBuf;

    if(stat(path,&stBuf) == -1){
        fprintf(stderr,"IsFileOk Function. Stat buffer could not be created. Error Code :%d\n",errno);
        exit(-1);
    }

    if(strcmp(properties->filename," ")!=0){
        filename_status=filename_checker(filename,properties->filename);
    }

    if(properties->size!=-1){
        size_status=size_checker(stBuf.size,properties->size);
    }

    if(properties->type !=' '){
        type_status = type_checker(stBuf.st_mode,properties->type);
    }

    if(strcmp(properties->permissions," ")!=0){
        permission_status = permission_checker(stBuf.st_mode,properties->permissions);
    }
    
    if(properties->link_count!=-1){
        link_status=link_count_checker(stBuf.st_nlink,properties->link_count);
    }

    return (filename_status && size_status && type_status && permission_status && link_status);
}

int filename_checker(char*filename,char*regex){
    
    int j = 0;
    int i = 0;
    for(; filename[i] != '\0' && regex[j] != '\0'; ++i) {
        
        if(regex[j] == '+') {
            char r = tolower(regex[j-1]);
                for(; filename[i] != '\0'; ++i) {
                   
                    if(tolower(filename[i]) != r) {
                        i--;
                        break;
                    }
                }
            j ++;
        } else if(tolower(filename[i]) == tolower(regex[j])) {
           
            j ++;
        } else {
            return 0;
        }
    }

    return filename[i] == '\0' && regex[j] == '\0';
}

int size_checker(int filesize,int expected_size){
    int status=0;
    
    if(filesize==expected_size){
        status=1;
    }

    return status;
}


int type_checker(mode_t type, char expected_type){
    int status=0;
    char file_type= ' ';

    if(S_ISREG(type)>0){
        file_type='f';
    }else if(S_ISDIR(type)>0){
        file_type='d';
    }else if(S_ISCHR(type)>0){
        file_type='c';
    }else if(S_ISBLK(type)>0){
        file_type = 'b';
    }else if(S_ISFIFO(type)>0){
        file_type = 'p';
    }else if(S_ISSOCK(type)>0){
        file_type = 's';
    }else if(S_ISLNK(type)>0){
        file_type='l';
    }
    
    if(file_type == expected_type){
        status=1;
    }
    return status;
}


int permission_checker(mode_t permission,char*expected_permissions){
    int status = 0;
    char permission_str[10];

    permission_str[6]= permission & S_IROTH ? 'r' : '-';
    permission_str[7]= permission & S_IWOTH ? 'w' : '-';
    permission_str[8]= permission & S_IXOTH ? 'x' : '-';

    permission_str[3]= permission & S_IRGRP ? 'r' : '-';
    permission_str[4]= permission & S_IWGRP ? 'w' : '-';
    permission_str[5]= permission & S_IXGRP ? 'x' : '-';

    permission_str[0]= permission & S_IRUSR ? 'r' : '-';
    permission_str[1]= permission & S_IWUSR ? 'w' : '-';
    permission_str[2]= permission & S_IXUSR ? 'x' : '-';

    permission_str[9]='\0';
    if(strcmp(permission_str,expected_permissions) == 0){
        status=1;
    }

    return status;
}


int link_count_checker(int link_count,int expected_link_count){
    int status=0;
    
    if(link_count == expected_link_count)
        status=1;

    return status;
}



void print_file(char*path,int level,int count){

}

int directory_traversal(char*path,struct search_t* file){
    DIR *dp;
    struct dirent *dir;
    struct stat sfBuf;
    int level=0;
    

    dp = opendir(path);

    if(dp){
        while((dir = readdir(dp)) != NULL){
          char* tPath;      
















        }

   }else{
        perror("Directory Could not be opened.\n");
        perror("ABORT PROGRAM \n");
        exit(1);

    }










}
