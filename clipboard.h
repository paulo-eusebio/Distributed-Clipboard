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
#include "utils.h"
#include "sock_stream.h"

#include <sys/types.h>

int clipboard_connect(char * clipboard_dir);

int clipboard_copy(int clipboard_id, int region, void *buf, size_t count);

int clipboard_paste(int clipboard_id, int region, void *buf, size_t count);

int clipboard_wait(int clipboard_id, int region, void *buf, size_t count);

void clipboard_close(int clipboard_id);

#endif

