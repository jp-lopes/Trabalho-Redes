#include "funcoes_client.h"

int main(int argc, char *argv[])
{
    int socket_fd;
    char ip[64];
    // quando não é passado nenhum argumento, conecta no IP local (127.0.0.1)
    if (argc == 1) {
        strcpy(ip, "127.0.0.1");
    // caso contrário usa o IP passado
    } else if (argc == 2) {
        strncpy(ip, argv[1], sizeof(ip) - 1);
        ip[sizeof(ip) - 1] = '\0';
    // entrada inválida
    } else {
        printf("Passe um IP como argumento ou nenhum argumento para usar o IP local.");
        return 1;
    }

    // cria socket usando protocolo TCP e IPv4.
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    // erro no socket (função socket() retorna -1 em caso de erro)
    if (socket_fd < 0) {
        perror("socket");
        return 1;
    }
    
    // cria e inicializa endereco socket
    struct sockaddr_in endereco;
    memset(&endereco, 0, sizeof(endereco));
    endereco.sin_family = AF_INET;
    endereco.sin_port = htons(PORT);

    // converte o IP em texto
    if (inet_pton(AF_INET, ip, &endereco.sin_addr) <= 0) {
        printf("IP invalido.\n");
        return 1;
    }

    // conecta com o servidor
    if (connect(socket_fd, (struct sockaddr *)&endereco, sizeof(endereco)) < 0) {
        perror("connect");
        return 1;
    }

    // erro caso o cliente nao consiga carregar o dicionario
    if (!hash_inicializar(PATH_DICIONARIO)) {
        printf("Erro ao carregar dicionario.\n");
        return 1;
    }

    // printa cabeçalho do jogo
    printf(
        "=============================\n"
        "         MEGA SENHA\n"
        "=============================\n"
        "Conectado ao servidor.\n"
        "Digite o seu apelido: "
    );

    fflush(stdout);

    // thread responsável por receber mensagens do servidor
    pthread_t thread_receive_messages;

    if (pthread_create(
            &thread_receive_messages,
            NULL,
            receive_messages,
            (void*)(&socket_fd)
        ) != 0) {

        return 1; // retorna em caso de erro na criação da thread
    }

    // thread principal responsavel pelo teclado
    char mensagem[BUFFER_SIZE];
    while (1) {
        // obtem estado atual do jogo e printa no terminal dependendo dele
        int estado = obter_estado();
        if (fgets(mensagem, sizeof(mensagem), stdin) == NULL) break;
        
        // esperando nome
        if (estado == ESPERANDO_NOME) {
            char envio[BUFFER_SIZE+5];
            snprintf(envio, sizeof(envio), "NOME|%s", mensagem); 
            enviar_linha(envio, socket_fd);
            continue;
        }

        printf("> ");
        fflush(stdout);

        //remove '\n'
        mensagem[strcspn(mensagem, "\r\n")] = '\0';
        if (mensagem[0] == '\0') continue;

        // se digitar 'sair'
        if (strcmp(mensagem, "/sair") == 0) break;

        // chat global
        if (estado == CHAT_GLOBAL) {
            enviar_linha(mensagem, socket_fd); // se for mensagens para o chat, envia para o servidor
            // caso o jogador digite /entrar ele entra na fila para procurar uma partida
            if (strcmp(mensagem, "/entrar") == 0) {
                definir_estado(ESPERANDO_PARTIDA);
            }
            continue;
        }

        // nao faz nada enquanto espera outro jogador
        if (estado == ESPERANDO_PARTIDA) {
            printf("Aguardando outro jogador...\n");
            continue;
        }

        // para o jogador que da a dica
        if (estado == DANDO_DICA) {
            // o proprio cliente valida a dica
            if (!verificar_dica_valida(mensagem) && !(strcmp(mensagem, "/pass") == 0)) {
                printf( "Dica invalida.\n"
                        "Ela deve existir no dicionario "
                        "e nao pode conter o radical.\n"
                );
                continue;
            }

            char envio[BUFFER_SIZE+5];
            snprintf(envio, sizeof(envio), "DICA|%s", mensagem); 
            enviar_linha(envio, socket_fd);

           // espera o outro jogador chutar
            definir_estado(ESPERANDO_JOGADA);
            continue;
        }

        // para o jogador que tenta acertar a palavra
        if (estado == DANDO_CHUTE) {

            if (!verificar_tentativa(mensagem)) {
                printf( "Tentativa invalida. "
                        "Digite uma palavra do dicionario.\n"
                );
                continue;
            }

            char envio[BUFFER_SIZE+6];
            snprintf(envio, sizeof(envio), "CHUTE|%s", mensagem);
            enviar_linha(envio, socket_fd);


            // espera o servidor dizer se a o chute esta certo
            definir_estado(ESPERANDO_JOGADA);
            continue;
        }

        // intervalo entre o primeiro e o segundo tempo, espera confirmação do jogador
        if (estado == CONFIRMANDO) {
            if (strcmp(mensagem, "/confirmar") != 0) {
                printf("Digite /confirmar.\n");
                continue;
            }
            enviar_linha("/confirmar", socket_fd); // avisa o servidor de que o cliente confirmou
            definir_estado(ESPERANDO_CONFIRMACAO); // aguarda confirmação do outro cliente
            continue;
        }

        // nos outros estados, apenas aguarda
        printf( "Aguarde o outro jogador.\n");
    }

    // libera memória alocada para o hash do dicionário
    hash_liberar();

    // encerra a conexao
    shutdown(socket_fd, SHUT_RDWR);
    close(socket_fd);

    return 0;
}