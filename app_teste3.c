#include "clipboard.h"
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "utils.h"

int main(){
	int copyData;
	char dados[2];

	dados[1] = '\0';

	// Connects to the cliboard
	int sock_fd = clipboard_connect(SOCK_ADDRESS);
	if(sock_fd == -1){
		exit(-1);
	}
	while(1) {
		dados[0] = rand()%(122-65)+65;

		// Sends the data to the cliboard server
		copyData = clipboard_copy(sock_fd, 0, dados, 2);
		if(copyData < 1) {
			printf("Error on copy\n");
		}
		else {
			//printf("Sent %d - data: %s\n", copyData, dados);
		}

		//sleep(5);
	}
	
	close(sock_fd);
	exit(0);
}
