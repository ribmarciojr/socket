#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <unistd.h>

#define BUFFER_SIZE 100
#define END_SIGNAL "END"

int main(){
    int network_socket;
    network_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_address;

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(9002);
    server_address.sin_addr.s_addr = INADDR_ANY;

    bind(network_socket, (struct sockaddr *) &server_address, sizeof(server_address));

    listen(network_socket, 5);

    int client_socket;
    client_socket = accept(network_socket, NULL, NULL);
    
    // Inicia a comunicação;
    char client_request[256];
    char server_response[256];

    // Recebe READY
    recv(client_socket, &client_request, sizeof(client_request), 0);
    printf("[SERVER] Client sent: %s! \n", client_request);
    
    // Envia READY ACK
    strcpy(server_response, "READY ACK");
    send(client_socket, server_response, sizeof(server_response), 0);

    char buffer[BUFFER_SIZE];
    while (1)
    {
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received == -1) {
            perror("Erro ao receber dados");
            break;
        } else if (bytes_received == 0) {
            printf("Conexão encerrada pelo cliente.\n");
            break;
        }

        buffer[bytes_received] = '\0';

        // Verifica se recebeu o sinal de fim
        if (strcmp(buffer, END_SIGNAL) == 0) {
            printf("Recebido sinal de fim.\n");
            break;
        }

        printf("Recebido: %s\n", buffer);
    }
    

    // Recebe bye
    recv(client_socket, &client_request, sizeof(client_request), 0);
    printf("[SERVER] Client sent: %s! \n", client_request);

    // Envia bye
    strcpy(server_response, "bye");
    send(client_socket, server_response, sizeof(server_response), 0);

    close(client_socket);
    close(network_socket);
    
    return 0;
}