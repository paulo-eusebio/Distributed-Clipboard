#ifndef LIBRARY
#define LIBRARY

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "utils.h"
#include "sock_stream.h"
#include "clipthreads.h"
#include "linkedList.h"

#include <sys/types.h>

/*** GLOBAL VARIABLES ***/

// Threads Declaration
pthread_t thread_app_listen_id;
pthread_t thread_clip_id;
pthread_t stdin_thread;

// matrix for our regions
char **regions;

size_t regions_length[10];

pthread_rwlock_t regions_rwlock[10];

// stack for save the file descriptors of apps
List* list_apps;

// stack for save the file descriptors of clips
List* list_clips;

// file descriptor of the parent clipboard
// if -1 it doesnt have a parent, else it does
int fd_parent;
pthread_mutex_t parent_socket_lock;// = PTHREAD_MUTEX_INITIALIZER;

//one conditional variable for each region
pthread_cond_t wait_regions[10];
pthread_mutex_t region_waits[10];
/************************/

void getClipboardBackUp(char const *argv[]);
void terminate();

// type -> 0 if copy, 1 if paste

#endif