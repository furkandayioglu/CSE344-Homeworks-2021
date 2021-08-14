#include "myqueue.h"
#include <stdlib.h>
#include <stdio.h>

/*node_t* head;
node_t* tail;


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
}*/