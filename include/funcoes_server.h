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
//estados possíveis de um cliente para o server
#define CHAT_GLOBAL             0
#define NA_FILA                 1
#define DANDO_DICA              2
#define ESPERANDO_DICA          3
#define DANDO_CHUTE             4
#define ESPERANDO_CHUTE         5
#define CONFIRMANDO             6
#define ESPERANDO_CONFIRMACAO   7

//struct que guarda as informações de um cliente
typedef struct {
    int socket;
    int id;
    int id_partida;
    int estado;
    int conectado;
} Client;

//struct qyue guarda informações da partida
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
    int pontos;
} Resultado;


//informações passadas para uma thread de cronômetro.
typedef struct {
    int id_partida;
    int geracao;
} TimerArgs;


//informação passada para a thread que atende um cliente.
typedef struct {
    int id_cliente;
} ThreadClientArgs;


#endif
