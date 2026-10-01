#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "mega_senha.h"
#include "hash_palavras.h"

#define PORT 8080
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024

#define ESPERANDO_PARTIDA 0
#define DANDO_DICA 1
#define ESPERANDO_CHUTE 2
#define DANDO_CHUTE 3
#define ESPERANDO_DICA 4

typedef struct {
    int socket;
    int id;
    Partida partida;
    int estado; 
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;


 // Envia uma mensagem para o cliente que esteja na mesma partida que sender_id_partida
void broadcast_message(const char *message, int sender_socket, int sender_id_partida)
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i].socket != sender_socket && clients[i].partida.id_partida == sender_id_partida) {
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

 // Remove um cliente da lista de clientes.
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

 // Função executada pela thread de cada cliente.
void *handle_client(void *arg)
{
    Client *client = (Client *)arg;
    client->estado = ESPERANDO_PARTIDA;
    char buffer[BUFFER_SIZE];
    printf("Cliente %d conectado.\n", client->id);

    // Busca outro cliente com id_partida == -1, ou seja, sem partida
    if (client->partida.id_partida == -1) {
        printf("Buscando partida para o cliente %d...\n", client->id);
        // Busca algum cliente sem partida a cada 1 segundo
        while(1){
            int oponente_index = -1;
            sleep(1);
            pthread_mutex_lock(&clients_mutex);
            // verifica no vetor global se já foi colocado em uma partida por outro cliente 
            for (int i = 0; i < client_count; i++) {
                if (clients[i].id == client->id && clients[i].partida.id_partida != -1) {
                    client->partida.id_partida = clients[i].partida.id_partida; // atualiza a variável local da thread
                    break;
                }
            }
            // em caso positivo, termina a busca 
            if (client->partida.id_partida != -1){
                pthread_mutex_unlock(&clients_mutex);
                break;
            }
            // caso contrário, procura outro cliente sem partida para alocar uma nova partida
            for (int i = 0; i < client_count; i++) {
                if (clients[i].id != client->id && clients[i].partida.id_partida == -1) {
                    oponente_index = i;
                    break;
                }
            }
            // caso tenha encontrado, cria a nova partida
            if(oponente_index != -1){
                Partida* nova_partida = criar_partida(clients[oponente_index].id, client->id);
                Senha senha = sortear_senha();
                nova_partida->senha_atual = senha;
                printf("Partida de ID %d criada com os clientes de ID %d e %d.\n", nova_partida->id_partida, nova_partida->id_client_1, nova_partida->id_client_2);
                // Atualiza o cliente atual na thread e no vetor global
                client->partida.id_partida = nova_partida->id_partida;    
                for (int i = 0; i < client_count; i++) {
                    if (clients[i].id == client->id) {
                        clients[i].partida.id_partida = nova_partida->id_partida;
                        break;
                    }
                }
                // atualiza registro global do oponente
                clients[oponente_index].partida.id_partida = nova_partida->id_partida;
                // libera mutex
                pthread_mutex_unlock(&clients_mutex);
                break;
            }
            // caso nao tenha encontrado, libera mutex e continua a busca
            pthread_mutex_unlock(&clients_mutex);
        }
    }

    //informa ao outro cliente da partida que alguém entrou
    char join_message[BUFFER_SIZE];
    snprintf(join_message,
             sizeof(join_message),
             "[Servidor] Cliente %d entrou na partida.\n",
             client->id);

    broadcast_message(join_message, client->socket, client->partida.id_partida);
    
    //define estado inicial dos clientes
    if(client->id == client->partida.id_cliente_adivinha){
        client->estado = ESPERANDO_DICA;
    }
    else {
        client->estado = DANDO_DICA;
    }
    // Recebe mensagens enquanto o cliente estiver conectado
    while (1) {
        if(client->id == client->partida.id_cliente_adivinha){
            if (client->estado == ESPERANDO_DICA) {
                // não deve ler chute ainda
                
            }
            else if (client->estado == DANDO_CHUTE) {
                // recebe chute
                // compara com senha
                
            }

        }
        else {
            if (client->estado == DANDO_DICA) {
            // recebe dica
            // repassa para o outro jogador
            }

            else if (client->estado == ESPERANDO_CHUTE) {
            // aguarda o outro jogador chutar
            }

        }

        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(
            client->socket,
            buffer,
            sizeof(buffer) - 1,
            0
        );
        // recv() retornando 0 significa que o cliente fechou a conexão.
        if (bytes_received == 0) {
            printf("Cliente %d desconectou.\n", client->id);
            break;
        }
        // recv() retornando -1 significa erro.
        if (bytes_received < 0) {
            perror("Erro ao receber mensagem");
            break;
        }
        buffer[bytes_received] = '\0';
        printf("Cliente %d: %s", client->id, buffer);
        // Monta a mensagem que será enviada aos outros clientes.
        char message[BUFFER_SIZE + 100];
        snprintf(
            message,
            sizeof(message),
            "Cliente %d: %s",
            client->id,
            buffer
        );
        broadcast_message(message, client->socket, client->partida.id_partida);
    }
    // Informa aos outros clientes que alguém saiu.
    char leave_message[BUFFER_SIZE];
    snprintf(
        leave_message,
        sizeof(leave_message),
        "[Servidor] Cliente %d saiu do jogo.\n",
        client->id
    );
    remove_client(client->socket);
    broadcast_message(leave_message, -1, client->partida.id_partida);
    close(client->socket);
    free(client);
    return NULL;
}

int main()
{
    carregar_senhas(PATH_SENHAS);

    int server_socket;
    int client_socket;

    struct sockaddr_in server_address;
    struct sockaddr_in client_address;

    socklen_t client_address_length =
        sizeof(client_address);

    // 1. Criação do socket.
    server_socket = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (server_socket < 0) {
        perror("Erro ao criar socket");
        return 1;
    }

    // Permite reutilizar a porta rapidamente após o encerramento do servidor.
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
    // Configuração do endereço do servidor.
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    // INADDR_ANY permite conexões vindas de qualquer interface de rede.
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);
    // 2. Associação do socket com IP e porta.
    if (bind(
            server_socket,
            (struct sockaddr *)&server_address,
            sizeof(server_address)
        ) < 0) {

        perror("Erro no bind");
        close(server_socket);
        return 1;
    }
    // 3. Coloca o socket em modo de escuta.
    if (listen(server_socket, MAX_CLIENTS) < 0) {

        perror("Erro no listen");
        close(server_socket);
        return 1;
    }

    printf("=================================\n");
    printf("       MEGA SENHA\n");
    printf("=================================\n");
    printf("Servidor iniciado na porta %d\n", PORT);
    printf("Aguardando jogadores...\n\n");

    // 4. Loop principal do servidor.
    int client_id = 1;
    while (1) {
        // 5. Aceita uma nova conexão.
        client_socket = accept(
            server_socket,
            (struct sockaddr *)&client_address,
            &client_address_length
        );

        if (client_socket < 0) {
            perror("Erro no accept");
            continue;
        }

        // Verifica limite de clientes.
        pthread_mutex_lock(&clients_mutex);
        if (client_count >= MAX_CLIENTS) {
            pthread_mutex_unlock(&clients_mutex);
            const char *message = "Servidor cheio.\n";
            send(
                client_socket,
                message,
                strlen(message),
                0
            );
            close(client_socket);
            continue;
        }

        // Cria estrutura para o novo cliente.
        Client *client = malloc(sizeof(Client));

        if (client == NULL) {
            perror("Erro ao alocar cliente");
            pthread_mutex_unlock(&clients_mutex);
            close(client_socket);
            continue;
        }

        client->socket = client_socket;
        client->id = client_id++;
        client->partida.id_partida = -1;

        clients[client_count++] = *client;

        pthread_mutex_unlock(&clients_mutex);

        // 6. Cria uma thread para atender o novo cliente.
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
        // A thread é independente. O servidor não precisa esperar por ela.
        pthread_detach(thread);
    }
    close(server_socket);
    return 0;
}