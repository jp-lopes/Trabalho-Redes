#include "funcoes_client.h"

// estado atual do cliente / jogo
EstadoCliente jogo = {
    .estado = ESPERANDO_NOME,
    .pontos = 0,
    .tempo_restante = 0
};

// mutex para controlar o acesso ao estado atual do cliente
pthread_mutex_t mutex_estado = PTHREAD_MUTEX_INITIALIZER;

// verifica se a dica é válida, isto é, se existe no dicionário e se não contém o radical da senha
int verificar_dica_valida(char* dica) {
    formatar_string(dica); // formata string retirando acentos e letras maiúsculas
    // verifica se não contém o radical da senha e se existe no dicionário
    return !verificar_radical_contido_na_palavra(dica,jogo.senha_atual) && hash_buscar(dica);
}


// thread que fica constantemente esperando mensagens do servidor
void *receive_messages(void *arg)
{
    int socket_fd = *((int*)arg);
    char recebido[BUFFER_SIZE];
    char acumulador[BUFFER_SIZE * 4];
    int usados = 0;

    // loop da thread de recebimento de mensagens
    while (1) {

        int bytes = recv(socket_fd, recebido, sizeof(recebido), 0);
        if (bytes <= 0) {
            printf("\nServidor desconectado.\n");
            exit(0);
        }

        // garante que o acumulador nao exploda
        if (usados + bytes >= (int)sizeof(acumulador))
            usados = 0;

        memcpy(acumulador + usados, recebido, bytes);
        usados += bytes;
        int inicio = 0;

        // procura mensagens terminadas por '\n'
        for (int i = 0; i < usados; i++) {
            if (acumulador[i] == '\n') {
                int tamanho = i - inicio;
                
                if (tamanho >= BUFFER_SIZE) tamanho = BUFFER_SIZE - 1;
                char mensagem[BUFFER_SIZE];
                memcpy(mensagem, acumulador + inicio, tamanho);
                mensagem[tamanho] = '\0';

                // remove '\r' caso exista
                if (tamanho > 0 && mensagem[tamanho - 1] == '\r') {
                    mensagem[tamanho - 1] = '\0';
                }

                if (mensagem[0] != '\0') {
                    processar_servidor(mensagem);
                }
                inicio = i + 1;
            }
        }

        // move para o comeco
        if (inicio > 0) {
            memmove(acumulador, acumulador + inicio, usados - inicio);
            usados -= inicio;
        }
    }
    return NULL;
}

// muda o valor de jogo.estado de forma segura (acesso usando mutex)
void definir_estado(int estado)
{
    pthread_mutex_lock(&mutex_estado);
    jogo.estado = estado;
    pthread_mutex_unlock(&mutex_estado);
}

// retorna o valor de jogo.estado de forma segura (acesso usando mutex)
int obter_estado()
{
    pthread_mutex_lock(&mutex_estado);
    int estado = jogo.estado;
    pthread_mutex_unlock(&mutex_estado);
    return estado;
}

// envia uma mensagem para o servidor
int enviar_linha(const char *texto, int socket)
{
    char buffer[BUFFER_SIZE + 2]; // buffer que guarda a mensagem + 2 chars para um possível /r e/ou /0
    int tamanho = snprintf(buffer, sizeof(buffer), "%s\n", texto);
    int total = 0;
    // garante o envio do texto completo
    while (total < tamanho) {
        int enviados = send(
            socket,
            buffer + total,
            tamanho - total,
            0
        );
        if (enviados <= 0)
            return 0;
        total += enviados;
    }
    return 1;
}

// printa o estado atual do jogo
void mostrar_status()
{
    printf("\n[ TEMPO %02d:%02d | PONTOS DA DUPLA: %d ]\n> ", jogo.tempo_restante / 60, jogo.tempo_restante % 60, jogo.pontos);
    fflush(stdout); //garante impressão no terminal
}

// processa uma senha recebida do servidor, que é do formato SENHA|'palavra'|'radical'|'dificuldade'
void processar_senha(char *mensagem)
{
    char copia[BUFFER_SIZE];

    strncpy(copia, mensagem, sizeof(copia) - 1);
    copia[sizeof(copia) - 1] = '\0'; //garante \0 no final da string 

    // descarta SENHA|
    strtok(copia, "|");

    // separa palavra, radical e dificuldade
    char *palavra = strtok(NULL, "|");
    char *radical = strtok(NULL, "|");
    char *dificuldade = strtok(NULL, "|");

    if (palavra == NULL || radical == NULL || dificuldade == NULL)
        return;

    // guarda a senha
    strncpy(jogo.senha_atual.palavra, palavra, sizeof(jogo.senha_atual.palavra) - 1);
    jogo.senha_atual.palavra[sizeof(jogo.senha_atual.palavra) - 1] = '\0'; //garante \0 no final da string

    // guarda radical
    strncpy(jogo.senha_atual.radical, radical, sizeof(jogo.senha_atual.radical) - 1);
    jogo.senha_atual.radical[sizeof(jogo.senha_atual.radical) - 1] = '\0'; //garante \0 no final da string

    // guarda dificuldade
    jogo.senha_atual.dificuldade = dificuldade[0];

    // muda para o próximo estado 
    definir_estado(DANDO_DICA);

    // mostra senha no terminal
    printf("\nSENHA: %s\n", jogo.senha_atual.palavra);
    printf("Digite uma dica:\n> ");

    fflush(stdout);
}


// processa mensagem recebida do servidor
void processar_servidor(char* msg)
{
    // muda para o estado aguardando nome
    if (strncmp(msg, "NOME_OK", 5) == 0) {
        definir_estado(CHAT_GLOBAL);
        printf(
            "\nCHAT GLOBAL\n"
            "/entrar  - procurar partida\n"
            "/ranking - classificacao\n"
            "/sair    - sair\n\n"
        );
        fflush(stdout);
        return;
    }

    // atualização do cronômetro em 60, 30 e 10 segundos
    if (strncmp(msg, "TEMPO|", 6) == 0) {
        jogo.tempo_restante = atoi(msg + 6);
        if (jogo.tempo_restante == 0)  mostrar_status();
        return;
    }

    // atualização da pontuação
    if (strncmp(msg, "PONTOS|", 7) == 0) {
        jogo.pontos = atoi(msg + 7);
        mostrar_status();
        return;
    }

    // mensagem do chat global.
    if (strncmp(msg, "CHAT|", 5) == 0) {
        printf("\n%s\n> ",msg + 5);
        fflush(stdout);
        return;
    }

    // cliente na fila
    if (strncmp(msg, "FILA|", 5) == 0) {
        printf("\n%s\n",msg + 5);
        return;
    }

    // o servidor encontrou outro jogador
    if (strncmp(msg, "PARTIDA_ENCONTRADA|", 19) == 0) {
        jogo.pontos = 0;
        definir_estado(ESPERANDO_JOGADA);
        printf( "\n=============================\n"
                "       PARTIDA INICIADA\n"
                "=============================\n");

        if (strcmp(msg + 19,"ADIVINHA") == 0) {
            printf("Voce comeca ADIVINHANDO.\n");
        }
        
        else {
            printf("Voce comeca DANDO DICAS.\n");
            printf("Digite \"/pass\" para pular uma senha.\n");
        }
        return;
    }

    // recebe a senha (se for o jogador que dá as dicas)
    if (strncmp(msg, "SENHA|", 6) == 0) {
        processar_senha(msg);
        return;
    }

    // recebe a senha (se for o jogador que dá as dicas)
    if (strncmp(msg, "PASS", 4) == 0) {
        printf("O outro jogador trocou a senha!\n");
        return;
    }

    // recebe a dica (se for o jogador que adivinha a senha)
    if (strncmp(msg, "DICA|", 5) == 0) {
        definir_estado(DANDO_CHUTE);
        printf("\nDICA: %s\n", msg + 5);
        printf("Digite sua tentativa:\n> ");
        fflush(stdout);
        return;
    }

    // confirmação do servidor de que a dica foi entregue
    if (strcmp(msg, "DICA_OK") == 0) {
        definir_estado(ESPERANDO_JOGADA);
        printf("\nDica enviada. Aguardando tentativa...\n");
        return;
    }

    // se a tentativa for errada
    if (strcmp(msg, "ERROU") == 0) {
        definir_estado(ESPERANDO_JOGADA);
        printf("\nErrou! Aguarde nova dica.\n");
        return;
    }


    // solicita outro dica, caso o outro jogador tenha errado o chute
    if (strncmp(msg, "NOVA_DICA|", 10) == 0) {
        definir_estado(DANDO_DICA);
        printf("\nO outro jogador chutou \"%s\".\nDigite outra dica para \"%s\":\n\n> ", msg+10, jogo.senha_atual.palavra);
        fflush(stdout);
        return;
    }

    // quando a dupla acertar a palavra
    if (strcmp(msg, "ACERTOU") == 0) {
        definir_estado(ESPERANDO_JOGADA);
        printf("\n*** ACERTOU! ***\n");
        return;
    }

    // fim do primeiro tempo, ambos os jogadores precisam confirmar para continuar
    if (strcmp(msg, "FIM_TEMPO") == 0) {
        definir_estado(CONFIRMANDO);
        printf(
            "\n=============================\n"
            "      FIM DOS 60 SEGUNDOS\n"
            "=============================\n"
        );
        printf("Pontuacao atual da dupla: %d\n", jogo.pontos);
        printf("Os papeis serao invertidos.\n");
        printf("Digite /confirmar quando estiver pronto.\n> ");
        fflush(stdout);
        return;
    }

    // o primeiro jogador confirmou
    if (strcmp(msg,"CONFIRMACAO_OK") == 0) {
        definir_estado(ESPERANDO_CONFIRMACAO);
        printf("\nConfirmado. Aguardando seu parceiro...\n");
        return;
    }

    // início do segundo tempo, após ambos terem confirmado
    if (strncmp(msg, "SEGUNDO_TEMPO|", 14) == 0) {
        definir_estado(ESPERANDO_JOGADA);
        printf(
            "\n=============================\n"
            "        SEGUNDO TEMPO\n"
            "=============================\n"
        );
        if (strcmp(msg + 14, "ADIVINHA") == 0) {
            printf("Agora voce ADIVINHA.\n");
        } else {
            printf("Agora voce DA AS DICAS.\n");
            printf("Digite \"/pass\" para pular uma senha.\n");
        }
        return;
    }

    // fim do segudo tempo, ambos voltam para o lobby (chat global)
    if (strncmp(msg, "FIM_PARTIDA|", 12) == 0) {
        definir_estado(CHAT_GLOBAL);
        printf(
            "\n=============================\n"
            "        FIM DA PARTIDA\n"
            "=============================\n"
        );
        printf("%s\n", msg + 12);
        printf("\nVoce voltou ao chat global.\n/entrar  - jogar novamente\n/ranking - ver classificacao\n> ");
        fflush(stdout);
        return;
    }

    // quando chamar a funcao /ranking
    if (strcmp(msg, "RANKING_INICIO") == 0) {
        printf( "\n=============================\n"
                "         CLASSIFICACAO\n"
                "=============================\n");
        return;
    }

    // para cada linha do ranking
    if (strncmp(msg, "RANKING|", 8) == 0) {
        printf("%s\n", msg + 8);
        return;
    }

    // fim do ranking
    if (strcmp(msg, "RANKING_FIM") == 0) {
        printf("> ");
        fflush(stdout);
        return;
    }

    // caso a mensagem não seja conhecida, ignora
    printf("\n%s\n> ", msg);
    fflush(stdout);
}