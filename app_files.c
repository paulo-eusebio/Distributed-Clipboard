#include "clipboard.h"
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "utils.h"

int main(int argc, char const *argv[]) {

	char message[MAX_INPUT] = "";
	int region = 0;
	int action;
	int many;

	int clipboard_id = clipboard_connect(SOCK_ADDRESS);
	if (clipboard_id == -1){
		printf("Error opening the FIFO files\n");
		exit(-1);
	}

	//Get Picture Size
	printf("Getting File Size\n");
	FILE *file = NULL;

	if(strcmp(argv[1],"image") == 0) {
		file = fopen(argv[2], "rb");
	} else if(strcmp(argv[1],"file") == 0) {
		file = fopen(argv[2], "r");
	}
	
	int filesize;
	fseek(file, 0, SEEK_END);
	filesize = ftell(file);
	rewind(file);

	char *sendbuf = (char*) malloc (sizeof(char)*filesize);
	
	size_t result = fread(sendbuf, 1, filesize, file);

	printf("File has a sizeof: %d\n",(int) result);

	if (result <= 0) {
		printf("Error fread\n");
		exit(-1);
	}

	while(1) {
		printf("\nPress:\n\t1 to Copy\n\t2 to Paste\n\t3 to Wait\n\t4 to close\n");
		
		fgets(message,MAX_INPUT,stdin);
		sscanf(message, "%d", &action);
		
		if (action == 4) {
			clipboard_close(clipboard_id);
			break;
		}
		
		printf("Region: ");
		fgets(message,MAX_INPUT,stdin);
		sscanf(message, "%d", &region);
		
		if(action == 1) { //copy
			printf("File going to be sent");
			/*fgets(message, MAX_INPUT, stdin); //nao usar scanf para sacar strings!! 
			strtok(message, "\n"); //removes \n, just to be prettier*/
			
			if(clipboard_copy(clipboard_id, region, sendbuf, result) == 0){
				fclose(file);
				close(clipboard_id);
			}

		} else if (action == 2) { //paste
			printf("How many bytes: ");
			fgets(message,MAX_INPUT,stdin);
			sscanf(message, "%d", &many);
			
			char *recv_buf = (char*)mymalloc(sizeof(char)*many);

			if(clipboard_paste(clipboard_id, region, recv_buf, (size_t)many) == 0) {
				free(recv_buf);
				fclose(file);
				close(clipboard_id);
			}

			if(strcmp(argv[1],"image") == 0) {
				printf("Image saved in file'\n");

				FILE *image;
				image = fopen("output.jpg", "w");
				fwrite(recv_buf, 1, many, image);
				fclose(image);
			} else {
				printf("\nReceived  message '%s'\n", recv_buf);
			}
			
			free(recv_buf);
		} else if (action == 3) {
			//TODO
		} else {
			printf("Wrong Command\n");
		}
		memset(message, '\0', strlen(message));
	}

	fclose(file);

	exit(0);
}
