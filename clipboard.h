#ifndef CLIPBOARD
#define CLIPBOARD

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

#include <sys/types.h>

#define LISTENING_CLIPS_PORT 9000

struct Message {
    int type;
    int region;
    int length;
    char message[100];
};

struct Connection{
	int fd;
	pthread_t thread_id;
	struct Connection *next;
};

// type -> 0 if copy, 1 if paste

int clipboard_connect(char * clipboard_dir);
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count);
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count);
int clipboard_wait(int clipboard_id, int region, void *buf, size_t count);
void clipboard_close(int clipboard_id);

#endif

