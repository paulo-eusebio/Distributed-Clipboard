#include "clipboard.h"


/************************************************
 * 
 * Conecta aplicação ao clipboard por AF UNIX
 * com base no ficheiro definido em sock_stream.h
 * 
 ************************************************/

int clipboard_connect(char *clipboard_dir){
	
	struct sockaddr_un server_addr;
	int sock_fd = -1;
	
	//creates socket
	if( (sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket: ");
		exit(-1);
	}
	
	//defines socket structure
	server_addr.sun_family = AF_UNIX;
	strcpy(server_addr.sun_path, clipboard_dir);

	//tries to connect to clipboard
	int err_c = connect(sock_fd, (const struct sockaddr *) &server_addr, sizeof(server_addr));
	if(err_c==-1){
		printf("Error connecting\n");
		exit(-1);
	}

	printf("connected %d\n", err_c);

	return sock_fd;
}

/**********************************************************************
 * 
 * Protocolo copy:
 * 
 * Envia uma mensagem para o clipboard com informação acerca do pedido
 * "tipo regiao tamanho", tipo "c" corresponde a um copy para a regiao
 * pedida com o tamanho da mensagem
 * 
 * De seguida envia o conteudo da mensagem
 * 
 * Return: nº bytes copiados, 0 em erro
 * *********************************************************************/


int clipboard_copy(int clipboard_id, int region, void *buf, size_t count) {

	if (region < 0 || region > 9) {
		printf("Trying to copy to an invalid region.\n");
		return 0;
	}

	if (clipboard_id < 0) {
		printf("Trying to copy to an invalid fd.\n");
		return 0;
	}

	char message[15] = "";

	// bytes sent
	int total = 0;

	// starts message as all \0
	memset(message, '\0', 15);

	sprintf(message,"c %d %d", region, (int) count);

	// sets up the clipboard for be ready to receive a message of a certain size 
	// to insert inside a certain region
	if(writeRoutine(clipboard_id, message, (size_t) sizeof(message)) == -1) {
		// error writing
		perror("Error in writeRoutine of clipboard_copy");
		return 0;
	}

	// sends the info to copy to the region
	if((total = writeRoutine(clipboard_id, buf, count)) == -1) {
		// error writing
		perror("Error in writeRoutine of clipboard_copy");
		return 0;
	}

	return total;
}

/**********************************************************************
 * 
 * Protocolo paste:
 * 
 * Envia uma mensagem para o clipboard com informação acerca do pedido
 * "tipo regiao tamanho", tipo "p" corresponde a um paste para a regiao
 * pedida com o numero de bytes requirido
 * 
 * De seguida bloqueia À espera de uma resposta, com informação da mensagem
 * que virá, caso seja de tamanho 0, sabe que a região está vazia, caso
 * contrário prepara para ler o numero de bytes respondido, que será 
 * o diferente do pedido, caso a mensagem tenha tamanho real inferior ao
 * pedido
 * 
 * Return: nº bytes recebidos, 0 em erro
 * *********************************************************************/


int clipboard_paste(int clipboard_id, int region, void *buf, size_t count) {
	
	if (region < 0 || region > 9) {
		printf("Trying to paste from an invalid region.\n");
		return 0;
	}

	if (clipboard_id < 0) {
		printf("Trying to paste from an invalid fd.\n");
		return 0;
	}

	char message[15] = "";

	// bytes sent
	int total = 0;

	int error_check = -2;

	// starts message as all \0
	memset(message, '\0', 15);

	sprintf(message,"p %d %d", region, (int) count);

	// asks the clipboard to send a message of a certain size from a certain region
	if(writeRoutine(clipboard_id, message, sizeof(message)) == -1) {
		// error writing
		perror("Error in writeRoutine of clipboard_paste");
		return 0;
	}

	// starts message as all \0
	memset(message, '\0', 15);

	if( (error_check = readRoutine(clipboard_id, message, sizeof(message))) == -1) {
		// error reading
		perror("Error in readRoutine of clipboard_paste");
		return 0;
	} else if(error_check == 0) {
		printf("Client disconnected. clipboard_paste\n");
		return 0;
	}

	int reg = -1;
	int len_message = -1;

	// decodes the message of the information about the region
	if (sscanf(message, "a %d %d", &reg, &len_message) != 2) {
		printf("sscanf didn't assign the variables correctly: clipboard_paste\n");
		return 0;
	}

	// No content in the region
	if(len_message == 0)
		return 0;

	// if the content available is less than what exists in the region
	if(len_message < count) {

		if( (total = readRoutine(clipboard_id, buf, len_message)) == -1) {
			// error reading
			perror("Error in readRoutine of clipboard_paste");
			return 0;
		}

	} else {

		if( (total = readRoutine(clipboard_id, buf, count)) == -1) {
			// error reading
			perror("Error in readRoutine of clipboard_paste");
			return 0;
		}
	}

	return total;
}

/***************************************************************
 * Envia mensagem ao clipboard a avisar que irá encerrar conexão
 * 
 * *************************************************************/
void clipboard_close(int clipboard_id) {

	char message[15] = "s";

	// warns the clipboard that the app is going to disconnect
	if(writeRoutine(clipboard_id, message, sizeof(message)) == -1) {
		// error writing
		perror("Error in writeRoutine of clipboard_close");
		return;
	}

	if(close(clipboard_id) == -1) {
		printf("Error closing connection: %s\n", strerror(errno));
	}

	return;
}

/**********************************************************************
 * 
 * Protocolo wait:
 * 
 * Envia uma mensagem para o clipboard com informação acerca do pedido
 * "tipo regiao tamanho", tipo "w" corresponde a um wait para a regiao
 * pedida com o numero de bytes requirido
 * 
 * De seguida bloqueia À espera de uma resposta, com informação da mensagem
 * que virá e bloqueia novamente para ler a mensagem até ao 
 * numero de bytes respondido, assim que o clipboard receber uma atualização
 * 
 * Return: nº bytes recebidos, 0 em erro
 * *********************************************************************/



int clipboard_wait(int clipboard_id, int region, void *buf, size_t count) {

	if (region < 0 || region > 9) {
		printf("Trying to paste from an invalid region.\n");
		return 0;
	}

	if (clipboard_id < 0) {
		printf("Trying to paste from an invalid fd.\n");
		return 0;
	}

	char message[15] = "";

	// bytes sent
	int total = 0;

	int error_check = -2;

	// starts message as all \0
	memset(message, '\0', 15);

	sprintf(message,"w %d %d", region, (int) count);

	// asks the clipboard to send a message of a certain size from a certain region
	if(writeRoutine(clipboard_id, message, sizeof(message)) == -1) {
		// error writing
		perror("Error in writeRoutine of clipboard_wait");
		return 0;
	}

	// starts message as all \0
	memset(message, '\0', 15);

	if( (error_check = readRoutine(clipboard_id, message, sizeof(message))) == -1) {
		// error reading
		perror("Error in readRoutine of clipboard_wait");
		return 0;
	} else if(error_check == 0) {
		printf("Clipboard disconnected in clipboard_wait.\n");
		return 0;
	}

	int reg = -1;
	int len_message = -1;

	// decodes the message of the information about the region
	if (sscanf(message, "a %d %d", &reg, &len_message) != 2) {
		printf("sscanf didn't assign the variables correctly: clipboard_paste\n");
		return 0;
	}

	// No content in the region
	if(len_message == 0)
		return 0;

	// if the content available is less than what exists in the region
	if(len_message < count) {

		if( (total = readRoutine(clipboard_id, buf, len_message)) == -1) {
			// error reading
			perror("Error in readRoutine of clipboard_wait");
			return 0;
		} else if(total == 0) {
			printf("Client disconnected in clipboard_wait\n");
			return 0;
		}

	} else {

		if( (total = readRoutine(clipboard_id, buf, count)) == -1) {
			// error reading
			perror("Error in readRoutine of clipboard_wait");
			return 0;
		} else if(total == 0) {
			printf("Client disconnected in clipboard_wait\n");
			return 0;
		}
	}

	return total;
}
