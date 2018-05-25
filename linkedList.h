#ifndef LINKEDLIST_HEADER
#define LINKEDLIST_HEADER

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct node {
  int fd;
  pthread_t id;
  struct node * next;
} Node;


typedef struct list {
  Node * head;
  pthread_mutex_t list_mutex; 
} List;


Node *createnode(int fd, pthread_t thread_id);
List * emptylist();
void add(int fd, pthread_t thread_id, List * list);
void freeNode(int data, List * list);
void display(List * list);
void reverse(List * list);
void destroy(List * list);

#endif