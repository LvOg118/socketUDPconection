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
    if (argc != 5){
    	printf("[!] Número de argumentos incompatível \n");
    	exit (1);
    }
    char* hostServidor = argv[1]; // Recebe o IP do servidor
    int portaServidor = atoi(argv[2]); // Recebe a porta de entrada no servidor
    char* nomeArquivo = argv[3]; // Recebe o nome do arquivo
    int tamBuffer = atoi(argv[4]); // Recebe o tamanho do buffer
    FILE *file; 
    int clientSocket, numDadosSocket, len; // Variáveis de controle da conexão
    unsigned int TotalBytes = 0;
    double taxa;
    char* buffer = (char*) malloc(tamBuffer * sizeof(char)); // Cria um buffer de tamanho tamBuffer
    printf("[+] Buffer de tamanho %i Criado \n", tamBuffer);

	struct timeval timeInit, timeEnd, timeDelta; // Estruturas de tempo
	struct sockaddr_in servidorAddr; // Estrutura existente em netinet/in.h que contém um endereço de internet

	servidorAddr.sin_family = AF_INET; // Família do endrereço
	servidorAddr.sin_port = htons(portaServidor); // Porta de entrada do servidor na ordem de bytes de rede
	servidorAddr.sin_addr.s_addr = inet_addr(hostServidor); // Converte o endereço de IP,  para um endereço válido

	gettimeofday(&timeInit, NULL); // Recebe o valor do tempo atual

	clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);	// Cria um novo socket
	if (clientSocket < 0){
		printf("[!] Socket não pôde ser criado \n");
    	exit (1);
	}
	printf("[+] Socket criado \n");
	for (int i=0; i < strlen(nomeArquivo); i++){ // Coloca o nome do arquivo no buffer
		buffer[i] = nomeArquivo[i];
	}
	numDadosSocket = sendto(clientSocket, buffer, strlen(nomeArquivo), 0, (const struct sockaddr *) &servidorAddr, sizeof(servidorAddr));
	if (numDadosSocket < 0){
		printf("[!] Escrita no socket não pôde ser realizada \n");
    	exit (1);
	}
    printf("[+] Requisição de arquivo enviada \n");
    file = fopen("saida.txt", "w"); // Abre o arquivo de escrita;
    printf("[+] Recebendo dados \n");
    numDadosSocket = recvfrom(clientSocket, buffer, tamBuffer, MSG_WAITALL, (struct sockaddr *) &servidorAddr, &len);
    if (numDadosSocket < 0){
    	printf("[!] Erro na leitura do socket\n");
    	exit (1);
    }
    while( numDadosSocket > 0 ){
        fwrite(buffer , 1 , numDadosSocket , file); // passa do buffer para o arquivo de saída
        TotalBytes += numDadosSocket;
        numDadosSocket = recvfrom(clientSocket, buffer, tamBuffer, MSG_WAITALL, (struct sockaddr *) &servidorAddr, &len);
    }
    printf("[+] Dados recebidos \n");
    fclose(file);
    close(clientSocket);
    gettimeofday(&timeEnd, NULL); // Recebe o valor do tempo final
    timersub(&timeEnd, &timeInit, &timeDelta); // Calcula a variação de tempo
    taxa = (double)((int)(timeDelta.tv_sec) + ((double)(timeDelta.tv_usec))/1000000); // calculo do tempo gasto
    taxa = (TotalBytes/1000)/taxa; // calculo da taxa em kbps
    printf("-------------------------------------------- \n");
    printf("--> Tempo percorrido: %3u.%06u segundos\n", (int)(timeDelta.tv_sec), (int)(timeDelta.tv_usec));
    printf("--> Buffer = %5u byte(s), %10.2f kbps (%u bytes em %3u.%06u s) \n", tamBuffer, taxa, TotalBytes, (unsigned int)timeDelta.tv_sec, (unsigned int)timeDelta.tv_usec);
    printf("-------------------------------------------- \n");
    free(buffer);
}
