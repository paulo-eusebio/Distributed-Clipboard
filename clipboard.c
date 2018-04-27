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

#include "clipboard.h"
#include "utils.h"
#include "sock_stream.h"

void * thread_app_listen(void * data){

	// UNIX sockets
	int fd_listen = -1, fd_connect = -1;
	struct sockaddr_un local_addr, client_addr;
	socklen_t size_addr = sizeof(client_addr);

	if((fd_listen = socket(AF_UNIX, SOCK_STREAM,0))== -1) {
		perror("Opening socket: ");
		exit(-1);
	}

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

	while(1) {

		if( (fd_connect = accept(fd_listen, (struct sockaddr*) & client_addr, &size_addr)) == -1) {
			perror("Error accepting");
			exit(-1);
		}

		// declare thread
		// create node
		// add node to list
		// create thread and send the node

	}


	return NULL;
}

// matrix for our regions
char **regions;

// stack for save the file descriptors of apps
struct Node* root_apps = NULL;

// stack for save the file descriptors of clips
struct Node* root_clips = NULL;

// where the magic happen
int main(int argc, char const *argv[]) {


	int clip_mode = checkMode(argc);

	if (clip_mode == -1) {
		printf("Invalid input:\n - Single Mode: No arguments\n - Connected Mode: -c IP port\n");
		exit(-1);
	}

	// Create Clipboard Regions 
	regions = (char**) mymalloc(10*sizeof(char*));
	for (int i = 0; i < 10; ++i) {
	    regions[i] = (char *) mymalloc(STRINGSIZE+1);
	    memset(regions[i], 0, STRINGSIZE+1);
	}

	unlink(SOCK_ADDRESS);

	signal(SIGINT, ctrl_c_callback_handler);


	pthread_t thread_listen_id;


	// fazer thread para ligar a clipboards

	// thread for listening to apps that want to connect to the clipboard
	pthread_create(&thread_listen_id, NULL, thread_app_listen, regions);



	return(0);
}

	/*

	// Sockets definition

	// UNIX Sockets
	int fd_listen = -1, fd_connect = -1;
	struct sockaddr_un local_addr;
	struct sockaddr_un client_addr;
	socklen_t size_addr = sizeof(client_addr);

	// IPV4 Sockets
	int fd_client = -1, fd_server;
	struct sockaddr_in ipv4_client;
	struct sockaddr_in ipv4_server;
	socklen_t addrlen = 0;

	unlink(SOCK_ADDRESS);
	
	signal(SIGINT, ctrl_c_callback_handler);

	//check if the clipboard is in single or connected mode
	if (clip_mode == SINGLE) {

		// Nothing to be done?

	} else if (clip_mode == CONNECTED) {

		struct in_addr temp_addr;

		// create clipboard server socket
		if( (fd_client = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			perror("Error creating client socket");
			exit(1);
		}

		memset((void*)&temp_addr,(int)'\0', sizeof(temp_addr));
		// converting the IP arg to the appropriate type
		inet_aton(argv[2], &temp_addr);

		// setting up the Socket
		setSockaddrIP(&ipv4_client, &addrlen, &temp_addr, (unsigned short) atoi(argv[3]));

		// connect to the server clipboard
		if( connect(fd_client, (struct sockaddr*) &ipv4_client, sizeof(ipv4_client)) == -1){
			perror("Error while connecting to another clipboard");
		}

		// fills the regions with the content from a connected clipboard
		regions = getBackup(fd_client, regions);
		printf("Updated cliboard from backup:\n\n");
			for(int i=0; i<NUM_REG;i++)
				printf("\t %d - %s\n", i, regions[i]);
	}

	// UNIX SOCKETS COMMUNICATION 
	
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
	char *data = (char*)mymalloc(sizeof(char)*sizeof(struct Message));
	
	//Relevant stuff starts here!!!
	while (1) {

		printf(".\n");

		if(readRoutine(fd_connect, data, sizeof(msg_recv)) == 0) { 
			printf("client disconnected, read is 0\n");
			exit(1);
		}
		memcpy(&msg_recv, data, sizeof(msg_recv));
		memset(data, '\0', strlen(data)); //cleans buffer for re-use

		if (msg_recv.type == COPY) { //COPY
			// for guarantee that the region request is a valid region
			if (msg_recv.region >= 0 && msg_recv.region < 10){
				strcpy(regions[msg_recv.region], msg_recv.message);
				//message propagation to other clipboard
				if (fd_client != -1) {
					data = getBuffer(REPLICATE, msg_recv.region, msg_recv.message, sizeof(struct Message));
					if(writeRoutine(fd_client, data, sizeof(struct Message)) == -1) {
						printf("Error replicating message: %s\n", strerror(errno));
					}		
				}
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
			if(writeRoutine(fd_connect, data, sizeof(struct Message)) == -1) {
				printf("Error writing answer: %s\n", strerror(errno));
			}
		}

		memset(&msg_recv, '\0', sizeof(msg_recv));
		memset(data, '\0', strlen(data));
	
	}*/