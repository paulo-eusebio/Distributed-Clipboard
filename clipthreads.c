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

	int n, nw;
	char *ptr, buffer[128] = "";

	printf("Started a thread to listen to this connection!\n");

	// Only reads for now

		while((n=read(fd,buffer,128))!=0){
			if(n==-1){
				exit(1);
			}
			ptr=&buffer[0];
			printf("Read: %s\n", buffer);
			while(n>0){
				if((nw=write(fd,ptr,n))<=0){
					exit(1);
				}
				n-=nw;
				ptr+=nw;

			}

		}

		// @TODO dar fclose do fd e eliminar da lista

		// @TODO ver se na thread das apps 

	// @TODO SE RECEBER ALGO DE UMA APP TENHO QUE PROPAGAR PARA TODOS OS CLIPBOARDS A QUE ESTOU CONECTADO
	// @TODO SE RECEBER UMA ATUALIZAÇÃO DE UM CLIPBOARD PRECISO DE PROPAGAR PARA TODOS OS OUTROS 
	// CLIPBOARDS ESTE O QUE ME ENVIOU

	return NULL;
}

//function to threads that deal with apps

void * thread_apps(void * data) {
	
	int fd = *(int*)data;
	
	char *message = (char*)mymalloc(sizeof(char)*sizeof(struct Message));
	
	if(readRoutine(fd, message, sizeof(struct Message)) == 0) { 
			printf("client disconnected, read is 0\n");
			exit(1);
	}
	
	printf("message received = %s\n", message);
	
	return NULL;
	
}

void * thread_stdin(void * data) {
	char bufstdin[10];
	char message[10];
	
	while(1) {
		fgets(bufstdin, MAX_INPUT, stdin);
		sscanf(bufstdin, "%[^\n]", message);
		printf("received stdin: %s\n", message);	
		if(strcmp(message,"exit")==0) 
			pthread_exit(NULL);		
	}
	
}


