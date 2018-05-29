#include "clipthreads.h"

/// Thread that receives connections from local applications
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

	if(listen(fd_listen, 3) == -1) {
		perror("Error while listening: ");
		exit(-1);
	}

	while(1) {

		if( (fd_connect = accept(fd_listen, (struct sockaddr*) & client_addr, &size_addr)) == -1) {
			perror("Error accepting");
			break;
		}

		printf("Accepted a new app connection\n");
		
		// Threads Variables
		pthread_t thread_apps_id;

		// Save the fd gotten in the accept operation and thread id that's going to be launched for this connection

		// MUTEX LOCK - MUTEXAPPSLIST
		if( pthread_mutex_lock(&list_apps->list_mutex) != 0) {
			perror("Error locking a mutex of apps in thread_app_listen");
		}

		add(fd_connect, &thread_apps_id, NULL, list_apps);

		// MUTEX UNLOCK - MUTEXAPPSLIST
		if( pthread_mutex_unlock(&list_apps->list_mutex) != 0) {
			perror("Error unlocking a mutex of apps in thread_app_listen");
		}
		
		// thread to interact with the newly connected app
		pthread_create(&thread_apps_id, NULL, thread_apps, &fd_connect); 
	}
	
	close(fd_listen);

	pthread_exit(NULL);

	return NULL;
}

/// Thread for receiving connections from cooperative remote clipboards
void * thread_clips_listen(void * data) {

	// IPv4 Sockets declaration
	int fd = -1, newfd = -1;
	socklen_t addrlen;
	struct sockaddr_in addr;
	memset((void*)&addr,(int)'\0', sizeof(addr));
	struct in_addr temp_addr;
	memset((void*)&temp_addr,(int)'\0', sizeof(temp_addr));

	srand(time(NULL));

	if((fd = socket(AF_INET, SOCK_STREAM,0))==-1){
		perror("Error creating socket in clips listening thread");
		exit(1);
	}

	// converting the IP it wants to listen to the appropriate type
	temp_addr.s_addr=htonl(INADDR_ANY);

	// PORT must be randomly generated between 1024 and 64738
	int port = randGenerator(PORT_MIN, PORT_MAX);

	setSockaddrIP( &addr, &addrlen, &temp_addr, port);

	printf("Clipboard listening for connections through port: %d\n", port);

	if(bind(fd, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
		perror("bind");
		exit(-1);
	}

	if (listen(fd,3)==-1){
		exit(1);
	}

	addrlen=sizeof(addr);

	while(1) {

		if((newfd = accept(fd,(struct sockaddr*) &addr, &addrlen)) == -1){
			perror("Error accepting connections from other clipboards: ");
			exit(1);
		}

		printf("Accepted a connection\n");

		// Threads Variables
		pthread_t thread_clips_id;

		// Save the fd gotten in the accept operation and thread id that's going to be launched for this connection
		// MUTEX LOCK - MUTEXCLIPSLIST
		if( pthread_mutex_lock(&list_clips->list_mutex) != 0) {
			perror("Error locking a mutex of clips in thread_clips");
		}

		// MUTEX UNLOCK
		if( pthread_mutex_unlock(&list_clips->list_mutex) != 0) {
			perror("Error unlocking a mutex of clips in thread_clips");
		}
		
		// thread for reading and writing to the clipboard that this clipboard just accepted the connection
		pthread_create(&thread_clips_id, NULL, thread_clips, &newfd); 
	}

	close(fd);

	pthread_exit(NULL);

	return NULL;
}

/// Thread for dealing with each connected clipboard
void * thread_clips(void * data) {

	int fd = *(int*)data;
	char information[15] = "";

	// auxiliar variables
	int region = -1;
	int len_message = -1;
	int error_check = -1;
	// to close the fd in case of need
	int error_fd = -1;

	printf("Started a thread to listen to this connection!\n");

	while(1) {

		region = -1;
		len_message = -1;
		memset(information, '\0', 15);

		//para a este fd nao ha dois writes ou reads em threads diferentes em simultaneo
		//pode haver um read aqui e um write na thread ligada ao pai mas como é duplex n ha stress
		if((error_check = readRoutine(fd, information, sizeof(information))) == 0) { 
			printf("client disconnected in first readRoutine of thread_clips\n");
			break;
		}else if(error_check == -1) {
			printf("Error readRoutine in thread_clips.\n");
			break;
		}


		// Its a request of the type copy
		if (information[0] == 'k') {
			// sends content from all its regions
			sendBackup(fd);

			pthread_mutex_t mutex;

			if( pthread_mutex_init(&mutex, NULL) != 0) {
	    		perror("Error initiating mutex");
	  		}

	  		pthread_t myvalue = pthread_self();

			add(fd, &myvalue, &mutex, list_clips);
			
		// received a message from children
		} else if (information[0] == 'n') {

			if(sscanf(information, "n %d %d", &region, &len_message) != 2) {
				printf("sscanf didn't assign the variables correctly\n");

				region = -1;
				len_message = -1;
				memset(information, '\0', sizeof(information));

				continue;
			}

			// if i'm a single clipboard, don't have a parent
			if(fd_parent == -1) {
				// reads into the region

				// MUTEX - WRITELOCK
				pthread_rwlock_wrlock(&regions_rwlock[region]);

				if(regions[region] != NULL){
					// resets
					memset(regions[region], '\0', regions_length[region]);	
				}

				// saves region
				regions[region] = (char*) realloc(regions[region], len_message);
				memset(regions[region], '\0', len_message);

				if(readRoutine(fd, regions[region], len_message) == 0) { 
					printf("client disconnected in thread_clips, read is 0\n");

					// MUTEX UNLOCK - WRITELOCK
					pthread_rwlock_unlock(&regions_rwlock[region]);

					break;
				}

				regions_length[region] = len_message;

				// propagates the message to its children
				// OUTDATED MUTEX - READLOCK porque a app pode querer fazer um paste ao mesmo tempo
				if(sendToChildren(regions[region], region, len_message) == -1){
					printf("Error writing in thread_clips, sendToChildren\n");

					// TODO ver se é preciso dar continue e limpar as variaveis
				}

				// MUTEX UNLOCK - WRITELOCK
				pthread_rwlock_unlock(&regions_rwlock[region]);
				//wakes all threads waiting for this region to update
				if(pthread_cond_broadcast(&wait_regions[region]) != 0) {
					perror("error broadcasting conditional var\n");
				}
				
			// if i'm not a single clipboard, have a parent
			} else {
				// reads into an auxiliar variable

				// doesn't save anything, only send to parent

				char *aux_buffer = (char*) mymalloc(sizeof(char)*len_message);

				if(readRoutine(fd, aux_buffer, len_message) == 0) { 
					printf("client disconnected in thread_clips, read is 0\n");
					free(aux_buffer);
					break;
				}		

				// propagates the message to its parent
				if(sendToParent(aux_buffer, region, len_message) == -1){
					printf("Error writing in thread_clips, sendToParent\n");

					// TODO ver se é preciso dar continue e limpar as variaveis
					
				}

				free(aux_buffer);
			}

		// receive a message from parent
		} else if (information[0] == 'm') {

			if(sscanf(information, "m %d %d", &region, &len_message) != 2) {
				printf("sscanf didn't assign the variables correctly\n");

				region = -1;
				len_message = -1;
				memset(information, '\0', sizeof(information));

				continue;
			}

			// MUTEX - WRITELOCK
			pthread_rwlock_wrlock(&regions_rwlock[region]);

			if(regions[region] != NULL){
				// resets
				memset(regions[region], '\0', regions_length[region]);	
			}

			// saves region
			regions[region] = (char*) realloc(regions[region], len_message);
			memset(regions[region], '\0', len_message);

			if(readRoutine(fd, regions[region], len_message) == 0) { 
				printf("client disconnected in thread_clips, read is 0\n");

				// MUTEX UNLOCK - WRITELOCK
				pthread_rwlock_unlock(&regions_rwlock[region]);

				break;
			}

			regions_length[region] = len_message;

			// propagates the message to its children
			if((error_fd = sendToChildren(regions[region], region, len_message)) != -1){
				printf("Error writing in thread_clips\n");

				// TODO ver se é preciso dar continue e limpar as variaveis
			}

			// MUTEX UNLOCK - WRITELOCK
			pthread_rwlock_unlock(&regions_rwlock[region]);
			if(pthread_cond_broadcast(&wait_regions[region]) != 0) {
				perror("error broadcasting conditional var\n");
			}
			

		}
	}
	
	// depois de sair do ciclo while (quando a conexão morrer) verificar se o fd é o do pai ou não
	// caso seja o do pai meter fd_parent a -1, caso contrário remover da lista
	if (fd == fd_parent) {
		fd_parent = -1;
		if( pthread_mutex_destroy(&parent_socket_lock) != 0) {
			perror("Error while destroying fd parent");
		}
	} else {

		// MUTEX LOCK - MUTEXCPLISPLIST
		if( pthread_mutex_lock(&list_clips->list_mutex) != 0) {
			perror("Error locking a mutex of clips in thread_apps");
		}

		// for destroying the mutex of a node of type clips
		destroyNodeMutex(fd, list_clips);

		freeNode(fd, list_clips);

		// MUTEX UNLOCK 
		if( pthread_mutex_unlock(&list_clips->list_mutex) != 0) {
			perror("Error unlocking a mutex of clips in thread_apps");
		}
	}

	close(fd);

	pthread_detach(pthread_self());

	pthread_exit(NULL);

	return NULL;
}

//function to threads that deal with apps
void * thread_apps(void * data) {
	
	int fd = *(int*)data;
	
	// 15 because of a letter, space, digit, space, long int
	char information[15] = "";
	
	while(1) {

		if(readRoutine(fd, information, sizeof(information)) == 0) { 
			printf("app disconnected in first readRoutine of thread_apps, read is 0\n");
			break;
		} 

		// Its a request of the type copy
		if (information[0] == 'c') {

			dealCopyRequests(fd, information);

		// Its a request of the type paste
		} else if (information[0] == 'p') {

			dealPasteRequests(fd, information);

		// Its a request of the type wait
		} else if (information[0] == 'w') {

			dealWaitRequests(fd, information);

		// Its a warning that the app is going to close the connection
		} else if (information[0] == 's') {

			printf("An application just disconnected.\n");
			// gets out of the loop and closes the fd
			break;
		}

		memset(information, '\0', sizeof(information));
	}

	// FREEING

	// this fd is no longer connected, so remove it from the list
	// MUTEX LOCK - MUTEXAPPSLIST
	if( pthread_mutex_lock(&list_apps->list_mutex) != 0) {
		perror("Error locking a mutex of apps in thread_clips");
	}

	freeNode(fd, list_apps);

	// MUTEX UNLOCK - MUTEXAPPSLIST
	if( pthread_mutex_unlock(&list_apps->list_mutex) != 0) {
		perror("Error unlocking a mutex of apps in thread_clips");
	}

	// then close it
	close(fd);

	pthread_detach(pthread_self());

	pthread_exit(NULL);
	
	return NULL;
}


//threads that exclusively listens for stdin commands
void * thread_stdin(void * data) {
	char bufstdin[20];
	char message[20];
	
	while(1) {
		if(fgets(bufstdin, 20, stdin) == NULL) {
			printf("Nothing could be read\n");
			memset(bufstdin, '\0', strlen(bufstdin));	
			memset(message, '\0', strlen(message));
			continue;
		}
		sscanf(bufstdin, "%[^\n]", message);
		printf("received stdin: %s\n", message);	
		
		if(strcmp(message,"exit")==0) {

			pthread_exit(NULL);
		}

		if(strcmp(message,"print")==0) { //se tiver \0 no meio, prolly dont funciona

			for(int i = 0; i < 10; i++) {

				// MUTEX LOCK - READLOCK
				if( pthread_rwlock_rdlock(&regions_rwlock[i]) != 0) {
					perror("Error locking in thread_stdin");
					continue;
				}

				if(regions[i] != NULL) {
					printf("Tamanho %d, Region %d: %s\n", (int)regions_length[i], i, regions[i]);
				}

				// MUTEX UNLOCK
				if( pthread_rwlock_unlock(&regions_rwlock[i]) != 0) {
					perror("Error unlocking in thread_stdin");
					continue;
				}

			}

		}

		if(strcmp(message,"print apps")==0) {

			// MUTEX LOCK - MUTEXAPPSLIST
			if( pthread_mutex_lock(&list_apps->list_mutex) != 0) {
				perror("Error locking a mutex of apps in thread_stdin");
			}

			display(list_apps);

			// MUTEX UNLOCK - MUTEXAPPSLIST
			if( pthread_mutex_unlock(&list_apps->list_mutex) != 0) {
				perror("Error unlocking a mutex of apps in thread_stdin");
			}
		}

		if(strcmp(message,"print clips")==0) {

			// MUTEX LOCK - MUTEXCLIPSLIST

			if( pthread_mutex_lock(&list_clips->list_mutex) != 0) {
				perror("Error locking a mutex of clips in thread_stdin");
			}

			display(list_clips);

			// MUTEX UNLOCK - MUTEXCLIPSLIST
			if( pthread_mutex_unlock(&list_clips->list_mutex) != 0) {
				perror("Error unlocking a mutex of clips in thread_stdin");
			}
		}

		memset(bufstdin, '\0', strlen(bufstdin));	
		memset(message, '\0', strlen(message));
	}

	pthread_detach(pthread_self());

	pthread_exit(NULL);
}

//wait mandar msg ao clipboard e ficar à espera bloqueado no read???

