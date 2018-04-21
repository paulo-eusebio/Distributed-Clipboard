#include "utils.h"

/// 
void preparefifos(int *fifo_in, int*fifo_out) {

	char file_name_inboud[100];
	char file_name_outbound[100];

	// unlink fifos
	sprintf(file_name_inboud, "./%s", INBOUND_FIFO);
	unlink(file_name_inboud);
	sprintf(file_name_outbound, "./%s", OUTBOUND_FIFO);
	unlink(file_name_outbound);

	// create fifos
	if (mkfifo(file_name_inboud, 0666) == -1) {
		printf("Error mkfifo inboud: %s\n", strerror(errno));
		exit(-1);
	}

	if (mkfifo(file_name_outbound, 0666) == -1) {
		printf("Error mkfifo outboud: %s\n", strerror(errno));
		exit(-1);
	}

	// open fifos
	if ((*fifo_in = open(file_name_inboud, O_RDONLY)) == -1) {
		printf("Error open inboud: %s\n", strerror(errno));
		exit(-1);
	}

	if ((*fifo_out = open(file_name_outbound, O_WRONLY)) == -1) {
		printf("Error open outboud: %s\n", strerror(errno));
		exit(-1);
	}
}

void * mymalloc(int size){
	void * node = (void*)malloc(size);
	if(node == NULL){
		printf("erro malloc:\n");
		exit(1);
	}
	return node;
}


/*
* @brief Turns the parameters into a struct and copies its memory into
* a string
* 
 */
char * getBuffer(int type, int region, char *message, int length) {

	struct Message message_struct;
	message_struct.type = type;
	strcpy(message_struct.message, message);
	message_struct.length = length;
	message_struct.region = region;
	char *msg = (char*)mymalloc(sizeof(message_struct)*sizeof(char));
	memcpy(msg, &message_struct, sizeof(message_struct));

	return msg;
}


void ctrl_c_callback_handler(int signum){
	printf("Caught signal Ctr-C\n");
	unlink(SOCK_ADDRESS);
	exit(0);
}

/*
* Checks the clipboard mode 
*/
int checkMode(int argc) {

	// single mode
	if(argc == 1) {
		return 0;
	// connected mode
	} else if (argc == 4) {
		return 1;
	// invalid input
	} else {
		return -1;
	}
}


/*
* Setup IPV4 socket
* Initializing sockaddr_in
*/
void setSockaddrIP( struct sockaddr_in * server, socklen_t *addrlen, struct in_addr * addr, unsigned short port){

	struct in_addr temp = (*addr);
	temp.s_addr = addr->s_addr;

	memset((void *)server, '\0', sizeof(server));
	server->sin_addr.s_addr = temp.s_addr;
	server->sin_port = htons(port);
	server->sin_family = AF_INET;
	addrlen = (socklen_t *)sizeof(*server);

	return;
}


/*
* For getting the content of the server the clipboard connects to
* 
 */
char** getBackup(int fd, char **regions) {

	char message[MAX_INPUT] = "";

	for (int region = 0; region < NUM_REG; region++) {

		// cleans buffer
		memset(message, '\0', strlen(message));

		// gets data from current region
		clipboard_paste(fd, region, message, sizeof(message));

		//  Only inserts the message if the region has any content
		if(strcmp(message, "No info available in requested region.") != 0) {
			strcpy(regions[region], message);
		} 

	}

	return regions;
}