#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include <sys/time.h>

#define EXCEPTION -1
#define MAX_FILES 100
#define ESCAPE_CHAR '\\' 
#define BUFFER_SIZE 2048
#define MAX_MESSAGE_LENGTH 100000
#define EMPTY_CHAR '*'

char **read_archive_names_from(const char *absolute_path_folder, int *file_count)
{
    DIR *dir = opendir(absolute_path_folder);

    if (dir == NULL)
    {
        perror("Erro ao abrir o diretório");
        return NULL;
    }

    struct dirent *entrada;
    char **file_list = NULL; // Inicializa como NULL
    *file_count = 0;         // Contador de arquivos encontrados

    // Primeira passagem: conta os arquivos
    while ((entrada = readdir(dir)) != NULL)
    {
        // Ignorar '.' e '..'
        if (entrada->d_name[0] == '.' && (entrada->d_name[1] == '\0' || (entrada->d_name[1] == '.' && entrada->d_name[2] == '\0')))
        {
            continue;
        }

        // Incrementa a contagem de arquivos
        (*file_count)++;
    }

    // Aloca memória para o número de arquivos encontrados
    file_list = malloc(*file_count * sizeof(char *));
    if (file_list == NULL)
    {
        perror("Erro ao alocar memória para a lista de arquivos");
        closedir(dir);
        return NULL;
    }

    // Segunda passagem: armazena os nomes dos arquivos
    rewinddir(dir); // Reinicia a leitura do diretório
    int index = 0;
    while ((entrada = readdir(dir)) != NULL)
    {
        // Ignorar '.' e '..'
        if (entrada->d_name[0] == '.' && (entrada->d_name[1] == '\0' || (entrada->d_name[1] == '.' && entrada->d_name[2] == '\0')))
        {
            continue;
        }

        // Aloca memória para o nome do arquivo e copia o nome para o array
        file_list[index] = strdup(entrada->d_name); // strdup aloca e copia a string
        if (file_list[index] == NULL)
        {
            perror("Erro ao alocar memória para o nome do arquivo");
            // Liberar memória alocada até o momento antes de sair
            for (int i = 0; i < index; i++)
            {
                free(file_list[i]);
            }
            free(file_list);
            closedir(dir);
            return NULL;
        }

        index++;
    }

    // Fecha o diretório
    closedir(dir);

    return file_list;
};

char* extract_last_part(const char *path) {
    // Localiza a última ocorrência de '/'
    const char *last_slash = strrchr(path, '/');
    if (last_slash != NULL) {
        // Retorna a substring após o último '/'
        return strdup(last_slash + 1); // strdup aloca memória para a substring
    } else {
        // Caso não haja '/', retorna o próprio path
        return strdup(path);
    }
}

void char_stuffing(char *data) {
    char *src = data;
    char *dst = data;
    
    while (*src != '\0') {
        if (*src == ESCAPE_CHAR || *src == '\n' || *src == '\r') {  // Adicione mais caracteres especiais conforme necessário
            *dst++ = ESCAPE_CHAR;  // Coloca o caractere de escape
        }
        *dst++ = *src++;  // Copia o caractere original
    }
    *dst = '\0';  // Finaliza a string
}

void adjust_message_for_buffer(const char *input_message, char *output_buffer) {
    size_t message_length = strlen(input_message);

    // Verifica se a mensagem é maior que o tamanho do buffer
    if (message_length >= BUFFER_SIZE) {
        printf("Mensagem muito longa, truncando...\n");
        // Trunca a mensagem para garantir que ela caiba no buffer
        strncpy(output_buffer, input_message, BUFFER_SIZE - 1);
        output_buffer[BUFFER_SIZE - 1] = '\0';  // Garante que a string termina com '\0'
    } else {
        // Se a mensagem for pequena o suficiente, copia normalmente
        strcpy(output_buffer, input_message);
    }
}

void fill_empty_space(char *buffer, size_t buffer_length) {
    size_t content_length = strlen(buffer);

    for (size_t i = content_length; i < buffer_length - 1; i++) {
        buffer[i] = EMPTY_CHAR;  // Preenche com EMPTY_CHAR
    }
    buffer[buffer_length - 1] = '\0';  // Garante o terminador nulo
}

int main(int argc, char *argv[])
{
    const char *server_host = argv[1];
    const int server_port = atoi(argv[2]);
    const char *absolute_path_folder = argv[3];

    int network_socket;
    network_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_address;

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    int addr_assign = inet_pton(AF_INET, server_host, &server_address.sin_addr.s_addr);

    if (addr_assign == EXCEPTION)
    {
        printf("[CLIENT] Falha ao localizar o endereço do servidor!");
        return 1;
    };

    int connection_status = connect(network_socket, (struct sockaddr *)&server_address, sizeof(server_address));

    if (connection_status == EXCEPTION)
    {
        printf("[CLIENT] Falha ao estabelecer conexão com o servidor!");
        return 1;
    };

    printf("[CLIENT] Conectado no Endereço: %s e Porta: %d \n", server_host, server_port);
    
    // Inicia a comunicação;
    char client_request[256];
    char server_response[256];

    int status_code;
    struct timeval start, end;

    // Envia READY
    strcpy(client_request, "READY");
    send(network_socket, client_request, sizeof(client_request), 0);
    gettimeofday(&start, NULL);

    // Recebe READY ACK
    recv(network_socket, &server_response, sizeof(server_response), 0);
    printf("[CLIENT] Server sent: %s!\n", server_response);

    // Envia nome do diretório
    strcpy(client_request, extract_last_part(absolute_path_folder));
    send(network_socket, client_request, sizeof(client_request), 0);
    
    int bytes_sent = 0;
    if (strcmp(server_response, "READY ACK") == 0)
    {
        int file_count = 0;
        char **file_names = read_archive_names_from(absolute_path_folder, &file_count);
        
        if (file_names != NULL)
        {
            char buffer[BUFFER_SIZE];
            for (int i = 0; i < file_count; i++)
            {
                if(strlen(file_names[i]) > (BUFFER_SIZE - 1)){
                    continue;
                }
                memset(buffer, 0, sizeof(buffer));
                char_stuffing(file_names[i]);

                strncpy(buffer, file_names[i], BUFFER_SIZE -1);
                //adjust_message_for_buffer(complete_message, buffer);

                fill_empty_space(buffer, strlen(buffer));
                
                int bytes = send(network_socket, buffer, strlen(buffer), 0);
                if (bytes == -1) {
                    perror("Erro ao enviar o nome do arquivo");
                }
                bytes_sent = bytes_sent + bytes;

                free(file_names[i]); // Liberar a memória de cada nome de arquivo
                
                // Recebe mensagem se a string foi enviada;
                recv(network_socket, &server_response, sizeof(server_response), 0);
            }

            free(file_names); // Liberar o array de ponteiros
        }
    }


    // Envia bye
    strcpy(client_request, "bye");
    send(network_socket, client_request, sizeof(client_request), 0);

    // Recebe bye
    recv(network_socket, &server_response, sizeof(server_response), 0);
    printf("[CLIENT] Server sent: %s!\n", server_response);
    
    gettimeofday(&end, NULL);
    
    close(network_socket);

    // long elapsed_time = (end.tv_sec - start.tv_sec) * 1000000L + (end.tv_usec - start.tv_usec);
    
    // printf("Time taken to sent %d bytes: %ld milliseconds!\n", bytes_sent, elapsed_time);
    // double thoughput = (double) bytes_sent / elapsed_time;

    // printf("Throughput: %.10f!", thoughput);

    long elapsed_time = (end.tv_sec - start.tv_sec) * 1000000L + (end.tv_usec - start.tv_usec);
    long elapsed_time_ms = elapsed_time / 1000;  // Convertendo para milissegundos

    printf("Time taken to send %d bytes: %ld milliseconds!\n", bytes_sent, elapsed_time_ms);
    double throughput = (double) bytes_sent / elapsed_time_ms;

    printf("Throughput: %.10f!\n", throughput);
    return 0;
}
