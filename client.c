#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int socket_fd;

// Thread responsável por receber mensagens do servidor.
void *receive_messages(void *arg)
{
    char buffer[BUFFER_SIZE];
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(
            socket_fd,
            buffer,
            sizeof(buffer) - 1,
            0
        );
        // Servidor encerrou a conexão.
        if (bytes_received == 0) {
            printf("\n[Servidor] Conexão encerrada.\n");
            exit(0);
        }
        // Erro na comunicação.
        if (bytes_received < 0) {
            perror("\nErro ao receber mensagem");
            exit(1);
        }
        buffer[bytes_received] = '\0';
        printf("\r%s", buffer);
        fflush(stdout);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    struct sockaddr_in server_address;
    // O IP do servidor é passado pela linha de comando. Exemplo: ./client 192.168.1.10
    if (argc != 2) {
        printf("Uso: %s <IP_DO_SERVIDOR>\n", argv[0]);
        return 1;
    }
    // 1. Criação do socket.
    socket_fd = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );
    if (socket_fd < 0) {
        perror("Erro ao criar socket");
        return 1;
    }
    // Configuração do endereço do servidor.
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    // Converte o IP informado para formato utilizado pelo socket.
    if (inet_pton(
            AF_INET,
            argv[1],
            &server_address.sin_addr
        ) <= 0) {
        printf("IP inválido: %s\n", argv[1]);
        close(socket_fd);
        return 1;
    }
    // 2. Conecta ao servidor.
    if (connect(
            socket_fd,
            (struct sockaddr *)&server_address,
            sizeof(server_address)
        ) < 0) {
        perror("Erro ao conectar ao servidor");
        close(socket_fd);
        return 1;
    }

    printf("=================================\n");
    printf("          CHAT CLIENTE\n");
    printf("=================================\n");
    printf("Conectado ao servidor %s:%d\n", argv[1], PORT);
    printf("Digite suas mensagens abaixo.\n");
    printf("Digite /sair para sair.\n\n");

    // 3. Cria thread responsável por receber mensagens.
    pthread_t receive_thread;
    if (pthread_create(
            &receive_thread,
            NULL,
            receive_messages,
            NULL
        ) != 0) {
        perror("Erro ao criar thread");
        close(socket_fd);
        return 1;
    }
    
     // 4. Loop principal:lê mensagens do usuário e envia para o servidor.
    char message[BUFFER_SIZE];
    while (1) {
        printf("> ");
        fflush(stdout);
        if (fgets(
                message,
                sizeof(message),
                stdin
            ) == NULL) {

            break;
        }
        // Verifica comando de saída.
        if (strcmp(message, "/sair\n") == 0) {
            printf("Desconectando...\n");
            break;
        }
        // Não envia mensagens vazias.
        if (strlen(message) <= 1) {
            continue;
        }
        // Envia mensagem ao servidor.
        if (send(
                socket_fd,
                message,
                strlen(message),
                0
            ) < 0) {
            perror("Erro ao enviar mensagem");
            break;
        }
    }
    // Fecha o socket.
    close(socket_fd);
    return 0;
}