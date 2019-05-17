Para a execução dos arquivos basta:

servidorFTP.c ------------------------------
COMPILAÇÃO: $ gcc servidorFTP.c -o servidor
EXECUÇÃO: $ ./servidor "porta" "tam buffer"


clienteFTP.c -------------------------------
COMPILAÇÃO: $ gcc clienteFTP.c -o cliente
EXECUÇÃO: $ ./cliente "IP servidor" "porta do servidor" "nome do arquivo" "tam buffer"

O servidor é geralmente executado antes do cliente, uma vez que aguarda o envio do nome do arquivo
pelo cliente.
DÚVIDAS: luan_gnp@outlook.com