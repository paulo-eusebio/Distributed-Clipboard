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

#include "clipboard.h"
#include "utils.h"
#include "sock_stream.h"

int main(int argc, char const *argv[]) {


	int clip_mode = checkMode(argc);

	if (clip_mode == -1) {
		printf("Invalid input:\n - Single Mode: No arguments\n - Connected Mode: -c IP port\n");
		exit(-1);
	}

	/* Create Clipboard Regions*/
	char **regions = (char**) mymalloc(10*sizeof(char*));
	for (int i = 0; i < 10; ++i) {
	    regions[i] = (char *) mymalloc(STRINGSIZE+1);
	    memset(regions[i], 0, STRINGSIZE+1);
	}

	/* Sockets definition */

	// UNIX Sockets
	int fd_listen = -1, fd_connect = -1;
	struct sockaddr_un local_addr;
	struct sockaddr_un client_addr;
	socklen_t size_addr = sizeof(client_addr);

	// IPV4 Sockets
	int fd_client = -1;
	struct sockaddr_in ipv4_client;
	socklen_t addrlen = 0;

	unlink(SOCK_ADDRESS);
	
	signal(SIGINT, ctrl_c_callback_handler);


	//check if the clipboard is in single or connected mode
	if (clip_mode == SINGLE) {


	} else if (clip_mode == CONNECTED) {

		struct in_addr temp_addr;

		// create clipboard server socket
		if( (fd_client = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			perror("Error creating client socket");
			exit(1);
		}

		// converting the IP arg to the appropriate type
		inet_aton(argv[2], &temp_addr);
		// converting the port from a char* to unsigned short
		unsigned short client_port = (unsigned short) atoi(argv[3]);

		// setting up the Socket
		setSockaddrIP(&ipv4_client, &addrlen, &temp_addr, client_port);

		// connect to the server clipboard
		if( connect(fd_client, (struct sockaddr*) &ipv4_client, addrlen) == -1){
			perror("Error while connecting to another clipboard");
		}

	}

	
	if( (fd_listen = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("Opening socket: ");
		exit(-1);
	}

	// create a function for this
	local_addr.sun_family = AF_UNIX;
	strcpy(local_addr.sun_path, SOCK_ADDRESS);

	if(bind(fd_listen, (struct sockaddr *)&local_addr, sizeof(local_addr)) == -1) {
		perror("Error while binding: ");
		exit(-1);
	}

	if(listen(fd_listen, 2) == -1) {
		perror("Error while listening: ");
		exit(-1);
	}

	// isto vai ter que envolver um select, for sure!
	if( (fd_connect = accept(fd_listen, (struct sockaddr *) & client_addr, &size_addr)) == -1) {
		perror("Error accepting");
		exit(-1);
	}

	struct Message msg_recv;
	char *data = (char*)mymalloc(sizeof(char)*sizeof(msg_recv));

	while (1) {

		printf(".\n");

		if(read(fd_connect, data, sizeof(msg_recv)) == 0) { // we probably we will to do a loop here in the stream part
			printf("client disconnected, read is 0\n");
			exit(1);
		}
		memcpy(&msg_recv, data, sizeof(msg_recv));

		if (msg_recv.type == 0) {
			printf("Received a copy request for region %d\n", msg_recv.region);
			printf("received %s\n", msg_recv.message);

			// for guarantee that the region request is a valid region
			if (msg_recv.region >= 0 && msg_recv.region < 10){
				char *ptr = strcpy(regions[msg_recv.region], msg_recv.message);

				/**
				 *
				 *	THE PROPAGATION TO THE OTHERS CLIPBOARDS WILL
				 *  PROBABLY BE DONE HERE
				 *  
				 */
				if (fd_client != -1) {
					int nbytes = strlen(msg_recv.message);
					int nleft = nbytes;
					int nwritten = 0;
					
					while(nleft > 0){

						nwritten = write(fd_client, ptr, nleft);
						if(nwritten<=0)
							exit(1);
						nleft -= nwritten;
						ptr += nwritten;

					}
				}

			}

		} else {
			printf("Received a paste request for region %d\n", msg_recv.region);

			char answer[MAX_INPUT] = "";

			// Preventing access to non-existant regions
			if (msg_recv.region >= NUM_REG || msg_recv.region < 0) {
				strcpy(answer, "Region unavailable.");
			// Testing if region is empty
			} else if(regions[msg_recv.region][0] == '\0') {
				strcpy(answer, "No info available in requested region.");
			} else {
				strcpy(answer, regions[msg_recv.region]);
			}

			if(write(fd_connect, answer, sizeof(answer)) == -1) {
				printf("Error writing answer: %s\n", strerror(errno));
			}
		}

		memset(&msg_recv, 0, sizeof(msg_recv));
		memset(data, 0, strlen(data));
	}

	exit(0);
}
