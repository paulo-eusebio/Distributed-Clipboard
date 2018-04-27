#ifndef STACK_API
#define STACK_API

// C program for linked list implementation of stack
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>

// A structure to represent a stack
struct StackNode
{
    int data;
    pthread_t id;
    int thread_status;
    struct StackNode* next;
};

struct StackNode* newNode(int data);

int isEmpty(struct StackNode *root);

void push(struct StackNode** root, int data);

int pop(struct StackNode** root);

int peek(struct StackNode* root);

#endif