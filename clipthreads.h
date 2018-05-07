#ifndef CLIPTHREADS
#define CLIPTHREADS

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

#include "clipboard.h"
#include "utils.h"
#include "sock_stream.h"
#include "linkedList.h"

// thread for listening to another clipboards that want to connect to this clipboard
void * thread_clips_listen(void * data);

// thread for reading and writing to the clipboard that this clipboard just accepted the connection
void * thread_clips(void * data);

// thread to receive new app conectioncs
void * thread_app_listen(void * data);


// thread that deals with connected apps
void * thread_apps(void * data);

// thread to deal with stdin inputs
void * thread_stdin(void * data);

#endif
