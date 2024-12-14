#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include <sys/time.h>

#define EXCEPTION -1
#define MAX_FILES 100

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


void bit_stuffing(const char *input, char *output) {
    int bit_count = 0;
    int output_index = 0;

    for (int i = 0; input[i] != '\0'; i++) {
        output[output_index++] = input[i];

        if (input[i] == '1') {
            bit_count++;
            if (bit_count == 5) {
                output[output_index++] = '0';  // Insere um bit extra após 5 bits consecutivos de '1'
                bit_count = 0;                // Reseta o contador
            }
        } else {
            bit_count = 0;
        }
    }

    output[output_index] = '\0';  // Finaliza a string
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

    printf("[CLIENT] Conectado no Endereço: %s e Porta: %d! \n", server_host, server_port);
    
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

    if (strcmp(server_response, "READY ACK") == 0)
    {
        int file_count = 0;
        char **file_names = read_archive_names_from(absolute_path_folder, &file_count);
        if (file_names != NULL)
        {
            for (int i = 0; i < file_count; i++)
            {
                if (send(network_socket, file_names[i], strlen(file_names[i]), 0) == -1) {
                    perror("Erro ao enviar o nome do arquivo");
                } else {
                    printf("Enviado: %s\n", file_names[i]);
                }
                
                free(file_names[i]); // Liberar a memória de cada nome de arquivo
            }

            free(file_names); // Liberar o array de ponteiros
        }
    }

    gettimeofday(&end, NULL);

    // Envia bye
    strcpy(client_request, "bye");
    send(network_socket, client_request, sizeof(client_request), 0);

    recv(network_socket, &server_response, sizeof(server_response), 0);
    printf("[CLIENT] Server sent: %s!\n", server_response);

    long elapsed_time = (end.tv_sec - start.tv_sec) * 1000000L + (end.tv_usec - start.tv_usec);

    close(network_socket);
    return 0;
}
