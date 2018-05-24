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
/*char * getBuffer(int type, int region, char *message, int length) {

	struct Message message_struct;
	message_struct.type = type;
	strcpy(message_struct.message, message);
	message_struct.length = length;
	message_struct.region = region;
	char *msg = (char*)mymalloc(sizeof(message_struct)*sizeof(char));
	memcpy(msg, &message_struct, sizeof(message_struct));

	return msg;
}*/


void ctrl_c_callback_handler(int signum){
	printf("Caught signal Ctr-C\n");

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

// gets the message stored in the requested paste region
/*char* getPasteMessage(int region, char **regions) {
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
}*/

/*
* For getting the content of the server the clipboard connects to
* 
 */
void getBackup(int fd) {
	
	char request[] = "k";
	char information[15] = "";

	int region = -1;
	int len_message = -1;

	memset(information, '\0', sizeof(information));

	if( writeRoutine(fd, request, strlen(request)) == -1) {
		printf("Error writing in getBackup\n");
		return;
	}

	for (int i = 0; i < NUM_REG; i++) {

		// expected to receive "n region size", with a maximum of 15 characters
		readRoutine(fd, information, sizeof(information));
		// TODO ERROR TEST THIS

		// decodes the message of the information about the region
		if (sscanf(information, "n %d %d", &region, &len_message) != 2) {
			printf("sscanf didn't assign the variables correctly\n");

			// resets variables
			memset(information, '\0', sizeof(information));
			region = -1;
		 	len_message = -1;

			continue;
		}


		// DEBUG 
		if (region == i) {
			printf("%s\n", information);
		}
		// --

		// that specific region doesn't have content, so there's no content to be sent afterward
		if (len_message == 0) {
			continue;
		}

		// This guaranteed to be the first time of allocking memory for regions, so we can use malloc
		regions[i] = (char*) mymalloc(len_message*sizeof(char));

		readRoutine(fd, regions[i], len_message);
		// TODO ERROR TEST THIS

		regions_length[i] = len_message;

		// resets variables
		memset(information, '\0', sizeof(information));
		region = -1;
		len_message = -1;
	}

	return;
}

/* Routine that calls write until it sends the "length" bytes
 * Starts writing at the beggining of the buffer
 * and advances according to the number of bytes that was able to write
 * until it reaches the end of the buffer (size length) */
int writeRoutine(int fd, char *buffer, size_t length) {
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
int readRoutine(int fd, char *storageBuf, size_t length){
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

	unlink(SOCK_ADDRESS);

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

	// free memory of re	gions
	for (int i = 0; i < 10; ++i) {
	    free(regions[i]);
	}
	free(regions);

	// Freeing lists
	destroy(list_clips);
	destroy(list_apps);

	return;
}



/*
*
*
*/
void dealCopyRequests(int fd, char information[15]) {

	int region = -1;
	int len_message = -1;

	// decodes the message of the information about the region
	if (sscanf(information, "c %d %d", &region, &len_message) != 2) {
		printf("sscanf didn't assign the variables correctly\n");
		
		return;
	}

	char *receive = (char*)mymalloc(sizeof(char)*len_message);

	if(readRoutine(fd, receive, len_message) == 0) { 
		printf("client disconnected, read is 0\n");
		return;
	} 

	printf("%s\n", receive);

	// I'm the top clipboard then save in the clipboard and send to my child
	if (fd_parent == -1) {

		//clears the previous stored message, by all its length
		if(regions[region] != NULL) {
			memset(regions[region], '\0', regions_length[region]);
		}

		regions[region] = (char*)realloc(regions[region], len_message);

		memcpy(regions[region],receive,len_message);

		regions_length[region] = len_message;

	}

	// TODO MUTEX THIS

	return;
}



/*
*
*
*/
void dealPasteRequests(int fd, char information[15]) {

	int region = -1;
	int len_message = -1;

	// decodes the message of the information about the region
	if (sscanf(information, "p %d %d", &region, &len_message) != 2) {
		printf("sscanf didn't assign the variables correctly\n");
		
		return;
	}

	memset(information, '\0', 15);

	// case that the region doesnt have content
	if(regions[region] == NULL || regions[region][0] == '\0') {

		sprintf(information,"a %d %d", region, 0);

		// asks the clipboard to send a message of a certain size from a certain region
		if(writeRoutine(fd, information, 15) == -1) {
			// error writing
			printf("Error writing in dealPasteRequests\n");
			return;
		}

		return;
	} else {

		sprintf(information,"a %d %d", region, (int) regions_length[region]);

		// asks the clipboard to send a message of a certain size from a certain region
		if(writeRoutine(fd, information, 15) == -1) {
			// error writing
			printf("Error writing in dealPasteRequests\n");
			return;
		}

	}

	//case the app requests more bytes than the actual region size
	if(len_message > regions_length[region]) 
		len_message = regions_length[region];

	// sends the region's content 
	if(writeRoutine(fd, regions[region], len_message) == -1) {
		printf("Error writing in dealPasteRequests\n");
		return;
	}

	// TODO MUTEX THIS

	return;
}


/*
*
* Iterates through the lists of fds of children and send the information about the region
*
*/
int sendToChildren(char *message, int region, int len_message) {

	// first message to inform the clipboard of the message that is about to be sent
	char information[15] = "";

	// starts message as all \0
	memset(information, '\0', 15);

	sprintf(information,"m %d %d", region, len_message);

	// create an auxiliar pointer for iterating through list
	Node *aux = list_clips->head;

	while (aux != NULL) {

		// sets up the clipboard for be ready to receive a message of a certain size 
		// to insert inside a certain region
		if(writeRoutine(aux->fd, information, (size_t) sizeof(information)) == -1) {
			// error writing
			return -1;
		}

		// sends the info for the child clipboard to save in that region
		if(writeRoutine(aux->fd, message, len_message) == -1) {
			// error writing
			return -1;
		}

		// No need to clean information because the information sent will always be the same

		aux = aux->next;
	}


	// TODO MUTEXAR

	// send complete
	return 0;
}	


/*
*
* Send the information about the region to the Parent clipboard
*
* 
*/
int sendToParent(char *message, int region, int len_message) {

	if(fd_parent == -1) {
		printf("Can't sent to parent because I have none.\n");
		return -1;
	}

	// first message to inform the clipboard of the message that is about to be sent
	char information[15] = "";

	// starts message as all \0
	memset(information, '\0', 15);

	sprintf(information,"n %d %d", region, len_message);

	// sets up the parent clipboard for be ready to receive a message of a certain size 
	// to insert inside a certain region
	if(writeRoutine(fd_parent, information, (size_t) sizeof(information)) == -1) {
		// error writing
		return -1;
	}

	// sends the info for the parent clipboard to save (in case of being the single) or send to its child
	if(writeRoutine(fd_parent, message, len_message) == -1) {
		// error writing
		return -1;
	}

	// TODO MUTEXAR

	// send complete
	return 0;
}	