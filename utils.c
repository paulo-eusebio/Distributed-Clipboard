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