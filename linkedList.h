#ifndef LINKEDLIST_HEADER
#define LINKEDLIST_HEADER

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

typedef struct node {
  int fd;
  pthread_t *id;
  pthread_mutex_t *mutex;
  struct node * next;
} Node;


typedef struct list {
  Node * head;
  pthread_mutex_t list_mutex; 
} List;


Node *createnode(int fd, pthread_t *thread_id, pthread_mutex_t *mutex);
List * emptylist();
void add(int fd, pthread_t *thread_id, pthread_mutex_t *mutex, List * list);
void freeNode(int data, List * list);
void destroyNodeMutex(int fd, List * list);
pthread_mutex_t *getNodeMutex(int fd, List * list);
void display(List * list);
void reverse(List * list);
void destroy(List * list);

#endif