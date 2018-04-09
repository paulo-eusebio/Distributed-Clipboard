#include "clipboard.h"
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main()
{
	struct Message message_sent;
	struct Message received;

	char dado = 'a';

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

	// 
	char *msg = malloc(sizeof(message_sent)*sizeof(char));
	memcpy(msg, &message_sent, sizeof(message_sent));

	int bytes_copied = clipboard_copy(fd, region, msg, sizeof(message_sent));

	printf("Copied %d bytes", bytes_copied);

	//scanf("%c\n", &dado);

	// testar retorno

	// int bytes_read = clipboard_paste(fd, region, &message, sizeof(message));

	// printf("Received %d\n", dados_int);

	exit(0);
}
