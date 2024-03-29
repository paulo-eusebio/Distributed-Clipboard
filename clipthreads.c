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

	if(listen(fd_listen, 2) == -1) {
		perror("Error while listening: ");
		exit(-1);
	}

	while(1) {

		if( (fd_connect = accept(fd_listen, (struct sockaddr*) & client_addr, &size_addr)) == -1) {
			perror("Error accepting");
			exit(-1);
		}

		printf("Accepted a new app connection\n");
		
		// Threads Variables
		pthread_t thread_apps_id;

		// Save the fd gotten in the accept operation and thread id that's going to be launched for this connection
		add(fd_connect, thread_apps_id, list_apps);
		
		// thread to interact with the newly connected app
		pthread_create(&thread_apps_id, NULL, thread_apps, &fd_connect); 
	}
	
	close(fd_connect);


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

	if (listen(fd,5)==-1){
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
		add(newfd, thread_clips_id, list_clips);
		
		// thread for reading and writing to the clipboard that this clipboard just accepted the connection
		pthread_create(&thread_clips_id, NULL, thread_clips, &newfd); 
	}

	close(fd);

	return NULL;
}

/// Thread for dealing with each connected clipboard
void * thread_clips(void * data) {

	int fd = *(int*)data;
	char information[15] = "";

	// auxiliar variables
	int region = -1;
	int len_message = -1;

	printf("Started a thread to listen to this connection!\n");

	memset(information, '\0', sizeof(information));

	while(1) {

		if(readRoutine(fd, information, sizeof(information)) == 0) { 
			printf("client disconnected, read is 0\n");
			break;
		}
		// Its a request of the type copy
		if (information[0] == 'k') {
			sendBackup(fd);

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

				if(regions[region] != NULL){
					// resets
					memset(regions[region], '\0', regions_length[region]);	
				}

				// saves region
				regions[region] = (char*) realloc(regions[region], len_message);

				if(readRoutine(fd, regions[region], len_message) == 0) { 
					printf("client disconnected, read is 0\n");
					break;
				}

				regions_length[region] = len_message;

				// propagates the message to its children
				if(sendToChildren(regions[region], region, len_message) == -1){
					printf("Error writing in thread_clips, sendToChildren\n");

					// TODO ver se é preciso dar continue e limpar as variaveis
				}
				
			// if i'm not a single clipboard, have a parent
			} else {
				// reads into an auxiliar variable

				// doesn't save anything, only send to parent

				char *aux_buffer = (char*) mymalloc(sizeof(char)*len_message);

				if(readRoutine(fd, aux_buffer, len_message) == 0) { 
					printf("client disconnected, read is 0\n");
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
			//TODO

			if(sscanf(information, "m %d %d", &region, &len_message) != 2) {
				printf("sscanf didn't assign the variables correctly\n");

				region = -1;
				len_message = -1;
				memset(information, '\0', sizeof(information));

				continue;
			}

			if(regions[region] != NULL){
				// resets
				memset(regions[region], '\0', regions_length[region]);	
			}

			// saves region
			regions[region] = (char*) realloc(regions[region], len_message);

			if(readRoutine(fd, regions[region], len_message) == 0) { 
				printf("client disconnected, read is 0\n");
				break;
			}

			regions_length[region] = len_message;

			// propagates the message to its children
			if(sendToChildren(regions[region], region, len_message) == -1){
				printf("Error writing in thread_clips\n");

				// TODO ver se é preciso dar continue e limpar as variaveis
			}


		}

		// ver qual o tipo da mensagem

		// se k -> pedido BackUp
		// invocar função para dar write de todas as suas regiões check

		// se n -> ver se é single ou connected
		// se single, guardar e enviar para os filhos check
		// se connected, enviar para o pai check

		// se m -> atualização vinda do pai
		// atualizar a região check
		// enviar para os filhos check
		// conditional variable para o wait <-------

		region = -1;
		len_message = -1;
		memset(information, '\0', sizeof(information));
	}
	
	// depois de sair do ciclo while (quando a conexão morrer) verificar se o fd é o do pai ou não
	// caso seja o do pai meter fd_parent a -1, caso contrário remover da lista
	if (fd == fd_parent) {
		fd_parent = -1;
	} else {
		freeNode(fd, list_clips);
	}

	close(fd);

	return NULL;
}

//function to threads that deal with apps
void * thread_apps(void * data) {
	
	int fd = *(int*)data;
	
	// 15 because of a letter, space, digit, space, long int
	char information[15] = "";
	
	while(1) {

		if(readRoutine(fd, information, sizeof(information)) == 0) { 
			printf("client disconnected, read is 0\n");
			break;
		} 

		printf("first message: %s\n", information);

		// Its a request of the type copy
		if (information[0] == 'c') {

			dealCopyRequests(fd, information);

		// Its a request of the type paste
		} else if (information[0] == 'p') {

			dealPasteRequests(fd, information);

		// Its a request of the type wait
		} else if (information[0] == 'w') {

			// TODO

		}

		memset(information, '\0', sizeof(information));
	}

	// this fd is no longer connected, so remove it from the list
	freeNode(fd, list_apps);

	// then close it
	close(fd);
	
	return NULL;
}


//threads that exclusively listens for stdin commands
void * thread_stdin(void * data) {
	char bufstdin[10];
	char message[10];
	
	while(1) {
		fgets(bufstdin, MAX_INPUT, stdin);
		sscanf(bufstdin, "%[^\n]", message);
		printf("received stdin: %s\n", message);	
		
		if(strcmp(message,"exit")==0) 
			pthread_exit(NULL);

		if(strcmp(message,"print")==0) { //se tiver \0 no meio, prolly dont funciona
			for(int i = 0; i < 10; i++) {
				if(regions[i] != NULL) {
					printf("Tamanho %d, Region %d: %s\n", (int)regions_length[i], i, regions[i]);
				}
			}
		}

		memset(bufstdin, '\0', strlen(bufstdin));	
		memset(message, '\0', strlen(message));
	}
	
}

//wait mandar msg ao clipboard e ficar à espera bloqueado no read???

