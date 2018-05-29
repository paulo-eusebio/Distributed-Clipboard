#include "clipboard.h"

// where the magic happen
int main(int argc, char const *argv[]) {

	sigPipe();

	// initialization at -1 (parentless)
	fd_parent = -1;

	// checks if the clipboard is single or connected
	int clip_mode = checkMode(argc);

	if (clip_mode == -1) {
		printf("Invalid input:\n - Single Mode: No arguments\n - Connected Mode: -c IP port\n");
		exit(-1);
	} else if (clip_mode == 0) {
		printf("Executing clipboard in single mode...\n");
	} else if (clip_mode == 1) {
		printf("Executing clipboard in connected mode...\n");
	}

	// Create Clipboard Regions 
	regions = (char**) mymalloc(10*sizeof(char*));
	for (int i = 0; i < 10; ++i) {
		// the size of the regions are initializad a null because it doesnt have a msg associated yet
	    regions[i] = NULL;
	    regions_length[i] = 0;
	    if(pthread_rwlock_init(&regions_rwlock[i], NULL) != 0) {
	    	printf("Inilization of rwlock wasn't succesful\n");
	    }
	    if(pthread_cond_init(&wait_regions[i], NULL) != 0) {
	    	printf("Inilization of conditional vars wasn't succesful\n");
	    } 
	    if(pthread_mutex_init(&region_waits[i], NULL) != 0) {
	    	printf("Inilization of conditional vars wasn't succesful\n");
	    }
	}

	// initializing lists of file descriptors and threads
	list_clips = emptylist();
	list_apps = emptylist();

	unlink(SOCK_ADDRESS);

	signal(SIGINT, ctrl_c_callback_handler);

	pthread_create(&stdin_thread, NULL, thread_stdin, NULL); 

	// in the case of the clipboard being type connected then with needs to connect
	// to the received IP and Port and request its region
	if(clip_mode == CONNECTED) {
		getClipboardBackUp(argv);
	}

	// thread for listening to apps that want to connect to the clipboard
	pthread_create(&thread_app_listen_id, NULL, thread_app_listen, regions);
	
	// thread for listening to another clipboards that want to connect to this clipboard
	pthread_create(&thread_clip_id, NULL, thread_clips_listen, NULL); 

	//pthread_join(thread_clip_id, NULL);
	pthread_join(stdin_thread, NULL);
	printf("Turning off\n");
	freeClipboard();

	// Free resources
	pthread_detach(thread_app_listen_id);
	pthread_detach(thread_clip_id);
	pthread_detach(stdin_thread);
	printf("Bye\n");
	return(0);
}



void getClipboardBackUp(char const *argv[]) {

	int fd_client = -1;
	struct sockaddr_in ipv4_client;
	struct in_addr temp_addr;
	socklen_t addrlen = 0;

	pthread_t thread_id;

	// create clipboard server socket
	if( (fd_client = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Error creating client socket");
		exit(1);
	}

	memset((void*)&temp_addr,(int)'\0', sizeof(temp_addr));
	// converting the IP arg to the appropriate type
	inet_aton(argv[2], &temp_addr);

	// setting up the Socket
	setSockaddrIP(&ipv4_client, &addrlen, &temp_addr, (unsigned short) atoi(argv[3]));

	// connect to the server clipboard
	if( connect(fd_client, (struct sockaddr*) &ipv4_client, sizeof(ipv4_client)) == -1){
		perror("Error while connecting to another clipboard");
		exit(-1);
	}

	// no need to start the thread before the backup because new msgs will arrive later
 
	fd_parent = fd_client;

	// initializion of the parent mutex
	if (pthread_mutex_init(&parent_socket_lock, NULL) != 0) {
		printf("Inilization of rwlock wasn't succesful\n");
	}

	// fills the regions with the content from a connected clipboard
	getBackup(fd_client);

	// Thread for listening to reads from this file descriptor
	pthread_create(&thread_id, NULL, thread_clips, &fd_client); 

	return;
}