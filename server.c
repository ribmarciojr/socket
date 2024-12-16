#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include <sys/stat.h>

#define BUFFER_SIZE 2048
#define END_SIGNAL "bye"
#define ESCAPE_CHAR '\\' 
#define EMPTY_CHAR '*'

void char_destuffing(char *data) {
    char *src = data;
    char *dst = data;
      
    while (*src != '\0') {
        if (*src == ESCAPE_CHAR) {
            // Se encontrar o caractere de escape, pula o próximo caractere
            src++;
        }
        *dst++ = *src++;  // Copia o caractere original
    }
    *dst = '\0';  // Finaliza a string
}

void save_string_to_file(const char *filename, const char *content) {
    // Abre o arquivo no modo de escrita (cria se não existir)
    FILE *file = fopen(filename, "ab+");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        return;
    }
    
    int message_lenth = strlen(content);   
    char message[message_lenth + 1];
    
    strcpy(message, content);
    
    message[message_lenth] = '\n';

    // Escreve a string no arquivo
    if (fputs(message, file) == EOF) {
        perror("Erro ao escrever no arquivo");
    }

    // Fecha o arquivo
    fclose(file);
}

char* create_directory_name(const char* prefix, const char* original) {
    if (prefix == NULL || original == NULL) {
        fprintf(stderr, "Erro: prefix ou original não podem ser NULL.\n");
        return NULL;
    }

    // Calcula o tamanho necessário para a string concatenada
    size_t total_length = strlen(prefix) + strlen(original) + 5;

    // Aloca memória para a nova string
    char *full_path = malloc(total_length);
    if (full_path == NULL) {
        perror("Erro ao alocar memória");
        return NULL;
    }

    // Concatena `prefix` e `original`
    strcpy(full_path, prefix);  // Copia `prefix` para `full_path`
    strcat(full_path, original); // Concatena `original` ao final de `prefix`

    return full_path; // Retorna o caminho completo
}

void remove_empty_space(char *buffer) {
    size_t read_pos = 0, write_pos = 0;

    while (buffer[read_pos] != '\0') {
        if (buffer[read_pos] != EMPTY_CHAR) {
            buffer[write_pos++] = buffer[read_pos];  // Copia somente os caracteres válidos
        }
        read_pos++;
    }
    buffer[write_pos] = '\0';  // Garante o terminador nulo
}

int main(int argc, char *argv[]){

    const char *server_host = argv[1];
    const int server_port = atoi(argv[2]);

    int network_socket;
    network_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_address;

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    inet_pton(AF_INET, server_host, &server_address.sin_addr.s_addr);

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
    send(client_socket, "READY ACK", sizeof(server_response), 0);

    // Recebe o nome do diretório para ser o nome do arquivo;
    char directory_name[256];
    recv(client_socket, directory_name, sizeof(client_request), 0);
    char *archive_name = create_directory_name(server_host, directory_name);
    strcat(archive_name, ".txt");

    char buffer[BUFFER_SIZE];
    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        
        remove_empty_space(buffer);

        if (bytes_received == -1) {
            perror("Erro ao receber dados");
            break;
        } else if (bytes_received == 0) {
            printf("Conexão encerrada pelo cliente.\n");
            break;
        }

        buffer[bytes_received] = '\0';
        
        char_destuffing(buffer);

        // Verifica se recebeu o sinal de fim
        if (strcmp(buffer, "bye") == 0) {
            printf("[SERVER] Client sent: %s! \n", buffer);
            break;
        }

        save_string_to_file(archive_name, buffer);
        // Envia sinal de que a string foi recebida;
        send(client_socket, "Received ACK", sizeof(server_response), 0);
    }

    // // Recebe bye
    // recv(client_socket, &client_request, sizeof(client_request), 0);
    // printf("[SERVER] Client sent: %s! \n", client_request);

    // Envia bye
    strcpy(server_response, "bye");
    send(client_socket, server_response, sizeof(server_response), 0);

    close(client_socket);
    close(network_socket);
    
    return 0;
}