#include "clipboard.h"
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main()
{
	struct Message message_sent;

	message_sent.type = 0;
	strcpy(message_sent.message, "Hello World\n");
	message_sent.length = strlen(message_sent.message);
	message_sent.region = 0;
	int region = 0;

	int fd = clipboard_connect("./");
	if (fd == -1){
		printf("Error opening the FIFO files\n");
		exit(-1);
	}

	printf(" Message info: type=%d ; message='%s' ; length='%d' ; region='%d'\n", message_sent.type, message_sent.message, message_sent.length, message_sent.region);

	char *msg = malloc(sizeof(message_sent)*sizeof(char));
	memcpy(msg, &message_sent, sizeof(message_sent));

	int bytes_copied = clipboard_copy(fd, region, msg, sizeof(message_sent));

	printf("Copied %d bytes", bytes_copied);

	free(msg);

	struct Message message_recv;
	message_recv.type = 1;
	memset(message_recv.message, 0, strlen(message_recv.message));
	message_recv.length = 0;
	message_recv.region = 0;
	region = 0;

	msg = malloc(sizeof(message_recv)*sizeof(char));
	memcpy(msg, &message_recv, sizeof(message_recv));

	int bytes_read = clipboard_paste(fd, region, msg, sizeof(message_recv));
	memcpy(&message_recv, msg, sizeof(message_recv));

	printf("Received %d, and the message '%s'\n", bytes_read, message_recv.message);

	free(msg);

	exit(0);
}
