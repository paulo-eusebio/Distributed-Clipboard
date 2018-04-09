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

	//struct Message regions[10];

	preparefifos(&fifo_in, &fifo_out);

	//abrir FIFOS
	int len_data;
	struct Message msg_recv;
	char *data = malloc(sizeof(char)*sizeof(msg_recv));

	while (1) {
		printf(".\n");
		read(fifo_in, data, sizeof(msg_recv));
		memcpy(&msg_recv, data, sizeof(msg_recv));
		printf("received %s\n", msg_recv.message);
		len_data = strlen(data);

		printf("sending value %d - length %d\n", len_data, (int) sizeof(len_data));
		//write(fifo_out, &len_data, sizeof(len_data));
	}

	// Deste lado temos que fazer
	// memcpy(struct, msg)

	exit(0);
}
