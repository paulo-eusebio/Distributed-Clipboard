#include "clipboard.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int clipboard_connect(char *clipboard_dir){
	char fifo_name[100];

	sprintf(fifo_name, "%s%s", clipboard_dir, INBOUND_FIFO);
	int fifo_send = open(fifo_name, O_WRONLY);
	if(fifo_send == -1) {
		printf("Error opening in fifo: %s\n", strerror(errno));
		exit(-1);
	}
	sprintf(fifo_name, "%s%s", clipboard_dir, OUTBOUND_FIFO);
	int fifo_recv = open(fifo_name, O_RDONLY);
	if(fifo_recv == -1) {
		printf("Error opening out fifo: %s\n", strerror(errno));
		exit(-1);
	}

	// To check if the recv wasn't opened
	if (fifo_recv < 0)
		return fifo_recv;
	else
		return fifo_send;
}

int clipboard_copy(int clipboard_id, int region, void *buf, size_t count) {

	int bytes_written = -1;

	if( (bytes_written = write(clipboard_id, buf, count)) == -1){
		printf("Error writing to clipboard: %s\n", strerror(errno));
		return -1;
	} else {
		return bytes_written;
	}
}

int clipboard_paste(int clipboard_id, int region, void *buf, size_t count) {

	clipboard_copy(clipboard_id, -1, buf, count);
	
	int bytes_read = read(clipboard_id + 1, buf, sizeof(buf));

	return bytes_read;
}

/// This function closes the connection between the application and the local clipboard
void clipboard_close(int clipboard_id){
	if(close(clipboard_id) == -1) {
		printf("Error closing connection: %s\n", strerror(errno));
	}
	return;
}
