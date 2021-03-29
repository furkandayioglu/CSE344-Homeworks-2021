#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>

/* Traverse A Directory Hierarchy */
/* Sum sizes of regular files */
int postOrderApply(char *path,int pathfun(char* path1));
int sizepathfun(char *path);
void permission_string(mode_t mode);
void file_type(mode_t mode);

int paramStat=0;


int main(int argc,char* argv[]){

    char*path;
    int size;

    if(argc<2 || argc > 3){
        printf("USAGE\n");
        printf("Program should have at least 2 arguments \n");
        printf("buNeDu [optional parameter -z] PATH \n");
        exit(1);
    }

    if(argc == 3 && (strcmp(argv[1],"-z")==0)){
     /* Parameter Check*/
        paramStat=1;
        path = argv[2];
       size = postOrderApply(path,sizepathfun);
       printf(" %d  : %s  \n",size,path);
        /***/
    }else if(argc == 2 && (strcmp(argv[1],"-z") != 0)){
        path = argv[1];
        
        size = postOrderApply(path,sizepathfun);

       printf(" %d  : %s  \n",size,path);
    }else{
        printf("USAGE\n");
        printf("Program should have at least 2 arguments \n");
        printf("buNeDu [optional parameter -z] PATH \n");
        exit(1);
    }

    return 0;
}

int postOrderApply(char*path,int pathfun(char* path1)){
    DIR *dp;					// directory pointer
	struct dirent *dir;
    struct stat statBuf;			// dirent struct
	int status = 0;				// status is total found counter
				
    int currentSize=0;          // size of current Directory
    int previousDirSize=0;      // Size of previous directory;
 
    dp = opendir(path);

    if(dp){
        while((dir = readdir(dp)) != NULL){
            char* tempPath = (char*) malloc(sizeof(char)*PATH_MAX);
            if(strcmp(dir->d_name, ".") && strcmp(dir->d_name, "..")){

                strcpy(tempPath,path);
                strcat(tempPath,"/");
                strcat(tempPath,dir->d_name);

                stat(tempPath,&statBuf);

                fprintf(stderr,"File name (dir->d_name): %s\n",dir->d_name);
                fprintf(stderr,"Temp path : %s\n",tempPath);
                file_type(statBuf.st_mode);
                permission_string(statBuf.st_mode);

                if(S_ISDIR(statBuf.st_mode) ){
                    int size = postOrderApply(tempPath,pathfun);
                    if(size>0){
                        currentSize+=size;
                        /*printf(" %d  : %s  \n",size,tempPath);*/
                    }
                    if(paramStat == 1){
                        previousDirSize=size;
                        currentSize-=previousDirSize;
                        printf(" %d  : %s  \n",size,tempPath);
                    }
                }else{
                    
                    if(pathfun(tempPath) >= 0){
                        int size;
                        size = pathfun(tempPath);
                        currentSize += size;
                        
                       /* printf(" %d  : %s  \n",size,tempPath);*/
                        if(paramStat == 1){
                            previousDirSize = currentSize;
                        }
                   
                    }
                }
            }

            free(tempPath);
        }

        while((closedir(dp) == 0) && (errno == EINTR));
    }else{
        perror("Directory Could not be opened.\n");
        perror("ABORT PROGRAM \n");
        exit(1);

    }


return currentSize;
}

/* https://stackoverflow.com/questions/25735299/traversing-a-path-in-c*/
/* sizepathfun is taken from here */

int sizepathfun(char *path){
    struct stat statbuf;
    if(stat(path, &statbuf) == -1){

        
        perror("Failed to get file status");
        return -1;
    }

    /*file_type(statbuf.st_mode);*/
    /*permission_string(statbuf.st_mode);*/
    /*fprintf(stderr,"File path : %s\n\n",path);*/
    if(S_ISREG(statbuf.st_mode) == 0){
        
        printf("Special File Beware : %s",path);
        return 0;
    }else{
        return statbuf.st_size;
    }
}


void permission_string(mode_t mode){
    char permission_str[10];

    permission_str[6]= mode & S_IROTH ? 'r' : '-';
    permission_str[7]= mode & S_IWOTH ? 'w' : '-';
    permission_str[8]= mode & S_IXOTH ? 'x' : '-';

    permission_str[3]= mode & S_IRGRP ? 'r' : '-';
    permission_str[4]= mode & S_IWGRP ? 'w' : '-';
    permission_str[5]= mode & S_IXGRP ? 'x' : '-';

    permission_str[0]= mode & S_IRUSR ? 'r' : '-';
    permission_str[1]= mode & S_IWUSR ? 'w' : '-';
    permission_str[2]= mode & S_IXUSR ? 'x' : '-';

    permission_str[9]='\0';

    fprintf(stderr,"Permission string: %s\n\n\n",permission_str);
}

void file_type(mode_t mode){

    char file_typ= ' ';

    if(S_ISREG(mode)>0){
        file_typ='f';
    }else if(S_ISDIR(mode)>0){
        file_typ='d';
    }else if(S_ISCHR(mode)>0){
        file_typ='c';
    }else if(S_ISBLK(mode)>0){
        file_typ = 'b';
    }else if(S_ISFIFO(mode)>0){
        file_typ = 'p';
    }else if(S_ISSOCK(mode)>0){
        file_typ = 's';
    }else if(S_ISLNK(mode)>0){
        file_typ='l';
    }
    
    fprintf(stderr,"File type : %c\n",file_typ);
   
}