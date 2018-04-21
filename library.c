#include "clipboard.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "utils.h"

int clipboard_connect(char *clipboard_dir){
	
	struct sockaddr_un server_addr;
	struct sockaddr_un client_addr;
	int sock_fd = -1;
	
	if( (sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket: ");
		exit(-1);
	}
	
	printf(" socket created \n");

	client_addr.sun_family = AF_UNIX;
	sprintf(client_addr.sun_path, "./socket_%d",getpid());

	int err = bind(sock_fd, (struct sockaddr *)&client_addr, sizeof(client_addr));
	if(err == -1) {
		perror("bind");
		exit(-1);
	}

	server_addr.sun_family = AF_UNIX;
	strcpy(server_addr.sun_path, clipboard_dir);

	int err_c = connect(sock_fd, (const struct sockaddr *) &server_addr, sizeof(server_addr));
	if(err_c==-1){
				printf("Error connecting\n");
				exit(-1);
	}

	printf("connected %d\n", err_c);

	return sock_fd;
}

int clipboard_copy(int clipboard_id, int region, void *buf, size_t count) {

	int bytes_written = -1;

	char * msg = getBuffer(0, region, (char*) buf, count);

	if( (bytes_written = write(clipboard_id, msg, count)) == -1){
		printf("Error writing to clipboard: %s\n", strerror(errno));
		free(msg);
		return -1;
	} else {
		free(msg);
		return bytes_written;
	}
}

int clipboard_paste(int clipboard_id, int region, void *buf, size_t count) {

	printf("The region is: %d\n", region);
	

	char *msg = getBuffer(1, region, "", count);

	// requests the content of a certain region
	if(write(clipboard_id, msg, count) == -1){
		printf("Error writing to clipboard: %s\n", strerror(errno));
		free(msg);
		return -1;
	}

	// falta meter um while aqui
	int bytes_read = read(clipboard_id, buf, count);

	free(msg);

	return bytes_read;
}

/// This function closes the connection between the application and the local clipboard
void clipboard_close(int clipboard_id){
	if(close(clipboard_id) == -1) {
		printf("Error closing connection: %s\n", strerror(errno));
	}
	return;
}
