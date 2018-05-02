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

// thread for listening to another clipboards that want to connect to this clipboard
void * thread_clips_listen(void * data);

// thread for reading and writing to the clipboard that this clipboard just accepted the connection
void * thread_clips(void * data);

#endif