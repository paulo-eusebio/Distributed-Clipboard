#include "clipthreads.h"

/************************************************************
 *															* 
 * Thread que está à espera de conexões de aplicações 		*
 * via AF_UNIX, assim que uma conexão é feita lança uma		*
 * thread para lidar especificamente com a aplicação aceite	*
 * e volta a esperar nova conexão							*
 * 															*
 * Também adiciona a nova aplicação a uma lista com todas 	*
 * as aplicações ligadas a este clipboard					*
 * 															*
 * Args: Não utilizado										*
 * **********************************************************/
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
		
		// Received new connection, creates thread to deal with new app
		pthread_t thread_apps_id;

		// MUTEX LOCK - MUTEXAPPSLIST
		if( pthread_mutex_lock(&list_apps->list_mutex) != 0) {
			perror("Error locking a mutex of apps in thread_app_listen");
		}

		// Save the fd gotten in the accept operation and thread id that's going to be launched for this connection
		add(fd_connect, &thread_apps_id, NULL, list_apps);

		// MUTEX UNLOCK - MUTEXAPPSLIST
		if( pthread_mutex_unlock(&list_apps->list_mutex) != 0) {
			perror("Error unlocking a mutex of apps in thread_app_listen");
		}
		
		int *i = (int*)malloc(sizeof(fd_connect));
		*i = fd_connect;
		
		// thread to interact with the newly connected app
		if(pthread_create(&thread_apps_id, NULL, thread_apps, i) != 0) {
			perror("Error creating a thread app listen");
		}
			
	}
	
	close(fd_listen);

	pthread_exit(NULL);

	return NULL;
}

/************************************************************
 * 															*
 * Thread que está à espera de conexões de clipboards 		*
 * via AF:INET, assim que uma conexão é feita lança uma		*
 * thread para lidar especificamente com o novo clipboard	*
 * e volta a esperar nova conexão							*
 * 															*
 * Args: Não utilizado										*
 * **********************************************************/


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
		
		//waits for a new clipboard connection
		if((newfd = accept(fd,(struct sockaddr*) &addr, &addrlen)) == -1){
			perror("Error accepting connections from other clipboards: ");
			exit(1);
		}

		printf("Accepted a connection\n");

		// Threads Variables
		pthread_t thread_clips_id;
		
		int *i = (int*)malloc(sizeof(newfd));
		*i = newfd;

		// thread for reading and writing to the clipboard that this clipboard just accepted the connection
		if(pthread_create(&thread_clips_id, NULL, thread_clips, i) != 0) {
			perror("Error creating a thread clip listen");
		}
	}

	close(fd);

	pthread_exit(NULL);

	return NULL;
}


/****************************************************************
 * 																*
 * Thread lançada para lidar com um clipboard conectado a		*
 * este clipboard												*
 * 																*
 * O protocolo consiste em mensagem com o formato 				*
 * "label região tamanho", que é guardado na string information	*
 * 																*
 * Caso a label seja k, os restantes campos são ignorados		*
 * significa que o clipboard recentemente						*
 * contectado pediu toda a informação das regiões. De seguida	*
 * envia essa informação e adiciona o clipboard à lista de clips*
 * conectados													*
 * 																*
 * Caso a label seja n, é uma replicação de mensagem vinda de um*	
 * filho, significa que a proxima mensagem dessa socket será a 	*
 * mensagem a propagar com o tamanho e região informados. 		*
 * Depois de receber  a mensagem propaga para o pai caso tenha,	*
 * se não tiver pai, guarda e propaga para todos os filhos		*
 * 																*
 * Caso a label seja m, é uma replicação vinda do pai, guardar	*
 * a mensagem que virá e replica para todos os clipboards filhos*
 * 
 * Sempre que uma mensagem é efectivamente guardada na região,	*
 * broadcasta um signal para todos as threads À espera de uma 	*
 * atulização dessa região										*
 * 
 * Args: fd da conexão a esse clipboard							*
 * **************************************************************/

void * thread_clips(void * data) {

	//file discriptor
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
		
		//clears vars
		region = -1;
		len_message = -1;
		memset(information, '\0', 15);
		
		//para a este fd nao ha dois writes ou reads em threads diferentes em simultaneo
		//pode haver um read aqui e um write na thread ligada ao pai mas como é duplex n ha stress
		
		//first message received is the length and the region where to save the region
		if((error_check = readRoutine(fd, information, sizeof(information))) == 0) { 
			printf("client disconnected in first readRoutine of thread_clips\n");
			break;
		}else if(error_check == -1) {
			printf("Error readRoutine in thread_clips.\n");
			break;
		}


		// A connected clipboard requested the database content
		if (information[0] == 'k') {
			// sends content from all its regions
			sendBackup(fd);

			pthread_mutex_t mutex;

			if( pthread_mutex_init(&mutex, NULL) != 0) {
	    		perror("Error initiating mutex");
	  		}

	  		pthread_t myvalue = pthread_self();
			
			//only adds clipboard info to the list after updating him
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

			// if i'm a single clipboard, don't have a parent, save message
			if(fd_parent == -1) {

				// MUTEX - WRITELOCK
				pthread_rwlock_wrlock(&regions_rwlock[region]);

				if(regions[region] != NULL){
					// resets
					memset(regions[region], '\0', regions_length[region]);	
				}

				// allocs according to the info received message before
				regions[region] = (char*) realloc(regions[region], len_message);
				memset(regions[region], '\0', len_message);
				
				//reads directly into the region
				if(readRoutine(fd, regions[region], len_message) == 0) { 
					printf("client disconnected in thread_clips, read is 0\n");

					// MUTEX UNLOCK - WRITELOCK
					pthread_rwlock_unlock(&regions_rwlock[region]);

					break;
				}
				
				regions_length[region] = len_message;

				// propagates the message to its children
				if(sendToChildren(regions[region], region, len_message) == -1){
					printf("Error writing in thread_clips, sendToChildren\n");

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
				
				// allocs according to the first message informatin
				char *aux_buffer = (char*) mymalloc(sizeof(char)*len_message);

				if(readRoutine(fd, aux_buffer, len_message) == 0) { 
					printf("client disconnected in thread_clips, read is 0\n");
					free(aux_buffer);
					break;
				}		

				// propagates the message to its parent
				if(sendToParent(aux_buffer, region, len_message) == -1){
					printf("Error writing in thread_clips, sendToParent\n");
				}

				free(aux_buffer);
			}

		// receive a message from parent
		} else if (information[0] == 'm') {
			
			//retrieves information about the incoming message
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
			if((error_fd = sendToChildren(regions[region], region, len_message)) != 0){
				printf("Error writing in thread_clips\n");
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

	if(pthread_detach(pthread_self()) != 0) {
		perror("Error deatching thread in thread_clips");
	}
		

	pthread_exit(NULL);

	return NULL;
}

/***********************************************************
 * 
 * Thread lançada para lidar com uma app conectada a
 * este clipboard
 * 
 * O protocolo consiste em mensagem com o formato 
 * "label região tamanho", que é guardado na string information
 * 
 * Para cada label é invocada a função correspondente que 
 * lidará com o pedido em questão, recebendo  como argumento
 * a string information e o file discriptor, excepto para 
 * a label do tipo 's' que significa fecho de conexão, aí
 * a thread é encerrada.
 * 
 * Args: fd da conexão a esse clipboard
 * ********************************************************/



void * thread_apps(void * data) {
	
	int fd = *(int*)data;
	
	// 15 because of a letter, space, digit, space, long int
	char information[15] = "";
	
	while(1) {
		//first message is an information about the incoming message
		if(readRoutine(fd, information, sizeof(information)) == 0) { 
			printf("app disconnected in first readRoutine of thread_apps.\n");
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

	if(pthread_detach(pthread_self()) != 0) {
		perror("Error detaching thread in thread_apsp");
	}

	pthread_exit(NULL);
	
	return NULL;
}


/***********************************************************
 * 
 * Thread lançada para lidar com comandos do stdin input
 * 
 * Aceita: 	
 * 		"exit" -> termina o programa
 *		"print" -> imprime o conteudo de todas as regioes
 * 		"print apps" -> imprime info de todas as apps ligadas 
 * 					ao clipboard
 * 		"print clips" -> imprime info de todos os clipboards 
 * 					ligados a este clipboard
 * 
 * Args: Não utilizado
 * ********************************************************/
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
				} else {
					printf("Tamanho %d, Region %d:\n", (int)regions_length[i], i);
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
	if(pthread_detach(pthread_self()) != 0) {
		perror("Error detaching stdin thread");
	}

	pthread_exit(NULL);
}
