#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "clipboard.h"
#include "utils.h"

int main() {

	int fifo_in, fifo_out = -1;	
	int bytes_read = -1;

	char **regions = (char**) mymalloc(10*sizeof(char*));
	for (int i = 0; i < 10; ++i) {
	    regions[i] = (char *) malloc(STRINGSIZE+1);
	    memset(regions[i], 0, STRINGSIZE+1);
	}

	preparefifos(&fifo_in, &fifo_out);

	//abrir FIFOS
	struct Message msg_recv;
	struct Message msg_send;
	char *data = malloc(sizeof(char)*sizeof(msg_recv));

	while (1) {
		printf(".\n");
		bytes_read=read(fifo_in, data, sizeof(msg_recv)); // we probably we will to do a loop here in the stream part
		if(bytes_read == -1) {
			printf("Error reading from pipe\n");
			exit(-1);
		}
		
		memcpy(&msg_recv, data, sizeof(msg_recv));

		if (msg_recv.type == 0) {
			printf("received %s\n", msg_recv.message);

			// for guarantee that the region request is a valid region
			if (msg_recv.region >= 0 && msg_recv.region < 10){
				strcpy(regions[msg_recv.region], msg_recv.message);

				/**
				 *
				 *	THE PROPAGATION TO THE OTHERS CLIPBOARDS WILL
				 *  PROBABLY DONE HERE
				 *  
				 */
			}

		} else {
			msg_send.type = 1;
			strcpy(msg_send.message, regions[msg_recv.region]);
			msg_send.length = strlen(regions[msg_recv.region]);
			msg_send.region = msg_recv.region;

			char *msg = malloc(sizeof(msg_send)*sizeof(char));
			memcpy(msg, &msg_send, sizeof(msg_send));

			if(write(fifo_out, msg,sizeof(msg_send)) == -1) {
				printf("Error writing answer: %s\n", strerror(errno));
			}

			free(msg);
		}


		memset(data, 0, strlen(data));
	}

	exit(0);
}
