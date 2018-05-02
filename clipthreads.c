#include "clipthreads.h"

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

		// declare thread
		// create node
		// add node to list
		// create thread and send the node

	}


	return NULL;
}

void * thread_clips_listen(void * data) {

	// IPv4 Sockets declaration
	int fd = -1, newfd = -1;
	socklen_t addrlen;
	struct sockaddr_in addr;
	memset((void*)&addr,(int)'\0', sizeof(addr));
	struct in_addr temp_addr;
	memset((void*)&temp_addr,(int)'\0', sizeof(temp_addr));

	// Threads Variables
	pthread_t thread_clips_id;

	if((fd = socket(AF_INET, SOCK_STREAM,0))==-1){
		perror("Error creating socket in clips listening thread");
		exit(1);
	}

	// converting the IP it wants to listen to the appropriate type
	temp_addr.s_addr=htonl(INADDR_ANY);

	setSockaddrIP( &addr, &addrlen, &temp_addr, LISTENING_CLIPS_PORT);

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

		// enviar nwfd para a lista de newfds
		
		// lançar a nova thread e enviar o newfd nos argumentos

		// thread for reading and writing to the clipboard that this clipboard just accepted the connection
		pthread_create(&thread_clips_id, NULL, thread_clips, &newfd); 
	}

	close(fd);

	return NULL;
}

void * thread_clips(void * data) {

	int fd = *(int*)data;

	int n, nw;
	char *ptr, buffer[128];

	// Only reads for now
	while(1) {

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

	}

	return NULL;
}
