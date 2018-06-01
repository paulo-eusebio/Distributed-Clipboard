#include "clipboard.h"

/*
* Aplicação teste para continuamente mensagens com um caracter random distanciados
* por um certo intervalo de segundos
* 	
* Funcionamento: correr a app ./app3 nº_seg_entre_msgs
*
*/
int main(int argc, char const *argv[]) {
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

		sleep(atoi(argv[1]));
	}
	
	close(sock_fd);
	exit(0);
}
