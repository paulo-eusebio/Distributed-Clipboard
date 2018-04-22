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

//gets the message stored in the requested paste region
char* getPasteMessage(int region, char **regions) {
	char *answer;
	// Preventing access to non-existant regions
	if (region >= NUM_REG || region < 0) {
		answer = (char*)mymalloc(strlen("Region unavailable."));
		strcpy(answer, "Region unavailable.");
	// Testing if region is empty
	} else if(regions[region][0] == '\0') {
		answer = (char*)mymalloc(strlen("No info available in requested region."));
		strcpy(answer, "No info available in requested region.");
	// There is a message stored, gets it
	} else { 
		answer = (char*)mymalloc(strlen(regions[region]));
		strcpy(answer, regions[region]);
	}
	return answer;
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

/* Routine that calls write until it sends the "length" bytes
 * Starts writing at the beggining of the buffer
 * and advances according to the number of bytes that was able to write
 * until it reaches the end of the buffer (size length) */
int writeRoutine(int fd, char *buffer, int length) {
	int nleft = length, nwritten = 0, total=0;
	char *ptr = buffer;
	while(nleft>0){
		nwritten=write(fd,ptr,nleft);		
		/*if(errno == EPIPE && nwritten == -1){ devemos de usar isto para quando um clip da discnect
			printf("servidor de mensagens disconectou\n");
			return -1;*/
		if(nwritten <= 0){
			printf("ocorreu um erro no write\n");
			exit(-1);
		}
		nleft -= nwritten;
		ptr += nwritten;
		total += nwritten;
	}
	return total;
}

/*Routine that calls read until it receives the "length" bytes
 * Reads always to readBuf and glues all the readings together
 * starting at the address pointed by storageBuf 
 * until it reads the all "length" that is supposed*/
int readRoutine(int fd, char *storageBuf, int length){
	int nstore=0,nread=0,nleft=length;
	char *readBuf = storageBuf;
	while(nleft>0){
		nread=read(fd,readBuf,nleft);
		nstore += nread;
		nleft -= nread;
		if(nread==-1){
			printf("Ocorreu um erro no read\n");
			exit(-1);
		}else if(nread==0){
			break;
		}
		readBuf += nread;
	}
	return nstore;
}

