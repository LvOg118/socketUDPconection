Para a execu��o dos arquivos basta:

servidorFTP.c ------------------------------
COMPILA��O: $ gcc servidorFTP.c -o servidor
EXECU��O: $ ./servidor "porta" "tam buffer"


clienteFTP.c -------------------------------
COMPILA��O: $ gcc clienteFTP.c -o cliente
EXECU��O: $ ./cliente "IP servidor" "porta do servidor" "nome do arquivo" "tam buffer"

O servidor � geralmente executado antes do cliente, uma vez que aguarda o envio do nome do arquivo
pelo cliente.
D�VIDAS: luan_gnp@outlook.com