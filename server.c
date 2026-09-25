#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 6767
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024

typedef struct {
    int socket;
    int id;
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Envia uma mensagem para todos os clientes,
 * exceto o cliente que enviou a mensagem.
 */
void broadcast_message(const char *message, int sender_socket)
{
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < client_count; i++) {

        if (clients[i].socket != sender_socket) {

            if (send(clients[i].socket,
                     message,
                     strlen(message),
                     0) < 0) {

                perror("Erro ao enviar mensagem");
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

/*
 * Remove um cliente da lista de clientes.
 */
void remove_client(int socket)
{
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < client_count; i++) {

        if (clients[i].socket == socket) {

            for (int j = i; j < client_count - 1; j++) {
                clients[j] = clients[j + 1];
            }

            client_count--;

            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

/*
 * Função executada pela thread de cada cliente.
 */
void *handle_client(void *arg)
{
    Client *client = (Client *)arg;

    char buffer[BUFFER_SIZE];

    printf("Cliente %d conectado.\n", client->id);

    /*
     * Informa aos outros clientes que alguém entrou.
     */
    char join_message[BUFFER_SIZE];

    snprintf(join_message,
             sizeof(join_message),
             "[Servidor] Cliente %d entrou no chat.\n",
             client->id);

    broadcast_message(join_message, client->socket);

    /*
     * Recebe mensagens enquanto o cliente estiver conectado.
     */
    while (1) {

        memset(buffer, 0, sizeof(buffer));

        int bytes_received = recv(
            client->socket,
            buffer,
            sizeof(buffer) - 1,
            0
        );

        /*
         * recv() retornando 0 significa que o cliente
         * fechou a conexão.
         */
        if (bytes_received == 0) {
            printf("Cliente %d desconectou.\n", client->id);
            break;
        }

        /*
         * recv() retornando -1 significa erro.
         */
        if (bytes_received < 0) {
            perror("Erro ao receber mensagem");
            break;
        }

        buffer[bytes_received] = '\0';

        printf("Cliente %d: %s", client->id, buffer);

        /*
         * Monta a mensagem que será enviada
         * aos outros clientes.
         */
        char message[BUFFER_SIZE + 100];

        snprintf(
            message,
            sizeof(message),
            "Cliente %d: %s",
            client->id,
            buffer
        );

        broadcast_message(message, client->socket);
    }

    /*
     * Informa aos outros clientes que alguém saiu.
     */
    char leave_message[BUFFER_SIZE];

    snprintf(
        leave_message,
        sizeof(leave_message),
        "[Servidor] Cliente %d saiu do chat.\n",
        client->id
    );

    remove_client(client->socket);

    broadcast_message(leave_message, -1);

    close(client->socket);

    free(client);

    return NULL;
}

int main()
{
    int server_socket;
    int client_socket;

    struct sockaddr_in server_address;
    struct sockaddr_in client_address;

    socklen_t client_address_length =
        sizeof(client_address);

    /*
     * 1. Criação do socket.
     */
    server_socket = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (server_socket < 0) {
        perror("Erro ao criar socket");
        return 1;
    }

    /*
     * Permite reutilizar a porta rapidamente
     * após o encerramento do servidor.
     */
    int option = 1;

    if (setsockopt(
            server_socket,
            SOL_SOCKET,
            SO_REUSEADDR,
            &option,
            sizeof(option)
        ) < 0) {

        perror("Erro no setsockopt");
        close(server_socket);
        return 1;
    }

    /*
     * Configuração do endereço do servidor.
     */
    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;

    /*
     * INADDR_ANY permite conexões vindas
     * de qualquer interface de rede.
     */
    server_address.sin_addr.s_addr = INADDR_ANY;

    server_address.sin_port = htons(PORT);

    /*
     * 2. Associação do socket com IP e porta.
     */
    if (bind(
            server_socket,
            (struct sockaddr *)&server_address,
            sizeof(server_address)
        ) < 0) {

        perror("Erro no bind");
        close(server_socket);
        return 1;
    }

    /*
     * 3. Coloca o socket em modo de escuta.
     */
    if (listen(server_socket, MAX_CLIENTS) < 0) {

        perror("Erro no listen");
        close(server_socket);
        return 1;
    }

    printf("=================================\n");
    printf("       SERVIDOR DE CHAT\n");
    printf("=================================\n");
    printf("Servidor iniciado na porta %d\n", PORT);
    printf("Aguardando clientes...\n\n");

    /*
     * 4. Loop principal do servidor.
     */
    int client_id = 1;

    while (1) {

        /*
         * 5. Aceita uma nova conexão.
         */
        client_socket = accept(
            server_socket,
            (struct sockaddr *)&client_address,
            &client_address_length
        );

        if (client_socket < 0) {
            perror("Erro no accept");
            continue;
        }

        /*
         * Verifica limite de clientes.
         */
        pthread_mutex_lock(&clients_mutex);

        if (client_count >= MAX_CLIENTS) {

            pthread_mutex_unlock(&clients_mutex);

            const char *message =
                "Servidor cheio.\n";

            send(
                client_socket,
                message,
                strlen(message),
                0
            );

            close(client_socket);

            continue;
        }

        /*
         * Cria estrutura para o novo cliente.
         */
        Client *client = malloc(sizeof(Client));

        if (client == NULL) {

            perror("Erro ao alocar cliente");

            pthread_mutex_unlock(&clients_mutex);

            close(client_socket);

            continue;
        }

        client->socket = client_socket;
        client->id = client_id++;

        clients[client_count++] = *client;

        pthread_mutex_unlock(&clients_mutex);

        /*
         * 6. Cria uma thread para atender
         * o novo cliente.
         */
        pthread_t thread;

        if (pthread_create(
                &thread,
                NULL,
                handle_client,
                client
            ) != 0) {

            perror("Erro ao criar thread");

            remove_client(client_socket);

            close(client_socket);

            free(client);

            continue;
        }

        /*
         * A thread é independente.
         * O servidor não precisa esperar por ela.
         */
        pthread_detach(thread);
    }

    close(server_socket);

    return 0;
}