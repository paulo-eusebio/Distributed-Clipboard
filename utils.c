#include "utils.h"

void * mymalloc(int size){
	void * node = (void*)malloc(size);
	if(node == NULL){
		printf("Error in malloc:\n");
		exit(1);
	}
	return node;
}

void sigPipe() {
	void (*old_handler)(int);
		if( (old_handler = signal(SIGPIPE, SIG_IGN)) == SIG_ERR) {
			printf("Houve um conflito a ignorar os sinais SIGPIPEs\n");
			exit(1);
		}
}


void ctrl_c_callback_handler(int signum){
	printf("Caught signal Ctr-C\n");

	// Free resources
	pthread_detach(thread_app_listen_id);
	pthread_detach(thread_clip_id);
	pthread_detach(stdin_thread);

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


/*
* For getting the content of the server the clipboard connects to
* 
 */
void getBackup(int fd) {
	
	char request[15] = "k";
	char information[15] = "";

	int region = -1;
	int len_message = -1;
	int error_check = -2;

	memset(information, '\0', sizeof(information));

	if( writeRoutine(fd, request, sizeof(request)) == -1) {
		printf("Error writing in getBackup\n");
		return;
	}

	for (int i = 0; i < NUM_REG; i++) {

		// expected to receive "n region size", with a maximum of 15 characters
		if((error_check = readRoutine(fd, information, sizeof(information))) == 0) {
			printf("Client disconnected, ending getBackup\n");
			return;
		} else if(error_check == -1) {
			printf("Error in readRoutine of getBackup\n");
			return;
		}

		// decodes the message of the information about the region
		if (sscanf(information, "m %d %d", &region, &len_message) != 2) {
			printf("sscanf didn't assign the variables correctly\n");

			// resets variables
			memset(information, '\0', sizeof(information));
			region = -1;
		 	len_message = -1;

			continue;
		}

		// that specific region doesn't have content, so there's no content to be sent afterward
		if (len_message == 0) {
			memset(information, '\0', sizeof(information));
			region = -1;
			len_message = -1;
			continue;
		}

		// This guaranteed to be the first time of allocking memory for regions, so we can use malloc
		// MUTEX - Write lock por causa da thread stdin
		if(pthread_rwlock_wrlock(&regions_rwlock[i]) != 0) {
			perror("Error doing wrlock in getBackup\n");
		}


		// no need for broadcast because we dont have any connected app yet
		regions[i] = (char*) mymalloc(len_message*sizeof(char));

		if((error_check = (readRoutine(fd, regions[i], len_message))) == 0) {

			printf("Client disconnected, ending getBackup\n");
			regions_length[i] = len_message;

			// MUTEX UNLOCK
			if(pthread_rwlock_unlock(&regions_rwlock[i]) != 0) {
				perror("Error doing unlock in getBackup\n");
			}

			return;

		} else if(error_check == -1) {

			printf("Error in readRoutine of getBackup\n");
			regions_length[i] = len_message;

			// MUTEX UNLOCK
			if(pthread_rwlock_unlock(&regions_rwlock[i]) != 0) {
				perror("Error doing unlock in getBackup\n");
			}

			return;
		}

		regions_length[i] = len_message;

		// MUTEX UNLOCK
		if(pthread_rwlock_unlock(&regions_rwlock[i]) != 0) {
			perror("Error doing unlock in getBackup\n");
		}

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
			printf("Error occured in writeRoutine\n");
			return -1;
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
	char *readBuf = NULL;
	readBuf = storageBuf;
	while(nleft>0){
		nread=read(fd,readBuf,nleft);		
		nstore += nread;
		nleft -= nread;
		if(nread==-1){
			// error reading
			perror("readRoutine returned an error\n");
			return -1; 
		}else if(nread==0){
			// eof - disconnected
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

	pthread_cancel(thread_app_listen_id);
	pthread_cancel(thread_clip_id);
	pthread_cancel(stdin_thread);

	// free memory of re	gions
	for (int i = 0; i < 10; ++i) {
	    free(regions[i]);

	    if(pthread_rwlock_destroy(&regions_rwlock[i]) != 0){
	    	perror("Error while destroying a mutex of a region");
	    }
	   
	}
	free(regions);

	// destroying mutexes of lists
	if( pthread_mutex_destroy(&list_clips->list_mutex) != 0) {
		perror("Error while destroying mutex of clips list");
	}

	if( pthread_mutex_destroy(&list_apps->list_mutex) != 0) {
		perror("Error while destroying mutex of apps list");
	}
	
	if(fd_parent != -1) {
		close(fd_parent);
		if( pthread_mutex_destroy(&parent_socket_lock) != 0) {
			perror("Error while destroying fd parent");
		}
	}

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
	int error_check = -2;

	// decodes the message of the information about the region
	if (sscanf(information, "c %d %d", &region, &len_message) != 2) {
		printf("sscanf didn't assign the variables correctly\n");
		
		return;
	}

	char *receive = (char*)mymalloc(sizeof(char)*len_message);

	if((error_check =readRoutine(fd, receive, len_message)) == 0) { 
		printf("app disconnected in readRoutine of dealCopyRequests, read is 0\n");
		free(receive);
		return;
	} else if (error_check == -1) {
		printf("Read error in dealCopyRequests\n");
		free(receive);
		return;
	}

	// I'm the top clipboard then save in the clipboard and send to my child
	if (fd_parent == -1) {

		// MUTEX - WRITELOCK, porque outros podem querer ler a region em paralelo, mas só um pode escrever
		if(pthread_rwlock_wrlock(&regions_rwlock[region]) != 0){
			perror("Error doing wrlock in dealCopyRequests\n");
		}

		//clears the previous stored message, by all its length
		if(regions[region] != NULL) {
			memset(regions[region], '\0', regions_length[region]);
		}

		if( (regions[region] = (char*)realloc(regions[region], len_message)) == NULL){
			printf("Realloc failed, NULL was returned. dealCopyRequests\n");
		}

		memcpy(regions[region], receive, len_message);

		regions_length[region] = len_message;

		// propagates the message to its parent and doesn't save
		if(sendToChildren(regions[region], region, len_message) == -1){
			printf("Error writing in dealCopyRequests\n");
			free(receive);
			if(pthread_rwlock_unlock(&regions_rwlock[region]) != 0){
				perror("Error doing unlock in dealCopyRequests\n");
			}
			return;
		}

		// MUTEX Unlock
		if(pthread_rwlock_unlock(&regions_rwlock[region]) != 0){
			perror("Error doing unlock in dealCopyRequests\n");
		}

	} else {

		// don't save anything

		// propagates the message to its parent and doesn't save
		if(sendToParent(receive, region, len_message) == -1){
			printf("Error writing in dealCopyRequests\n");
			free(receive);
			return;
		}

	}

	free(receive);

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

	// MUTEX - READLOCK
	if(pthread_rwlock_rdlock(&regions_rwlock[region]) != 0) {
		perror("Error doing rdlock in dealPasteRequests\n");
	}

	// case that the region doesnt have content
	if(regions[region] == NULL || regions[region][0] == '\0') {

		sprintf(information,"a %d %d", region, 0);

		// asks the clipboard to send a message of a certain size from a certain region
		if(writeRoutine(fd, information, 15) == -1) {
			// error writing
			printf("Error writing in dealPasteRequests\n");

			// MUTEx UNLOCK
			if(pthread_rwlock_unlock(&regions_rwlock[region]) != 0) {
				perror("Error doing unlock in dealPasteRequests\n");
			}

			return;
		}

		// MUTEx UNLOCK
		if(pthread_rwlock_unlock(&regions_rwlock[region]) != 0) {
			perror("Error doing unlock in dealWaitRequests\n");
		}

		return;

	} else {

		sprintf(information,"a %d %d", region, (int) regions_length[region]);

		// asks the clipboard to send a message of a certain size from a certain region
		if(writeRoutine(fd, information, 15) == -1) {
			// error writing
			printf("Error writing in dealPasteRequests\n");

			// MUTEx UNLOCK
			if(pthread_rwlock_unlock(&regions_rwlock[region]) != 0) {
				perror("Error doing unlock in dealPasteRequests\n");
			}

			return;
		}

	}

	//case the app requests more bytes than the actual region size
	if(len_message > regions_length[region]) 
		len_message = regions_length[region];

	// no need to do a lock in this fd because an app can't send anything while waiting to receive the paste

	// sends the region's content 
	if(writeRoutine(fd, regions[region], len_message) == -1) {
		printf("Error writing in dealPasteRequests\n");

		// MUTEX UNLOCK
		if(pthread_rwlock_unlock(&regions_rwlock[region]) != 0) {
			perror("Error doing unlock in dealPasteRequests\n");
		}

		return;
	}

	// MUTEX UNLOCK
	if(pthread_rwlock_unlock(&regions_rwlock[region]) != 0) {
		perror("Error doing unlock in dealPasteRequests\n");
	}

	return;
}

void dealWaitRequests(int fd, char information[15]) {

	int region = -1;
	int len_message = -1;

	// decodes the message of the information about the region
	if (sscanf(information, "w %d %d", &region, &len_message) != 2) {
		printf("sscanf didn't assign the variables correctly\n");
		return;
	}

	if(pthread_mutex_lock(&region_waits[region]) != 0) {
		perror("error rdlock in dealWaitRequests\n");
	}

	if(pthread_cond_wait(&wait_regions[region], &region_waits[region]) != 0) {
		perror("error wait cond var in dealWaitRequests\n");
	}
	
	if(pthread_mutex_unlock(&region_waits[region]) != 0) {
		perror("error rdlock in dealWaitRequests\n");
	}
	
	if(pthread_rwlock_rdlock(&regions_rwlock[region]) != 0) {
		perror("error rdlock in dealWaitRequests\n");
	}
	//region's content has changed
	
	if(regions[region] == NULL || regions[region][0] == '\0') {

		sprintf(information,"a %d %d", region, 0);

		// asks the clipboard to send a message of a certain size from a certain region
		if(writeRoutine(fd, information, 15) == -1) {
			// error writing
			printf("Error writing in dealWaitRequests\n");

			// MUTEx UNLOCK
			pthread_rwlock_unlock(&regions_rwlock[region]);

			return;
		}

		// MUTEx UNLOCK
		if(pthread_rwlock_unlock(&regions_rwlock[region]) != 0) {
			perror("Error doing unlock in dealWaitRequests\n");
		}

		return;

	} else {

		sprintf(information,"a %d %d", region, (int) regions_length[region]);

		// asks the clipboard to send a message of a certain size from a certain region
		if(writeRoutine(fd, information, 15) == -1) {
			// error writing
			printf("Error writing in dealWaitRequests\n");

			// MUTEx UNLOCK
			if(pthread_rwlock_unlock(&regions_rwlock[region]) != 0) {
				perror("Error doing unlock in dealWaitRequests\n");
			}

			return;
		}

	}

	//case the app requests more bytes than the actual region size
	if(len_message > regions_length[region]) 
		len_message = regions_length[region];

	// no need to do a lock in this fd because an app can't send anything while waiting to receive the paste

	// sends the region's content 
	if(writeRoutine(fd, regions[region], len_message) == -1) {
		printf("Error writing in dealWaitRequests\n");

		// MUTEX UNLOCK
		if(pthread_rwlock_unlock(&regions_rwlock[region]) != 0) {
			perror("Error doing unlock in dealWaitRequests\n");
		}

		return;
	}

	// MUTEX UNLOCK
	if(pthread_rwlock_unlock(&regions_rwlock[region])!= 0) {
		perror("error unlock in dealWaitRequests\n");
	}

	return;
}


/*
*
* Iterates through the lists of fds of children and send the information about the region
*
* returns -1 if the operations were correct and the fd if not
*/
int sendToChildren(char *message, int region, int len_message) {

	// first message to inform the clipboard of the message that is about to be sent
	char information[15] = "";

	// starts message as all \0
	memset(information, '\0', 15);

	sprintf(information,"m %d %d", region, len_message);

	// create an auxiliar pointer for iterating through list
	// MUTEX while iterating linkedlist of clipbards
	if( pthread_mutex_lock(&list_clips->list_mutex) != 0) {
		perror("Error locking a mutex of clips in sendToChildren");
	}
		
	Node *aux = list_clips->head;
	
	while (aux != NULL) {

		// MUTEX - LOCK do fd
		if( pthread_mutex_lock(aux->mutex) != 0) {
			perror("Error locking a mutex in sendToChildren");
		}
		
		// sets up the clipboard for be ready to receive a message of a certain size 
		// to insert inside a certain region
		if(writeRoutine(aux->fd, information, (size_t) sizeof(information)) == -1) {
			// error writing
			printf("Error in writeRoutine of sendToChildren\n");
			if( pthread_mutex_unlock(aux->mutex) != 0) {
				perror("Error unlocking a mutex in sendToChildren");
			}

			if( pthread_mutex_unlock(&list_clips->list_mutex) != 0) {
				perror("Error unlocking a mutex of clips in sendToChildren");
			}

			// if there was an error while writing to a guy then no need to write again
			aux = aux->next;
			continue;
		}

		// sends the info for the child clipboard to save in that region
		if(writeRoutine(aux->fd, message, len_message) == -1) {
			// error writing
			printf("Error in writeRoutine of sendToChildren\n");
			if( pthread_mutex_unlock(aux->mutex) != 0) {
				perror("Error unlocking a mutex in sendToChildren");
			}

			if( pthread_mutex_unlock(&list_clips->list_mutex) != 0) {
				perror("Error unlocking a mutex of clips in sendToChildren");
			}

			aux = aux->next;
			continue;
		}

		// MUTEX UNLOCK - LOCK do fd <---- Desnecessário
		if( pthread_mutex_unlock(aux->mutex) != 0) {
			perror("Error unlocking a mutex in sendToChildren");
		}


		// No need to clean information because the information sent will always be the same

		aux = aux->next;
	}
	
	// MUTEX UNLOCK
	if( pthread_mutex_unlock(&list_clips->list_mutex) != 0) {
		perror("Error unlocking a mutex of clips in sendToChildren");
	}

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
	
	//MUTEX LOCK socket para fd parent
	
	if( pthread_mutex_lock(&parent_socket_lock) != 0) {
		perror("Error locking a mutex in send parent");
	}

	// sets up the parent clipboard for be ready to receive a message of a certain size 
	// to insert inside a certain region
	if(writeRoutine(fd_parent, information, (size_t) sizeof(information)) == -1) {
		// error writing
		printf("Error in writeRoutine of sendToParent\n");
		if( pthread_mutex_unlock(&parent_socket_lock) != 0) {
			perror("Error unlocking a mutex in send parent");
		}
		return -1;
	}
	
	// sends the info for the parent clipboard to save (in case of being the single) or send to its child
	if(writeRoutine(fd_parent, message, len_message) == -1) {
		// error writing
		printf("Error in writeRoutine of sendToParent\n");
		if( pthread_mutex_unlock(&parent_socket_lock) != 0) {
			perror("Error unlocking a mutex in send parent");
		}
		return -1;
	}

	// MUTEX UNLOCK
	if( pthread_mutex_unlock(&parent_socket_lock) != 0) {
		perror("Error unlocking a mutex in send parent");
	}

	// send complete
	return 0;
}

void sendBackup(int fd) {
	
	char information[15] = "";
	
	memset(information, '\0', sizeof(information));


	for(int i = 0; i < NUM_REG; i++) {

		// MUTEX - READLOCK
		if(pthread_rwlock_rdlock(&regions_rwlock[i]) != 0) {
			perror("Error rdlocking in sendBackup");
		}

		sprintf(information, "m %d %d", i, (int)regions_length[i]);

		// não damos unlock aqui porque não queremos arriscar enviar uma região de tamanho diferente
		// ao combinado na mensagem de aviso

		if(writeRoutine(fd, information, sizeof(information)) == -1) {
			printf("Error returned in writeRoutine of sendBackup\n");
			
			if(pthread_rwlock_unlock(&regions_rwlock[i]) != 0){
				perror("Error unlocking rwlock in sendBackup");
			}

			return;
		}

		if(regions_length[i] != 0) {
			if(writeRoutine(fd, regions[i], regions_length[i]) == -1 ) {
				printf("Error returned in writeRoutine of sendBackup\n");
				
				if(pthread_rwlock_unlock(&regions_rwlock[i]) != 0){
					perror("Error unlocking rwlock in sendBackup");
				}
				return;
			}
		}

		// UNLOCK - READLOCK
		if(pthread_rwlock_unlock(&regions_rwlock[i]) != 0){
			perror("Error unlocking rwlock in sendBackup");
		}
		
		memset(information, '\0', sizeof(information));
	}


	return;
}


