#include "clipboard.h"
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "utils.h"

int main(int argc, char const *argv[]) {

	char bufstdin[MAX_INPUT] = "";
	char message[MAX_INPUT] = "";
	int region = 0;

	int fd = clipboard_connect("./");
	if (fd == -1){
		printf("Error opening the FIFO files\n");
		exit(-1);
	}

	// Getting Input
	printf("Insert message:\n");
	fgets(bufstdin, MAX_INPUT, stdin);
	sscanf(bufstdin, "%[^\n]", message);
	printf("Insert Region to save:\n");
	memset(bufstdin, 0, strlen(bufstdin));
	fgets(bufstdin, MAX_INPUT, stdin);
	sscanf(bufstdin, "%d", &region);

	int bytes_copied = clipboard_copy(fd, region, message, sizeof(message));

	printf("Copied %d bytes\n", bytes_copied);

	memset(message, 0, strlen(message));

	int bytes_read = clipboard_paste(fd, region, message, sizeof(message)); // <-- este 0 é uma possível fonte de erro

	printf("Received %d, and the message '%s'\n", bytes_read, message);

	exit(0);
}
