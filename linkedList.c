#include "linkedList.h"

Node *createnode(int fd, pthread_t thread_id){
  Node * newNode = malloc(sizeof(Node));
  newNode->fd = fd;
  newNode->id = thread_id;
  newNode->next = NULL;
  return newNode;
}

List * emptylist(){
  List * list = malloc(sizeof(List));
  list->head = NULL;
  return list;
}

void display(List * list) {
  Node * current = list->head;
  if(list->head == NULL) 
    return;
  while(current->next != NULL){
    printf("%d, %d;", current->fd, (int) current->id);
    current = current->next;
  }
  printf("%d, %d", current->fd, (int) current->id);
}


/// adds to the end of the list
void add(int fd, pthread_t thread_id, List * list){
  Node * current = NULL;

  // METER MUTEXES

  if(list->head == NULL){
    list->head = createnode(fd, thread_id);
  }
  else {
    current = list->head; 
    while (current->next!=NULL){
      current = current->next;
    }
    current->next = createnode(fd, thread_id);
  }
}

/// free a certain node with data
void freeNode(int fd, List * list){
  Node * current = list->head;            
  Node * previous = current;           
  while(current != NULL){           
    if(current->fd == fd){      
      previous->next = current->next;
      if(current == list->head)
        list->head = current->next;
      free(current);
      return;
    }                               
    previous = current;             
    current = current->next;        
  }                                 
}                                   

void reverse(List * list){
  Node * reversed = NULL;
  Node * current = list->head;
  Node * temp = NULL;
  while(current != NULL){
    temp = current;
    current = current->next;
    temp->next = reversed;
    reversed = temp;
  }
  list->head = reversed;
}

/// frees the whole list
void destroy(List * list){

  if(list == NULL){
  	return;
  }

  Node * current = list->head;
  Node * next = current;
  while(current != NULL){
    next = current->next;
    //close(current->fd);
    free(current);
    current = next;
  }
  free(list);
}
