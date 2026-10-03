#include "funcoes_server.h"

Client clients[MAX_CLIENTS];
Partida partidas[MAX_PARTIDAS];
Resultado ranking[MAX_RANKING];

int qtd_ranking = 0;
int proximo_id_cliente = 1;
int proximo_id_partida = 1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// envia uma mensagem completa para um socket
int enviar_mensagem_socket(int socket, const char *mensagem)
{
    //coloca mensagem no buffer
    char buffer[BUFFER_SIZE + 2];
    int tamanho = snprintf(buffer, sizeof(buffer), "%s\n", mensagem);
    int total = 0;  //variavel para garantir o envio da mensagem completa

    //tratamento de erro
    if (tamanho <= 0 || tamanho >= (int)sizeof(buffer))
        return 0;

    // garante que a mensagem inteira seja enviada
    while (total < tamanho) {
        int enviados = send(socket,buffer + total,tamanho - total,0);
        if (enviados <= 0) return 0;
        total += enviados;
    }

    return 1;
}

// procura um cliente conectado pelo id
Client* buscar_cliente_id(int id)
{
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].conectado && clients[i].id == id)
            return &clients[i];
    }

    return NULL;
}

// procura uma partida ativa pelo id
Partida* buscar_partida_id(int id)
{
    for (int i = 0; i < MAX_PARTIDAS; i++) {
        if (partidas[i].ativa && partidas[i].id_partida == id)
            return &partidas[i];
    }

    return NULL;
}

// procura outro jogador que esteja esperando na fila
Client *procurar_jogador_fila(int ignorar)
{
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].conectado && clients[i].id != ignorar && clients[i].estado == NA_FILA) {
            return &clients[i];
        }
    }

    return NULL;
}


// comparação utilizada pelo qsort para ordenar o ranking do maior para o menor
int comparar_resultados(const void *a, const void *b)
{
    const Resultado *r1 = a;
    const Resultado *r2 = b;
    return r2->pontos - r1->pontos;
}


// salva o resultado de uma partida no ranking
void registrar_ranking(Partida *p)
{
    if (qtd_ranking >= MAX_RANKING)
        return;

    pthread_mutex_lock(&mutex);
    
    Client* c1 = buscar_cliente_id(p->id_client_1);
    Client* c2 = buscar_cliente_id(p->id_client_2);

    if(c1 == NULL || c2 == NULL) {
        pthread_mutex_unlock(&mutex);
        return;
    }

    ranking[qtd_ranking].id_client_1 = p->id_client_1;
    ranking[qtd_ranking].id_client_2 = p->id_client_2;
    strcpy(ranking[qtd_ranking].nome_cliente_1, c1->nome);
    strcpy(ranking[qtd_ranking].nome_cliente_2, c2->nome);
    ranking[qtd_ranking].pontos = p->pontos;

    qtd_ranking++;

    // ordena o ranking pela pontuação
    qsort(ranking, qtd_ranking,sizeof(Resultado), comparar_resultados);

    pthread_mutex_unlock(&mutex);
}

// envia o ranking para um cliente
void enviar_ranking(int socket)
{
    char mensagem[BUFFER_SIZE];
    enviar_mensagem_socket(socket, "RANKING_INICIO");

    // mostra no máximo os 10 melhores resultados
    int limite = qtd_ranking < 10 ? qtd_ranking : 10;
    if (limite == 0) {
        enviar_mensagem_socket(socket, "RANKING|Nenhuma partida registrada.");
    }

    for (int i = 0; i < limite; i++) {
        snprintf(mensagem, sizeof(mensagem), "RANKING|%d. Jogadores \"%s\" e \"%s\" - %d pontos", i + 1, ranking[i].nome_cliente_1, ranking[i].nome_cliente_2, ranking[i].pontos);
        enviar_mensagem_socket(socket, mensagem);
    }
    enviar_mensagem_socket(socket, "RANKING_FIM");
}


// envia uma mensagem para todos que estão no chat global, menos quem enviou
void broadcast_chat(int id_remetente, const char *texto)
{
    pthread_mutex_lock(&mutex);
    Client *remetente = buscar_cliente_id(id_remetente);
    if (remetente == NULL) {
        pthread_mutex_unlock(&mutex);
        return;
    }

    char mensagem[BUFFER_SIZE];
    snprintf(mensagem, sizeof(mensagem), "CHAT|%s: %s",remetente->nome, texto);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].conectado && clients[i].estado == CHAT_GLOBAL && clients[i].id != id_remetente) {
            enviar_mensagem_socket(clients[i].socket, mensagem);
        }
    }
    pthread_mutex_unlock(&mutex);
}


// sorteia uma senha que ainda não foi utilizada nessa partida
Senha nova_senha(Partida *p)
{
    Senha senha;
    int repetida = 1;
    
    while(repetida && p->qtd_historico < 100){
        senha = sortear_senha();
        repetida = 0;
        // verifica se a senha já apareceu nessa partida
        for (int i = 0; i < p->qtd_historico; i++) {
            if (strcmp(senha.palavra,p->historico[i].palavra) == 0) {
                repetida = 1;
                break;
            }
        } 
    }

    // salva a nova senha no histórico
    if (p->qtd_historico < 100) {p->historico[p->qtd_historico] = senha;
        p->qtd_historico++;
    }
    return senha;
}

// envia a senha apenas para o cliente responsável pelas dicas
void enviar_senha(Partida *p)
{
    Client *cliente_dica = buscar_cliente_id(p->id_cliente_dica);
    if (cliente_dica == NULL)
        return;

    char mensagem[BUFFER_SIZE];
    snprintf(mensagem, sizeof(mensagem), "SENHA|%s|%s|%c", p->senha_atual.palavra, p->senha_atual.radical, p->senha_atual.dificuldade);
    cliente_dica->estado = DANDO_DICA;
    enviar_mensagem_socket(cliente_dica->socket, mensagem);
}

// envia a pontuação atual da dupla para os dois clientes
void enviar_pontos(Partida *p)
{
    char mensagem[100];
    snprintf(mensagem, sizeof(mensagem), "PONTOS|%d", p->pontos);
    Client *client_1 = buscar_cliente_id(p->id_client_1);
    Client *client_2 = buscar_cliente_id(p->id_client_2);

    if (client_1 != NULL) enviar_mensagem_socket(client_1->socket, mensagem);
    if (client_2 != NULL) enviar_mensagem_socket(client_2->socket, mensagem);
}


// finaliza a partida e devolve os dois jogadores para o chat global
void finalizar_partida(Partida *p)
{
    Client *client_1 = buscar_cliente_id(p->id_client_1);
    Client *client_2 = buscar_cliente_id(p->id_client_2);

    // salva a pontuação da dupla no ranking
    registrar_ranking(p);
    char mensagem[256];
    snprintf(mensagem, sizeof(mensagem), "FIM_PARTIDA|Pontuacao final da dupla: %d", p->pontos
    );

    if (client_1 != NULL) {
        enviar_mensagem_socket(client_1->socket, mensagem);
        client_1->estado = CHAT_GLOBAL;
        client_1->id_partida = -1;
    }

    if (client_2 != NULL) {
        enviar_mensagem_socket(client_2->socket, mensagem);
        client_2->estado = CHAT_GLOBAL;
        client_2->id_partida = -1;
    }

    printf(
        "Partida %d terminou com %d pontos.\n",
        p->id_partida,
        p->pontos
    );

    // libera essa posição do vetor de partidas
    p->ativa = 0;

    // invalida qualquer cronômetro antigo dessa partida
    p->geracao_timer++;
}


// thread responsável pelo cronômetro de 60 segundos de uma partida
void* thread_timer(void *arg)
{
    TimerArgs dados = *((TimerArgs *)arg);
    free(arg);

    // conta de 60 até 0
    for (int restante = 60; restante >= 0; restante--) {
        pthread_mutex_lock(&mutex);

        Partida *p = buscar_partida_id(dados.id_partida);

        // verifica se a partida ainda existe e se este timer ainda é válido
        if (p == NULL || p->geracao_timer != dados.geracao) {
            pthread_mutex_unlock(&mutex);
            return NULL;
        }

        Client *client_1 = buscar_cliente_id(p->id_client_1);
        Client *client_2 = buscar_cliente_id(p->id_client_2);

        char mensagem[100];
        snprintf(mensagem, sizeof(mensagem), "TEMPO|%d", restante);

        // manda o cronômetro para os dois clientes
        if (client_1 != NULL)
            enviar_mensagem_socket(client_1->socket, mensagem);

        if (client_2 != NULL)
            enviar_mensagem_socket(client_2->socket, mensagem);

        // acabou um período de 60 segundos
        if (restante == 0) {
            if (p->tempo == 1) {
                p->client_1_confirmou = 0;
                p->client_2_confirmou = 0;

                if (client_1 != NULL) {
                    client_1->estado = CONFIRMANDO;
                    enviar_mensagem_socket(client_1->socket, "FIM_TEMPO");
                }

                if (client_2 != NULL) {
                    client_2->estado = CONFIRMANDO;
                    enviar_mensagem_socket(client_2->socket, "FIM_TEMPO");
                }

                pthread_mutex_unlock(&mutex);
                return NULL;
            }

            // terminou o segundo tempo
            pthread_mutex_unlock(&mutex);
            finalizar_partida(p);
            return NULL;
        }
        pthread_mutex_unlock(&mutex);
        sleep(1);
    }
    return NULL;
}

// inicia o cronômetro de uma partida
void iniciar_timer(Partida *p)
{
    TimerArgs *args = malloc(sizeof(TimerArgs));
    if (args == NULL) return;

    // aumenta a geração para diferenciar esse timer de timers antigos
    p->geracao_timer++;
    args->id_partida = p->id_partida;
    args->geracao = p->geracao_timer;
    pthread_t thread;

    if (pthread_create(&thread, NULL, thread_timer, args) != 0) {
        free(args);
        return;
    }
    pthread_detach(thread);
}

// cria uma partida entre dois clientes
void criar_partida(Client *primeiro, Client *segundo)
{
    Partida *p = NULL;
    
    // procura uma posição livre no vetor de partidas
    for (int i = 0; i < MAX_PARTIDAS; i++) {
        if (!partidas[i].ativa) {
            p = &partidas[i];
            break;
        }
    }

    // não existe espaço para uma nova partida
    if (p == NULL) {
        primeiro->estado = CHAT_GLOBAL;
        segundo->estado = CHAT_GLOBAL;

        enviar_mensagem_socket(primeiro->socket, "ERRO|Nao ha espaco para novas partidas.");
        enviar_mensagem_socket(segundo->socket, "ERRO|Nao ha espaco para novas partidas.");

        return;
    }

    // limpa os valores antigos dessa posição do vetor
    memset(p, 0, sizeof(Partida));

    p->ativa = 1;
    p->id_partida = proximo_id_partida++;

    p->id_client_1 = primeiro->id;
    p->id_client_2 = segundo->id;

    // pontuação é da dupla
    p->pontos = 0;

    // começa no primeiro período de 60 segundos
    p->tempo = 1;

    // quem entrou primeiro na fila começa adivinhando
    p->id_cliente_adivinha = primeiro->id;
    p->id_cliente_dica = segundo->id;

    // associa os clientes à partida
    primeiro->id_partida = p->id_partida;
    segundo->id_partida = p->id_partida;

    primeiro->estado = ESPERANDO_DICA;
    segundo->estado = DANDO_DICA;

    // sorteia a primeira senha
    p->senha_atual = nova_senha(p);

    // avisa para cada cliente qual será seu papel
    enviar_mensagem_socket(primeiro->socket, "PARTIDA_ENCONTRADA|ADIVINHA");
    enviar_mensagem_socket(segundo->socket, "PARTIDA_ENCONTRADA|DICA");

    // manda a pontuação inicial
    enviar_pontos(p);

    // manda a senha somente para quem vai dar a dica
    enviar_senha(p);

    printf("Partida %d criada entre os clientes %d e %d.\n", p->id_partida, primeiro->id, segundo->id);

    // começa o primeiro cronômetro de 60 segundos
    iniciar_timer(p);
}

// coloca um cliente na fila ou cria uma partida caso já exista alguém esperando
void entrar_fila(int id)
{
    pthread_mutex_lock(&mutex);
    Client *cliente = buscar_cliente_id(id);

    // só pode entrar na fila se estiver no chat global
    if (cliente == NULL || cliente->estado != CHAT_GLOBAL) {
        pthread_mutex_unlock(&mutex);
        return;
    }

    Client *oponente = procurar_jogador_fila(id);

    // se não encontrou ninguém, fica esperando
    if (oponente == NULL) {
        cliente->estado = NA_FILA;
        enviar_mensagem_socket(cliente->socket, "FILA|Aguardando outro jogador...");
    }

    // se encontrou alguém, cria uma partida
    else {
        // o oponente já estava na fila, portanto entrou primeiro
        criar_partida(oponente, cliente);
    }
    pthread_mutex_unlock(&mutex);
}

// recebe uma dica e envia para o cliente que está tentando adivinhar
void processar_dica(int id, const char *dica)
{
    pthread_mutex_lock(&mutex);

    Client *cliente = buscar_cliente_id(id);

    // só aceita a mensagem se realmente for a vez desse cliente dar uma dica
    if (cliente == NULL || cliente->estado != DANDO_DICA) {
        pthread_mutex_unlock(&mutex);
        return;
    }

    Partida *p = buscar_partida_id(cliente->id_partida);

    if (p == NULL || p->id_cliente_dica != id) {
        pthread_mutex_unlock(&mutex);
        return;
    }

    Client *cliente_adivinha = buscar_cliente_id(
        p->id_cliente_adivinha
    );

    if (cliente_adivinha == NULL) {
        pthread_mutex_unlock(&mutex);
        return;
    }

    char mensagem[BUFFER_SIZE];
    snprintf(mensagem, sizeof(mensagem), "DICA|%s", dica);

    // manda a dica para o jogador que está adivinhando
    enviar_mensagem_socket(cliente_adivinha->socket, mensagem);

    // confirma para o outro jogador que a dica foi enviada
    enviar_mensagem_socket(cliente->socket, "DICA_OK");

    // agora quem deu a dica espera e o outro pode chutar
    cliente->estado = ESPERANDO_CHUTE;
    cliente_adivinha->estado = DANDO_CHUTE;

    pthread_mutex_unlock(&mutex);
}


// recebe um chute e compara com a senha atual
void processar_chute(int id, char *tentativa)
{
    pthread_mutex_lock(&mutex);

    Client *cliente = buscar_cliente_id(id);

    // só aceita chute se realmente for a vez desse cliente
    if (cliente == NULL || cliente->estado != DANDO_CHUTE) {
        pthread_mutex_unlock(&mutex);
        return;
    }

    Partida *p = buscar_partida_id(cliente->id_partida);

    if (p == NULL || p->id_cliente_adivinha != id) {
        pthread_mutex_unlock(&mutex);
        return;
    }

    Client *cliente_dica = buscar_cliente_id(p->id_cliente_dica);

    if (cliente_dica == NULL) {
        pthread_mutex_unlock(&mutex);
        return;
    }

    // verifica no servidor se o jogador acertou a senha
    if (compara_tentativa_e_senha(tentativa, p->senha_atual)) {

        // soma um ponto para a dupla
        p->pontos++;
        enviar_mensagem_socket(cliente->socket, "ACERTOU");
        enviar_mensagem_socket(cliente_dica->socket, "ACERTOU");

        // atualiza a pontuação nos dois clientes
        enviar_pontos(p);

        // sorteia uma nova senha
        p->senha_atual = nova_senha(p);

        cliente->estado = ESPERANDO_DICA;
        cliente_dica->estado = DANDO_DICA;

        // envia a nova senha para quem dá dicas
        enviar_senha(p);
    }

    // se errou, continua com a mesma senha
    else {
        // avisa que o jogador que chuta chutou errado
        enviar_mensagem_socket(cliente->socket, "ERROU");
        // avisa para o jogador que dá dicas que o outro jogador errou, e passa a tentativa errada para ele
        char mensagem[BUFFER_SIZE+10];
        snprintf(mensagem, sizeof(mensagem), "NOVA_DICA|%s", tentativa);
        enviar_mensagem_socket(cliente_dica->socket, mensagem);

        cliente->estado = ESPERANDO_DICA;
        cliente_dica->estado = DANDO_DICA;
    }
    pthread_mutex_unlock(&mutex);
}

// inicia os segundos 60 segundos da partida
void iniciar_segundo_tempo(Partida *p)
{
    // troca quem adivinha e quem dá as dicas
    int antigo_adivinha = p->id_cliente_adivinha;

    p->id_cliente_adivinha = p->id_cliente_dica;
    p->id_cliente_dica = antigo_adivinha;
    p->tempo = 2;

    Client *cliente_adivinha = buscar_cliente_id(p->id_cliente_adivinha);
    Client *cliente_dica = buscar_cliente_id(p->id_cliente_dica);

    if (cliente_adivinha == NULL || cliente_dica == NULL)
        return;

    cliente_adivinha->estado = ESPERANDO_DICA;
    cliente_dica->estado = DANDO_DICA;

    // começa o segundo tempo com uma nova senha
    p->senha_atual = nova_senha(p);

    enviar_mensagem_socket(cliente_adivinha->socket, "SEGUNDO_TEMPO|ADIVINHA");
    enviar_mensagem_socket(cliente_dica->socket, "SEGUNDO_TEMPO|DICA");

    // manda a senha somente para quem agora dá as dicas
    enviar_senha(p);

    // começa um novo cronômetro de 60 segundos
    iniciar_timer(p);
}

// registra a confirmação de um cliente para começar o segundo tempo
void confirmar(int id)
{
    pthread_mutex_lock(&mutex);
    Client *cliente = buscar_cliente_id(id);

    if (cliente == NULL || cliente->estado != CONFIRMANDO) {
        pthread_mutex_unlock(&mutex);
        return;
    }

    Partida *p = buscar_partida_id(cliente->id_partida);

    if (p == NULL) {
        pthread_mutex_unlock(&mutex);
        return;
    }

    // registra qual dos dois clientes confirmou
    if (id == p->id_client_1) p->client_1_confirmou = 1;
    if (id == p->id_client_2) p->client_2_confirmou = 1;

    cliente->estado = ESPERANDO_CONFIRMACAO;
    enviar_mensagem_socket(cliente->socket, "CONFIRMACAO_OK");

    // só começa quando os dois tiverem confirmado
    if (p->client_1_confirmou && p->client_2_confirmou) iniciar_segundo_tempo(p);
    pthread_mutex_unlock(&mutex);
}


// processa uma mensagem recebida de um cliente
void processar_mensagem(int id, char *mensagem)
{
    pthread_mutex_lock(&mutex);
    Client *cliente = buscar_cliente_id(id);

    if (cliente == NULL) {
        pthread_mutex_unlock(&mutex);
        return;
    }

    // salva o estado porque as funções abaixo também utilizam o mutex
    int estado = cliente->estado;
    pthread_mutex_unlock(&mutex);

    // nome do cliente recebido
    if (estado == ESPERANDO_NOME) {
        if(strncmp(mensagem, "NOME|", 5) == 0) {
            pthread_mutex_lock(&mutex);
            Client *c = buscar_cliente_id(id);
            if (c == NULL){
                pthread_mutex_unlock(&mutex);
                return;
            }
            strncpy(c->nome, mensagem+5, TAM_MAX_NOME_CLIENTE-1);
            c->nome[TAM_MAX_NOME_CLIENTE-1] = '\0';
            c->estado = CHAT_GLOBAL;
            printf("Cliente %d escolheu o apelido %s\n", c->id, c->nome);
            enviar_mensagem_socket(c->socket, "NOME_OK");
            pthread_mutex_unlock(&mutex);
        }
        return;
    }

    // comandos e mensagens do chat global
    if (estado == CHAT_GLOBAL) {

        if (strcmp(mensagem, "/entrar") == 0) {
            entrar_fila(id);
        }

        else if (strcmp(mensagem, "/ranking") == 0) {
            pthread_mutex_lock(&mutex);

            Client *c = buscar_cliente_id(id);

            if (c != NULL) enviar_ranking(c->socket);
            pthread_mutex_unlock(&mutex);
        }

        else {
            broadcast_chat(id, mensagem);
        }

        return;
    }

    // recebe dica
    if (estado == DANDO_DICA){
        if (strcmp(mensagem, "DICA|/pass") == 0) {
            Client *c = buscar_cliente_id(id);
            Partida *p = buscar_partida_id(c->id_partida);
            // troca a senha e envia para os jogadores
            p->senha_atual = nova_senha(p);
            enviar_senha(p);
            return;
        }
        if(strncmp(mensagem, "DICA|", 5) == 0) {
            processar_dica(id, mensagem + 5);
            return;
        }
    }

    // recebe chute
    if (estado == DANDO_CHUTE &&
        strncmp(mensagem, "CHUTE|", 6) == 0) {
        processar_chute(id, mensagem + 6
        );
        return;
    }

    // recebe confirmação para o segundo tempo
    if (estado == CONFIRMANDO &&
        strcmp(mensagem, "/confirmar") == 0) {
        confirmar(id);
    }
}

// remove um cliente do servidor e trata uma possível partida em andamento
void desconectar(int id)
{
    pthread_mutex_lock(&mutex);
    Client *cliente = buscar_cliente_id(id);

    if (cliente == NULL) {
        pthread_mutex_unlock(&mutex);
        return;
    }

    int socket = cliente->socket;

    // verifica se o cliente estava jogando
    if (cliente->id_partida != -1) {
        Partida *p = buscar_partida_id(cliente->id_partida);

        if (p != NULL) {

            // descobre qual era o outro jogador
            int outro_id;
            if (p->id_client_1 == id) 
                outro_id = p->id_client_2 ;
            else 
                outro_id = p->id_client_1;
                
            Client *outro = buscar_cliente_id(outro_id);

            // devolve o outro jogador para o chat global
            if (outro != NULL) {
                enviar_mensagem_socket(outro->socket, "FIM_PARTIDA|O outro jogador desconectou.");
                outro->estado = CHAT_GLOBAL;
                outro->id_partida = -1;
            }

            // encerra a partida e invalida seu cronômetro
            p->ativa = 0;
            p->geracao_timer++;
        }
    }

    cliente->conectado = 0;
    pthread_mutex_unlock(&mutex);
    close(socket);
}

// thread responsável por receber as mensagens de um cliente
void* handle_client(void* arg)
{
    // recupera id do argumento
    int id = *((int*) arg);

    // busca informações do cliente no vetor
    pthread_mutex_lock(&mutex);
    Client *cliente = buscar_cliente_id(id);
    if (cliente == NULL) {
        pthread_mutex_unlock(&mutex);
        return NULL;
    }
    int socket = cliente->socket;
    pthread_mutex_unlock(&mutex);

    // avisa cliente que ele está no chat global
    // enviar_mensagem_socket(socket,"CHAT|Voce entrou no chat global.");
    char recebido[BUFFER_SIZE];

    // acumulador usado porque o TCP não garante uma mensagem completa em cada recv
    char acumulador[BUFFER_SIZE * 4];
    int usados = 0;

    // thread de recebimento de mensagens do cliente
    while (1) {
        int bytes = recv(socket, recebido, sizeof(recebido), 0);

        // cliente fechou a conexão ou ocorreu algum erro
        if (bytes <= 0) break;

        // proteção para não ultrapassar o tamanho do acumulador
        if (usados + bytes >= (int)sizeof(acumulador)) usados = 0;
        memcpy(acumulador + usados, recebido, bytes);
        usados += bytes;
        int inicio = 0;

        // procura mensagens completas separadas por \n
        for (int i = 0; i < usados; i++) {
            if (acumulador[i] == '\n') {
                int tamanho = i - inicio;
                if (tamanho >= BUFFER_SIZE) tamanho = BUFFER_SIZE - 1;
                char mensagem[BUFFER_SIZE];
                memcpy(mensagem, acumulador + inicio, tamanho);
                mensagem[tamanho] = '\0';

                // remove \r se existir
                if (tamanho > 0 && mensagem[tamanho - 1] == '\r') mensagem[tamanho - 1] = '\0';
                if (mensagem[0] != '\0') processar_mensagem(id, mensagem);
                inicio = i + 1;
            }
        }

        // guarda apenas o pedaço que ainda não formou uma mensagem completa
        if (inicio > 0) {
            memmove(acumulador, acumulador + inicio, usados - inicio);
            usados -= inicio;
        }
    }

    printf("Cliente %d desconectou.\n", id);
    desconectar(id);
    return NULL;
}