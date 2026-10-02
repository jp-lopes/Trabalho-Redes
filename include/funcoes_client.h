#ifndef FUNCOES_CLIENT_H
#define FUNCOES_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "hash_palavras.h"
#include "mega_senha.h"

//constantes do client
#define PORT 8080
#define BUFFER_SIZE 1024
// estados possíveis do cliente
#define CHAT_GLOBAL             0
#define ESPERANDO_PARTIDA       1
#define ESPERANDO_JOGADA        2
#define DANDO_DICA              3
#define DANDO_CHUTE             4
#define CONFIRMANDO             5
#define ESPERANDO_CONFIRMACAO   6
// estado do cliente de um processo client.c
typedef struct {
    int estado;
    int pontos;
    int tempo_restante;
    Senha senha_atual;
} EstadoCliente;



#endif
