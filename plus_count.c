#include<stdio.h>
#include<string.h>
#include<ctype.h>

int regex_plus_count(char*filepath){
    int count=0;
    int len = strlen(filepath);
    int i;
    for(i=0;i<len;i++){
        if(filepath[i]=='+'){
            count++;
        }
    }

    return count;
}

int filename_checker(char*filename,char*regex){

    int j = 0;
    int i = 0;
    for(; filename[i] != '\0' && regex[j] != '\0'; ++i) {
        /*fprintf(stderr,"Top For: filename : %s    regrex : %s   \n",filename,regex);*/
        /*fprintf(stderr,"Outer For Loop\n filename[%d] : %c\nRegex[%d]: %c\n\n",i,filename[i],j,regex[j]);*/
        if(regex[j] == '+') {
            char r = tolower(regex[j-1]);
                for(; filename[i] != '\0'; ++i) {
                    /*fprintf(stderr,"inner For Loop\n filename[%d] : %c\nRegex[%d]: %c\nr : %c\n\n",i,filename[i],j,regex[j],r);*/
                    if(tolower(filename[i]) != r) {
                        i--;
                        break;
                    }
                }
            j ++;
        } else if(tolower(filename[i]) == tolower(regex[j])) {
            /*fprintf(stderr,"Outer For Loop Else If\n filename[%d] : %c\nRegex[%d]: %c\n\n",i,filename[i],j,regex[j]);*/
            j ++;
        } else {
            return 0;
        }
    }

    return filename[i] == '\0' && regex[j] == '\0';
}


int main(int argc,char* argv[]){

    char* string;
    char* regex;
   /* int i=0,j=0;*/

    string=argv[1];
    regex=argv[2];

    printf("String : %s\nRegex : %s\nResult : %d\n",string,regex,filename_checker(string,regex));
  

    return 0;
}