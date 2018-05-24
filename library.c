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

	if (region < 0 || region > 9) {
		printf("Trying to copy to an invalid region.\n");
		return 0;
	}

	if (clipboard_id < 0) {
		printf("Trying to copy to an invalid fd.\n");
		return 0;
	}

	char message[15] = "";

	// bytes sent
	int total = 0;

	// starts message as all \0
	memset(message, '\0', 15);

	sprintf(message,"c %d %d", region, (int) count);

	// sets up the clipboard for be ready to receive a message of a certain size 
	// to insert inside a certain region
	if(writeRoutine(clipboard_id, message, (size_t) sizeof(message)) == -1) {
		// error writing
		return 0;
	}

	// sends the info to copy to the region
	if((total = writeRoutine(clipboard_id, buf, count)) == -1) {
		// error writing
		return 0;
	}

	return total;
}

int clipboard_paste(int clipboard_id, int region, void *buf, size_t count) {
	
	if (region < 0 || region > 9) {
		printf("Trying to paste from an invalid region.\n");
		return 0;
	}

	if (clipboard_id < 0) {
		printf("Trying to paste from an invalid fd.\n");
		return 0;
	}

	char message[15] = "";

	// bytes sent
	int total = 0;

	// starts message as all \0
	memset(message, '\0', 15);

	sprintf(message,"p %d %d", region, (int) count);

	// asks the clipboard to send a message of a certain size from a certain region
	if(writeRoutine(clipboard_id, message, sizeof(message)) == -1) {
		// error writing
		return 0;
	}

	// starts message as all \0
	memset(message, '\0', 15);

	if( readRoutine(clipboard_id, message, sizeof(message)) == -1) {
		// error reading
		return 0;
	}

	int reg = -1;
	int len_message = -1;

	// decodes the message of the information about the region
	if (sscanf(message, "a %d %d", &reg, &len_message) != 2) {
		printf("sscanf didn't assign the variables correctly: clipboard_paste\n");
		return 0;
	}

	// No content in the region
	if(len_message == 0)
		return 0;

	// if the content available is less than what exists in the region
	if(len_message < count) {

		if( (total = readRoutine(clipboard_id, buf, len_message)) == -1) {
			// error reading
			return 0;
		}

	} else {

		if( (total = readRoutine(clipboard_id, buf, count)) == -1) {
			// error reading
			return 0;
		}
	}



	return total;
}

/// This function closes the connection between the application and the local clipboard
void clipboard_close(int clipboard_id) {

	if(close(clipboard_id) == -1) {
		printf("Error closing connection: %s\n", strerror(errno));
	}

	return;
}

int clipboard_wait(int clipboard_id, int region, void *buf, size_t count) {

	// TODO

	return 0;
}
