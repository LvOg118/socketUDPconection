#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include "tp_socket.c"

typedef struct pacote{ // "pacote que trafega pelo UDP"
    char numSeq;
    char ack;
    char* dados;
}pkg;

void serialize(char* b, pkg* p, int t){ // transforma a estrutura pacote em um vetor e vice-versa
    if (t == 0){
        b = b + 2;
        strcpy(b, p->dados);
        b = b - 2;
        b[0] = p->numSeq;
        b[1] = p->ack;
    }
    if (t == 1){
        p->numSeq = b[0];
        p->ack = b[1];
    }
}

int main(int argc, char *argv[ ]){
	if (argc < 3){
		printf("[!] Número de argumentos incompátiveis \n");
    	exit (1);
	}
    int portaServidor = atoi(argv[1]); // Recebe a porta de entrada do servidor
    int tamBuffer = atoi(argv[2]); // Recebe o tamanho do buffer
    FILE* file;
    int serverSocket, numDadosSocket, deltaTime = 0; // Variáveis de controle da conexão
    unsigned int TotalBytes = 0, numDadosArquivo;
    char idPkg = '0', ackRec = '1', temp; // variaveis de controle para a transferencia confiável
    pkg pkgEnv; // pacote que é enviado
    pkg pkgRec; // pacote que é recebido
    tp_init();
    pkgEnv.dados = (char*)malloc((tamBuffer - 2) * sizeof(char));
    char* nomeArquivo = (char*) malloc(tamBuffer * sizeof(char));
    char* buffer = (char*) malloc(tamBuffer * sizeof(char)); // Cria um buffer de tamanho tamBuffer
    printf("[+] Buffer de tamanho %d Criado \n", tamBuffer);

	struct timeval timeInit, timeEnd, timeDelta, timer; // estruturas de tempo
	struct sockaddr_in clienteAddr; // Estrutura existente em netinet/in.h que contém um endereço de internet

	gettimeofday(&timeInit, NULL); // Recebe o valor do tempo atual

	serverSocket = tp_socket((unsigned short) portaServidor);	// Cria um socket para o servidor
	if (serverSocket < 0){
		printf("[!] Socket não pôde ser criado \n");
    	exit (1);
	}
	printf("[+] Socket criado \n");

        /* --------------------------------------------------
        Receber o nome do arquivo (não há perda de dados aqui)
        -----------------------------------------------------*/

    numDadosSocket = tp_recvfrom(serverSocket, buffer, tamBuffer, &clienteAddr); // recebe o nome
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

        /* --------------------------------------------------
            Enviar arquivo (pode haver perda de dados aqui)
        -----------------------------------------------------*/

	numDadosArquivo = fread (pkgEnv.dados, 1, tamBuffer - 2, file); // lê certo numero de dados e poe no pacote
    timer.tv_sec = 1;
    timer.tv_usec = 0;
    setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&timer, sizeof(struct timeval)); // temporizador de 1 sec
    printf("[+] Enviando dados ao dominio %s, porta %d, endereco %s\n",(clienteAddr.sin_family == AF_INET?"AF_INET":"UNKNOWN"),ntohs(clienteAddr.sin_port),inet_ntoa(clienteAddr.sin_addr));
    pkgEnv.numSeq = idPkg; // seta o primeiro numeros de sequencia e ack
    pkgEnv.ack = ackRec;
    serialize(buffer, &pkgEnv, 0); // serializa o pacote
    numDadosSocket = tp_sendto(serverSocket, buffer, numDadosArquivo + 2, &clienteAddr); // envia o primeiro pacote
    if (numDadosSocket < 0){
		printf("[!] Erro ao escrever no socket \n");
        printf ("%s", strerror(errno));
    	exit (1);
	}
    while (1){
        numDadosSocket = tp_recvfrom(serverSocket, buffer, tamBuffer, &clienteAddr); // espera a confirmação do cliente
        serialize(buffer, &pkgRec, 1); // deserializa a resposta
        if (errno == EAGAIN){ // caso não receba nenhuma temporização em 1 segundo
        	errno = 0;
        	printf("Pacote perdido! Reenviando último pacote ...\n");
            serialize(buffer, &pkgEnv, 0);
            tp_sendto(serverSocket, buffer, numDadosArquivo + 2, &clienteAddr); // reenvia ultimo pacote
        }
        else if (pkgRec.numSeq == ackRec && pkgRec.ack == idPkg){ // caso receba uma resposta como o esperado
            temp = idPkg; // altera as variaveis de estado
            idPkg = ackRec;
            ackRec = temp;
            pkgEnv.numSeq = idPkg;
            pkgEnv.ack = ackRec;
            TotalBytes += numDadosArquivo;
            numDadosArquivo = fread (pkgEnv.dados, 1, tamBuffer - 2, file); // remonta o pacote
            serialize(buffer, &pkgEnv, 0); // serializa o pacote
            if (numDadosArquivo > 0){          	
            	tp_sendto(serverSocket, buffer, numDadosArquivo + 2, &clienteAddr);
        	}
        	else if (numDadosArquivo == 0){ // caso queira encerrar a conexão
        		tp_sendto(serverSocket, "xx", 2, &clienteAddr);  // envia pacote de fechamento
        		break;
        	}
        }
        else{
        	printf("Alguma coisa deu errado!\n"); // geralmente esse caso nunca ocorre
        }
    }

         /* --------------------------------------------------
                             Escrita de dados
         -----------------------------------------------------*/

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
    free(nomeArquivo);
    free(pkgEnv.dados);
}
