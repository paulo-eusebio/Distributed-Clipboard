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

#include "clipboard.h"
#include "utils.h"
#include "sock_stream.h"

int main() {


	/* Create Clipboard Regions*/
	char **regions = (char**) mymalloc(10*sizeof(char*));
	for (int i = 0; i < 10; ++i) {
	    regions[i] = (char *) mymalloc(STRINGSIZE+1);
	    memset(regions[i], 0, STRINGSIZE+1);
	}

	/* Sockets definition */
	int fd_listen = -1, fd_connect = -1;
	struct sockaddr_un local_addr;
	struct sockaddr_un client_addr;
	socklen_t size_addr = sizeof(client_addr);

	unlink(SOCK_ADDRESS);
	
	signal(SIGINT, ctrl_c_callback_handler);
	
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
				strcpy(regions[msg_recv.region], msg_recv.message);

				/**
				 *
				 *	THE PROPAGATION TO THE OTHERS CLIPBOARDS WILL
				 *  PROBABLY BE DONE HERE
				 *  
				 */
			}

		} else {
			printf("Received a paste request for region %d\n", msg_recv.region);


			// TESTAR SE EXISTE ALGUMA COISA NESTA REGIÃO

			char answer[MAX_INPUT] = "";
			strcpy(answer, regions[msg_recv.region]);

			if(write(fd_connect, answer, sizeof(answer)) == -1) {
				printf("Error writing answer: %s\n", strerror(errno));
			}
		}

		memset(&msg_recv, '\0', sizeof(msg_recv));
		memset(data, '\0', strlen(data));
	}

	exit(0);
}
