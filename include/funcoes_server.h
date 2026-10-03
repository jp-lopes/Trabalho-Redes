#ifndef FUNCOES_SERVER_H
#define FUNCOES_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "mega_senha.h"
#include "hash_palavras.h"

//constantes do server
#define PORT 8080
#define MAX_CLIENTS 100
#define MAX_PARTIDAS 50
#define MAX_RANKING 1000
#define BUFFER_SIZE 1024
#define TAM_MAX_NOME_CLIENTE 30
//estados possíveis de um cliente para o server
#define CHAT_GLOBAL             0
#define NA_FILA                 1
#define DANDO_DICA              2
#define ESPERANDO_DICA          3
#define DANDO_CHUTE             4
#define ESPERANDO_CHUTE         5
#define CONFIRMANDO             6
#define ESPERANDO_CONFIRMACAO   7
#define ESPERANDO_NOME          8

//struct que guarda as informações de um cliente
typedef struct {
    int socket;
    int id;
    int id_partida;
    int estado;
    int conectado;
    char nome[TAM_MAX_NOME_CLIENTE];
} Client;

//struct que guarda informações da partida
typedef struct {
    int id_partida;
    int id_client_1;
    int id_client_2;
    int pontos;
    int id_cliente_adivinha;
    int id_cliente_dica;
    Senha senha_atual;
    Senha historico[100];
    int qtd_historico;
    int tempo;
    int client_1_confirmou;
    int client_2_confirmou;
    int geracao_timer;
    int ativa;
} Partida;


//resultado de uma partida (ranking)
typedef struct {
    int id_client_1;
    int id_client_2;
    char nome_cliente_1[TAM_MAX_NOME_CLIENTE];
    char nome_cliente_2[TAM_MAX_NOME_CLIENTE];
    int pontos;
} Resultado;


//informações passadas para uma thread de cronômetro.
typedef struct {
    int id_partida;
    int geracao;
} TimerArgs;

// vetor com todos os clientes conectados
extern Client clients[MAX_CLIENTS];

// vetor com todas as partidas que podem estar acontecendo ao mesmo tempo
extern Partida partidas[MAX_PARTIDAS];

// vetor que guarda os resultados das partidas finalizadas
extern Resultado ranking[MAX_RANKING];

extern int qtd_ranking;
extern int proximo_id_cliente;
extern int proximo_id_partida;

// mutex para proteger clients, partidas e ranking
extern pthread_mutex_t mutex;

// envia uma mensagem completa para um socket
int enviar_mensagem_socket(int socket, const char *mensagem);

// procura um cliente conectado pelo id
Client* buscar_cliente_id(int id);

// procura uma partida ativa pelo id
Partida* buscar_partida_id(int id);

// thread responsável por receber as mensagens de um cliente
void *handle_client(void *arg);

// procura outro jogador que esteja esperando na fila
Client *procurar_jogador_fila(int ignorar);

// comparação utilizada pelo qsort para ordenar o ranking do maior para o menor
int comparar_resultados(const void *a, const void *b);

// salva o resultado de uma partida no ranking
void registrar_ranking(Partida *p);

// envia o ranking para um cliente
void enviar_ranking(int socket);

// envia uma mensagem para todos que estão no chat global, menos quem enviou
void broadcast_chat(int id_remetente, const char *texto);

// sorteia uma senha que ainda não foi utilizada nessa partida
Senha nova_senha(Partida *p);

// envia a senha apenas para o cliente responsável pelas dicas
void enviar_senha(Partida *p);

// envia a pontuação atual da dupla para os dois clientes
void enviar_pontos(Partida *p);

// finaliza a partida e devolve os dois jogadores para o chat global
void finalizar_partida(Partida *p);

// thread responsável pelo cronômetro de 60 segundos de uma partida
void* thread_timer(void *arg);

// inicia o cronômetro de uma partida
void iniciar_timer(Partida *p);

// cria uma partida entre dois clientes
void criar_partida(Client *primeiro, Client *segundo);

// coloca um cliente na fila ou cria uma partida caso já exista alguém esperando
void entrar_fila(int id);

// recebe uma dica e envia para o cliente que está tentando adivinhar
void processar_dica(int id, const char *dica);

// recebe um chute e compara com a senha atual
void processar_chute(int id, char *tentativa);

// inicia os segundos 60 segundos da partida
void iniciar_segundo_tempo(Partida *p);

// registra a confirmação de um cliente para começar o segundo tempo
void confirmar(int id);

// processa uma mensagem recebida de um cliente
void processar_mensagem(int id, char *mensagem);

// remove um cliente do servidor e trata uma possível partida em andamento
void desconectar(int id);


#endif
