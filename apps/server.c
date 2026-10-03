#include "funcoes_server.h"

int main(void) {
    // inicializa os vetores
    memset(clients, 0, sizeof(clients));
    memset(partidas, 0, sizeof(partidas));
    
    // carrega as senha
    carregar_senhas(PATH_SENHAS);

    // cria o socket TCP
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        return 1;
    }

    // reinicia o servidor
    int option = 1;
    
    if(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option))<0){
        perror("setsockopt");
        close(server_socket);
        return 1;
    }

    struct sockaddr_in endereco;
    memset(&endereco, 0, sizeof(endereco));
    endereco.sin_family = AF_INET;

    // aceita conexoes
    endereco.sin_addr.s_addr = INADDR_ANY;
    endereco.sin_port = htons(PORT);

    // configura o socket na porta correta
    if (bind(server_socket, (struct sockaddr *)&endereco, sizeof(endereco)) < 0) {
        perror("bind");
        close(server_socket);
        return 1;
    }

    // coloca o socket em modo escuta
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("listen");
        close(server_socket);
        return 1;
    }

    printf( "=============================\n"
            "         MEGA SENHA\n"
            "=============================\n"
            "Servidor na porta %d\n\n", PORT);


    // loop em que o servidor fica constantemente aceitando conexoes
    while (1) {
        struct sockaddr_in endereco_cliente;
        socklen_t tamanho = sizeof(endereco_cliente);

        // bloqueia ate um cliente se conectar
        int socket_cliente = accept(server_socket, (struct sockaddr *) &endereco_cliente, &tamanho);
        if (socket_cliente < 0) continue;
        pthread_mutex_lock(&mutex);


        // procura uma posicao livre em clients[]
        Client *novo = NULL;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!clients[i].conectado) {
                novo = &clients[i];
                break;
            }
        }

        // caso o servidor lote
        if (novo == NULL) {
            pthread_mutex_unlock(&mutex);
            close(socket_cliente);
            continue;
        }

        // inicializa o cliente
        novo->socket = socket_cliente;
        novo->id = proximo_id_cliente++;
        novo->nome[0] = '\0';

        // enquanto nao pertencer a um partida
        novo->id_partida = -1;

        // coloca o cliente para digitar o seu nome
        novo->estado = ESPERANDO_NOME;
        novo->conectado = 1;

        //salva ID para passar para a thread
        int* id = malloc(sizeof(int));
        if (id == NULL){
            pthread_mutex_unlock(&mutex);
            close(socket_cliente);
            continue;
        }
        * id = novo->id;

        pthread_mutex_unlock(&mutex);

        // cria thread para o cliente
        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, id) != 0) {
            desconectar(*id);
            free(id);
            continue;
        }

        pthread_detach(thread);
    }

    // fecha o servidor
    close(server_socket);

    return 0;
}