#include "clipboard.h"

/*
* Aplicação teste para enviar ficheiros de qualquer tamanho.
* 	
* Funcionamento: Meter o ficheiro que se quer enviar no directory
* do projeto e para correr no terminal ./appfiles name_of_file.
* 
* Durante a execução da aplicação é possível escolher para que região
* se quer enviar o ficheiro e quantos bytes. Da mesma forma é possivel pedir
* um certo nº de bytes de uma região. Esses bytes lidos serão inseridos num ficheiro 
* chamado output e com o extensão do ficheiro recebido no argumento.
* Permite também fazer o wait de uma região.
*
*/

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

	file = fopen(argv[1], "r");
	
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

			if(clipboard_copy(clipboard_id, region, sendbuf, result) == 0){
				fclose(file);
				close(clipboard_id);
			}

		} else if (action == 2) { //paste
			printf("How many bytes: ");
			fgets(message,MAX_INPUT,stdin);
			sscanf(message, "%d", &many);

			printf("value of many: %d\n", many);
			
			char *recv_buf = (char*)mymalloc(sizeof(char)*many);

			memset(recv_buf, '\0', many);

			if(clipboard_paste(clipboard_id, region, recv_buf, (size_t)many) == 0) {
				free(recv_buf);
				fclose(file);
				close(clipboard_id);
			}

			printf("Image saved in file'\n");

			FILE *outputfile;
			char outputname[11] = "output";
			outputfile = fopen(outputname, "w");
			fwrite(recv_buf, 1, many, outputfile);
			fclose(outputfile);
			
			free(recv_buf);
		} else if (action == 3) {
			printf("How many bytes: ");
			fgets(message,MAX_INPUT,stdin);
			sscanf(message, "%d", &many);
			
			char *recv_buf = (char*)mymalloc(sizeof(char)*many);

			if(clipboard_wait(clipboard_id, region, recv_buf, (size_t)many) == 0) {
				free(recv_buf);
				fclose(file);
				close(clipboard_id);
			}

			printf("Image saved in file'\n");

			FILE *outputfile;
			char outputname[11] = "output.";
			strcat(outputname, argv[2]);
			outputfile = fopen(outputname, "w");
			fwrite(recv_buf, 1, many, outputfile);
			fclose(outputfile);
		} else {
			printf("Wrong Command\n");
		}
		memset(message, '\0', strlen(message));
	}

	fclose(file);

	exit(0);
}
