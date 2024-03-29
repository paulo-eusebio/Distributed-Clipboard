#ifndef UTILS
#define UTILS

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>

#include "clipboard.h"
#include "linkedList.h"
#include "sock_stream.h"

#define STRINGSIZE 255
#define MAX_INPUT 100
#define NUM_REG 10

#define CONNECTED 1
#define SINGLE 0

#define COPY 0
#define PASTE_REQUEST 1
#define PASTE_REPLY 2
#define REPLICATE 3

#define PORT_MIN 8000
#define PORT_MAX 8100

void * mymalloc(int size);

char * getBuffer(int type, int region, char *message, int length);

void ctrl_c_callback_handler(int signum);

int checkMode(int argc);

void setSockaddrIP( struct sockaddr_in * server, socklen_t *addrlen, struct in_addr * addr, unsigned short port);

void getBackup(int fd);

int writeRoutine(int fd, char *buffer, size_t length);

int readRoutine(int fd, char *storageBuf, size_t length);

char* getPasteMessage(int region, char **regions);

int randGenerator(int min, int max);

void freeClipboard();

void dealCopyRequests(int fd, char information[15]);

void dealPasteRequests(int fd, char information[15]);

int sendToChildren(char *message, int region, int len_message);

int sendToParent(char *message, int region, int len_message);

void sendBackup(int fd);

#endif
