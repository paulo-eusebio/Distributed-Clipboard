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

#include "clipboard.h"

#define STRINGSIZE 255

void preparefifos(int *fifo_in, int*fifo_out);

void * mymalloc(int size);

#endif