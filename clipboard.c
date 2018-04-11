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

	char **regions = (char**) mymalloc(10*sizeof(char*));
	for (int i = 0; i < 10; ++i) {
	    regions[i] = (char *) mymalloc(STRINGSIZE+1);
	    memset(regions[i], 0, STRINGSIZE+1);
	}

	preparefifos(&fifo_in, &fifo_out);

	//abrir FIFOS
	struct Message msg_recv;
	char *data = (char*) mymalloc(sizeof(char)*sizeof(msg_recv));

	while (1) {
		printf(".\n");
		read(fifo_in, data, sizeof(msg_recv)); // we probably we will to do a loop here in the stream part

		memcpy(&msg_recv, data, sizeof(msg_recv));

		if (msg_recv.type == 0) {
			printf("received %s\n", msg_recv.message);
			printf("Saved message in region %d\n", msg_recv.region);

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
			printf("Received a paste request for region %d\n", msg_recv.region);


			// TESTAR SE EXISTE ALGUMA COISA NESTA REGIÃO

			char answer[MAX_INPUT] = "";
			strcpy(answer, regions[msg_recv.region]);

			if(write(fifo_out, answer, sizeof(answer)) == -1) {
				printf("Error writing answer: %s\n", strerror(errno));
			}
		}

		memset(&msg_recv, 0, sizeof(msg_recv));
		memset(data, 0, strlen(data));
	}

	exit(0);
}
