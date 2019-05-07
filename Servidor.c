#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netdb.h>
#include <arpa/inet.h> 
#include <unistd.h>

int main(int argc, char *argv[ ]){
	if (argc < 3){
		printf("[!] Número de argumentos incompátiveis \n");
    	exit (1);
	}
    int portaServidor = atoi(argv[1]); // Recebe a porta de entrada do servidor
    int tamBuffer = atoi(argv[2]); // Recebe o tamanho do buffer
    FILE* file;
    int serverSocket, numDadosSocket, deltaTime = 0, len; // Variáveis de controle da conexão
    unsigned int TotalBytes = 0, numDadosArquivo;
    char* nomeArquivo = (char*) malloc(tamBuffer * sizeof(char));
    char* buffer = (char*) malloc(tamBuffer * sizeof(char)); // Cria um buffer de tamanho tamBuffer
    printf("[+] Buffer de tamanho %d Criado \n", tamBuffer);

	struct timeval timeInit, timeEnd, timeDelta;
	struct sockaddr_in servidorAddr, clienteAddr; // Estrutura existente em netinet/in.h que contém um endereço de internet

	servidorAddr.sin_family = AF_INET; // Família do endrereço
	servidorAddr.sin_port = htons(portaServidor); // Porta de entrada do servidor na ordem de bytes de rede
	servidorAddr.sin_addr.s_addr = INADDR_ANY ; // Endereço IP da maquina servidor

	gettimeofday(&timeInit, NULL); // Recebe o valor do tempo atual

	serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);	// Cria um socket para o servidor
	if (serverSocket < 0){
		printf("[!] Socket não pôde ser criado \n");
    	exit (1);
	}
	printf("[+] Socket criado \n");
	if (bind(serverSocket, (struct sockaddr*) &servidorAddr, sizeof(servidorAddr)) < 0){ // "Linka" o socket a um endereço
		printf("[!] Bind não pôde ser realizado \n");
    	exit (1);
	}
	printf("[+] Bind criado à porta %d \n", portaServidor);
	
    numDadosSocket = recvfrom(serverSocket, buffer, tamBuffer, 0, ( struct sockaddr *) &clienteAddr, &len);
    printf("[+] Recebendo dados do dominio %s, porta %d, endereco %s\n",(clienteAddr.sin_family == PF_INET?"AF_INET":"UNKNOWN"),ntohs(clienteAddr.sin_port),inet_ntoa(clienteAddr.sin_addr));
    if (numDadosSocket < 0){
		printf("[!] Erro ao ler socket \n");
    	exit (1);
	}
    for (int i=0; i < numDadosSocket; i++){ // Recupera o nome do arquivo do buffer
		nomeArquivo[i] = buffer[i];
	}
	nomeArquivo = (char*) realloc(nomeArquivo, numDadosSocket * sizeof(char)); // realoca o tamanho do vetor com o nome do arquivo
    file = fopen(nomeArquivo, "r"); // Abre o arquivo
    if (file == NULL){
    	printf("[!] Arquivo não encontrado \n");
    	close(serverSocket);
    	exit (1);
    }

    numDadosArquivo = fread (buffer, 1, tamBuffer, file); // lê e armazena o numero de caracteres lidos no arquivo
    while (numDadosArquivo > 0){
    	numDadosSocket = sendto(serverSocket, buffer, numDadosArquivo, MSG_CONFIRM, (const struct sockaddr *) &clienteAddr, sizeof(clienteAddr)); 
        if (numDadosSocket < 0){
			printf("[!] Erro ao escrever no socket \n");
    		exit (1);
		}
    	TotalBytes += numDadosArquivo;
        numDadosArquivo = fread (buffer, 1, tamBuffer, file);
    }
    printf("[+] Arquivo enviado \n");
    fclose(file);
    close(serverSocket);
    printf("[+] Socket fechado \n");
    gettimeofday(&timeEnd, NULL);
    timersub(&timeEnd, &timeInit, &timeDelta);
    printf("-------------------------------------------- \n");
    printf("--> Tempo percorrido: %3u.%06u segundos\n", (unsigned int)(timeDelta.tv_sec), (unsigned int)(timeDelta.tv_usec));
    printf("--> Arquivo enviado: %s \n", nomeArquivo);
    printf("--> Total de bytes enviados: %i \n", TotalBytes);
    printf("-------------------------------------------- \n");
    free(buffer);
}
