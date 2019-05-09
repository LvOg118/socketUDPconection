#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>

typedef struct pacote{
    char numSeq;
    char ack;
    char* dados;
}pkg;

void serialize(char* b, pkg* p, int t){
    if (t == 0){
        b[0] = p->numSeq;
        b[1] = p->ack;
    }
    if (t == 1){
        p->numSeq = b[0];
        p->ack = b[1];
        for (int i=0; i<(strlen(b) - 2); i++){
            p->dados[i] = b[i+2];
        }
    }
}

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
    int clientSocket, numDadosSocket; // Variáveis de controle da conexão
    unsigned int TotalBytes = 0;
    double taxa;
    socklen_t size;
    char idPkg = '0', ackRec = '1', temp;
    pkg pkgEnv;
    pkg pkgRec;
    pkgRec.dados = (char*) malloc((tamBuffer - 2) * sizeof(char)); // Aloca um tamanho para dados do pacote
    char* buffer = (char*) malloc(tamBuffer * sizeof(char)); // Cria um buffer de tamanho tamBuffer
    printf("[+] Buffer de tamanho %i Criado \n", tamBuffer);

	struct timeval timeInit, timeEnd, timeDelta, timer; // Estruturas de tempo
	struct sockaddr_in servidorAddr; // Estrutura existente em netinet/in.h que contém um endereço de internet

	servidorAddr.sin_family = AF_INET; // Família do endrereço
	servidorAddr.sin_port = htons(portaServidor); // Porta de entrada do servidor na ordem de bytes de rede
	servidorAddr.sin_addr.s_addr = inet_addr(hostServidor); // Converte o endereço de IP,  para um endereço válido

	gettimeofday(&timeInit, NULL); // Recebe o valor do tempo atual

	clientSocket = socket(AF_INET, SOCK_DGRAM, 0);	// Cria um novo socket
	if (clientSocket < 0){
		printf("[!] Socket não pôde ser criado \n");
    	exit (1);
	}
	printf("[+] Socket criado \n");

        /* --------------------------------------------------
         Envia o nome do arquivo (não há perda de dados aqui) 
        -----------------------------------------------------*/

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

        /* --------------------------------------------------
            Enviar arquivo (pode haver perda de dados aqui) 
        -----------------------------------------------------*/

    do {
    	numDadosSocket = recv(clientSocket, buffer, tamBuffer , 0);
        if (numDadosSocket < 0){
    		printf("[!] Erro na leitura do socket\n");
    		exit (1);
    	}
        //printf("%i, ", numDadosSocket);
        serialize (buffer, &pkgRec, 1);
        //printf("id recebido = %c, idPkg = %c, ack recebido = %c, ackRec = %c\n", pkgRec.numSeq, idPkg, pkgRec.ack, ackRec);
    	//printf("%s\n", buffer);
        //printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
        //printf("%s\n", pkgRec.dados);
        //printf("-------------------------------------------------------");
        if (pkgRec.numSeq == idPkg && pkgRec.ack == ackRec){
    		//printf("%s\n", "OPAAAAAAAA");
            TotalBytes += numDadosSocket - 2;
            //printf("%i, ", numDadosSocket);
    		fwrite(pkgRec.dados , 1 , numDadosSocket - 2 , file); // passa do buffer para o arquivo de saída
            temp = idPkg;
            idPkg = ackRec;
            ackRec = temp;
            pkgEnv.numSeq = idPkg; 
            pkgEnv.ack = ackRec;
            //printf("ack env = %c, id env = %c\n", pkgEnv.ack, pkgEnv.numSeq);
            serialize (buffer, &pkgEnv, 0);
            //printf("mano");
    	    sendto(clientSocket, buffer, 2, 0, (const struct sockaddr *) &servidorAddr, sizeof(servidorAddr)); 
    	}
        else if (pkgRec.numSeq == 'x' || pkgRec.ack == 'x'){
            break;
        }
        else if (pkgRec.numSeq != idPkg || pkgRec.ack != ackRec){
            //printf("hehe");
            sendto(clientSocket, buffer, 2, 0, (const struct sockaddr *) &servidorAddr, sizeof(servidorAddr)); 
        }
    } while (1);

         /* --------------------------------------------------
                             Escrita de dados 
         -----------------------------------------------------*/ 

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
