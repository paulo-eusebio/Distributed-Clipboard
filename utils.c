#include "utils.h"

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

	freeClipboard();

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
		answer = (char*)mymalloc(strlen("Region unavailable.")+1);
		strcpy(answer, "Region unavailable.");
	// Testing if region is empty
	} else if(regions[region][0] == '\0') {
		answer = (char*)mymalloc(strlen("No info available in requested region.")+1);
		strcpy(answer, "No info available in requested region.");
	// There is a message stored, gets it
	} else { 
		answer = (char*)mymalloc(strlen(regions[region])+1);
		strcpy(answer, regions[region]);
	}
	return answer;
}

/*
* For getting the content of the server the clipboard connects to
* 
 */
char** getBackup(int fd, char **regions) {
	
	struct Message msg;
	char *data = (char*)mymalloc(sizeof(char)*sizeof(struct Message));

	for (int region = 0; region < NUM_REG; region++) {

		// cleans buffer
		memset(data, '\0', strlen(data));
		
		// creats byte stream with the request to paste a message
		data = getBuffer(PASTE_REQUEST, region, "", sizeof(struct Message)); 
		// sends request to get region message
		if(writeRoutine(fd, data, sizeof(struct Message)) == -1)
			printf("Error writing at backup: %s\n", strerror(errno));
		memset(data, '\0', strlen(data));
		//waits for reply
		if(readRoutine(fd, data, sizeof(struct Message)) == -1)
			printf("Error reading reply at backup: %s\n", strerror(errno));
		memcpy(&msg, data, sizeof(struct Message));
		//  Only inserts the message if the region has any content
		if(strcmp(msg.message, "No info available in requested region.") != 0) {
			strcpy(regions[region], msg.message);
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


/// Random Number Generator with Range
int randGenerator(int min, int max) {
   return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}


/// deals with close file descriptors and freeing memory used
void freeClipboard() {

	// Closes all file descriptors in use
	if(list_clips->head != NULL) {
		Node *aux = list_clips->head;
		while(aux->next != NULL){
			if(close(aux->fd)) {
				perror("Closing TCP file descriptor");
			}
			
			aux = aux->next;
		}
		if(close(aux->fd)) {
			perror("Closing TCP file descriptor");
		}
	}

	if(list_apps->head != NULL) {
		Node *aux = list_apps->head;
		while(aux->next != NULL){
			if(close(aux->fd)) {
				perror("Closing UNIX file descriptor");
			}

			aux = aux->next;
		}
		if(close(aux->fd)) {
			perror("Closing UNIX file descriptor");
		}
	}

	// @TODO DAR SHUTDOWN DAS THREADS??????? <--- Dúvida

	// free memory of regions
	for (int i = 0; i < 10; ++i) {
	    free(regions[i]);
	}
	free(regions);

	// Freeing lists
	destroy(list_clips);
	destroy(list_apps);

	return;
}

