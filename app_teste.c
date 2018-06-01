#include "clipboard.h"


/*
* Aplicação teste para enviar mensagens string de qualquer tamanho.
* 	
* ./appteste
*
* Funcionamento: 
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

	while(1) {
		printf("\nPress:\n\t1 to Copy\n\t2 to Paste\n\t3 to Wait\n\t4 to Close\n");
		
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
			printf("Type a message: ");
			fgets(message, MAX_INPUT, stdin); //nao usar scanf para sacar strings!! 
			strtok(message, "\n"); //removes \n, just to be prettier
			
			clipboard_copy(clipboard_id, region, message, strlen(message));
		} else if (action == 2) { //paste
			printf("How many bytes: ");
			fgets(message,MAX_INPUT,stdin);
			sscanf(message, "%d", &many);
			
			char *recv_buf = (char*)mymalloc(sizeof(char)*many);
			
			clipboard_paste(clipboard_id, region, recv_buf, (size_t)many); 
			printf("\nReceived  message '%s'\n", recv_buf);
		} else if (action == 3) {
			printf("How many bytes: ");
			fgets(message,MAX_INPUT,stdin);
			sscanf(message, "%d", &many);
			
			char *recv_buf = (char*)mymalloc(sizeof(char)*many);
			
			clipboard_wait(clipboard_id, region, recv_buf, (size_t)many); 
			printf("\nReceived  message '%s'\n", recv_buf);
		} else {
			printf("Wrong Command\n");
		}
		memset(message, '\0', strlen(message));
	}

	exit(0);
}
