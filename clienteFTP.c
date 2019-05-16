#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "tp_socket.c"

typedef struct pacote{ // "pacote" que trafega pelo UDP
    char numSeq;
    char ack;
    char* dados;
}pkg;

void serialize(char* b, pkg* p, int t){ // transforma a estrutura pacote em um vetor e vice-versa
    if (t == 0){
        b[0] = p->numSeq;
        b[1] = p->ack;
    }
    if (t == 1){
        p->numSeq = b[0];
        p->ack = b[1];
        b = b + 2;
        strcpy(p->dados, b);
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
    char idPkg = '0', ackRec = '1', temp; // variaveis de controle para a transferencia confiável
    pkg pkgEnv; // pacote que é enviado
    pkg pkgRec; // pacote que é recebido
    tp_init();
    pkgRec.dados = (char*) malloc((tamBuffer - 2) * sizeof(char)); // Aloca um tamanho para dados do pacote
    char* buffer = (char*) malloc(tamBuffer * sizeof(char)); // Cria um buffer de tamanho tamBuffer
    printf("[+] Buffer de tamanho %i Criado \n", tamBuffer);

	struct timeval timeInit, timeEnd, timeDelta, timer; // Estruturas de tempo
	struct sockaddr_in servidorAddr; // Estrutura existente em netinet/in.h que contém um endereço de internet

	if(tp_build_addr(&servidorAddr, hostServidor, portaServidor) == -1){ // criar estrutura de endereço
        perror("[!] Falha ao criar endereçamento do servidor \n");
    }

	gettimeofday(&timeInit, NULL); // Recebe o valor do tempo atual

	clientSocket = tp_socket(0);	// Cria um novo socket
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
	numDadosSocket = tp_sendto(clientSocket, buffer, strlen(nomeArquivo), &servidorAddr); // envia o nome do arquivo
	if (numDadosSocket < 0){
		printf("[!] Escrita no socket não pôde ser realizada \n");
    	exit (1);
	}
    printf("[+] Requisição de arquivo enviada \n");
    file = fopen("saida.txt", "w"); // Abre o arquivo de escrita;
    printf("[+] Esperando dados ...\n");

        /* --------------------------------------------------
            Enviar arquivo (pode haver perda de dados aqui)
        -----------------------------------------------------*/
    
    do {
    	numDadosSocket = tp_recvfrom(clientSocket, buffer, tamBuffer , &servidorAddr); // esperando receber algo
        if (numDadosSocket < 0){
    		printf("[!] Erro na leitura do socket\n");
    		exit (1);
    	}
        serialize (buffer, &pkgRec, 1); // ao receber, deserializa o pacote
        if (pkgRec.numSeq == idPkg && pkgRec.ack == ackRec){ // caso se receba um pacote como o esperado
            TotalBytes += numDadosSocket - 2;
    		fwrite(pkgRec.dados , 1 , numDadosSocket - 2 , file); // passa do pacote para o arquivo de saída
            temp = idPkg; // altera as variaveis de estado
            idPkg = ackRec;
            ackRec = temp;
            pkgEnv.numSeq = idPkg; // monta o pacote de envio
            pkgEnv.ack = ackRec;
            serialize (buffer, &pkgEnv, 0);
    	    tp_sendto(clientSocket, buffer, 2, &servidorAddr); // envia uma confirmação
    	}
        else if (pkgRec.numSeq == 'x' && pkgRec.ack == 'x'){ // caso o pacote recebido seja para fechar a conexão
            break;
        }
        else if (pkgRec.numSeq != idPkg || pkgRec.ack != ackRec){ // caso o pacote recebido esteja errado (perde de ack)
            serialize (buffer, &pkgEnv, 0);
            tp_sendto(clientSocket, buffer, 2, &servidorAddr); // reenvia último pacote
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
    free(pkgRec.dados);
}
