#include "linkedList.h"

Node *createnode(int fd, pthread_t *thread_id, pthread_mutex_t *mutex){
  Node * newNode = malloc(sizeof(Node));
  newNode->fd = fd;
  newNode->id = thread_id;

  newNode->mutex = mutex;

  newNode->next = NULL;
  return newNode;
}

List * emptylist(){
  List * list = malloc(sizeof(List));
  list->head = NULL;

  if(pthread_mutex_init(&list->list_mutex, NULL) != 0){
    perror("Error creating the mutex of a list");
    exit(-1);
  }
  
  return list;
}

void display(List * list) {
  Node * current = list->head;
  
  if(list->head == NULL) 
    return;

  while(current->next != NULL){
    printf("%d, %d;", current->fd, (int) *current->id);
    current = current->next;
  }

  printf("%d, %d", current->fd, (int) *current->id);
}


/// adds to the end of the list
void add(int fd, pthread_t *thread_id, pthread_mutex_t *mutex, List * list){
  Node * current = NULL;

  if(list->head == NULL){
    list->head = createnode(fd, thread_id, mutex);
  }
  else {
    current = list->head; 
    while (current->next!=NULL){
      current = current->next;
    }
    
    current->next = createnode(fd, thread_id, mutex);
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


/// destroy a mutex of a certain node
void destroyNodeMutex(int fd, List * list){
  Node * current = list->head;                      
  while(current != NULL){           
    if(current->fd == fd){      
      if( pthread_mutex_destroy(current->mutex) != 0) {
        perror("Error while destroying fd parent");
      }
      return;
    }                                          
    current = current->next;        
  }                                 
}      

/// returns the mutex of a certain node
pthread_mutex_t *getNodeMutex(int fd, List * list) {
  Node * current = list->head;                       
  while(current != NULL){           
    if(current->fd == fd){      
      return current->mutex;
    }                                          
    current = current->next;        
  }                    

  return NULL;             
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

    if( close(current->fd) ) {
        perror("Closing file descriptor");
    }

    pthread_detach(*current->id);

    free(current);
    current = next;
  }
  free(list);
}
