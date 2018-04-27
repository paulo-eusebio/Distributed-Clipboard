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
#include <netinet/in.h>

#include "clipboard.h"
#include "utils.h"
#include "sock_stream.h"

/**
 * PROVISORIO, apenas tem uma string hardcoded que envia para outro
 * e recebe replicações doutro clipboard
 * 
 * gcc -g -std=c99 remote.c utils.c -o remote
 **/

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
	strcpy(regions[2], "ja ca tava eheh");

	/* Sockets definition */
	struct sockaddr_in local_addr;
	struct sockaddr_in client_addr;
	socklen_t size_addr;

	int sock_fd= socket(AF_INET, SOCK_STREAM, 0);
	
	if (sock_fd == -1){
		perror("socket: ");
		exit(-1);
	}
	
	local_addr.sin_family = AF_INET;
	local_addr.sin_port= htons(3010);
	inet_aton("192.168.1.4", &local_addr.sin_addr);

	int err = bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr));
	if(err == -1) {
		perror("bind");
		exit(-1);
	}
	
	listen(sock_fd, 5);

	int fd_client= accept(sock_fd, (struct sockaddr *) & client_addr, &size_addr);
	struct Message msg_recv;
	char *data = (char*)mymalloc(sizeof(char)*sizeof(struct Message));
	
	
	//Relevant stuff starts here!!!
	while (1) {

		printf(".\n");

		if(readRoutine(fd_client, data, sizeof(msg_recv)) == 0) { 
			printf("client disconnected, read is 0\n");
			exit(1);
		}
		memcpy(&msg_recv, data, sizeof(msg_recv));
		memset(data, '\0', strlen(data)); //cleans buffer for re-use

		if (msg_recv.type == REPLICATE) { //REPLICATE
			// for guarantee that the region request is a valid region
			if (msg_recv.region >= 0 && msg_recv.region < 10){
				strcpy(regions[msg_recv.region], msg_recv.message);
				//message propagation to other clipboard
			}
			printf("Updated cliboard:\n\n");
			for(int i=0; i<NUM_REG;i++)
				printf("\t %d - %s\n", i, regions[i]);
				
		} else { //PASTE
			printf("Received a paste request for region %d\n", msg_recv.region);
			//gets message stored in requested region
			char* answer = getPasteMessage(msg_recv.region, regions); 
			//creates stream of bytes with message
			data = getBuffer(PASTE_REPLY, msg_recv.region, answer, sizeof(struct Message)); 
			if(writeRoutine(fd_client, data, sizeof(struct Message)) == -1) {
				printf("Error writing answer: %s\n", strerror(errno));
			}
		}
		

		memset(&msg_recv, '\0', sizeof(msg_recv));
		memset(data, '\0', strlen(data));
	}

	exit(0);
}
