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
#define ESPERANDO_NOME          7
#define ESPERANDO_NOME_OK       8
// estado do cliente de um processo client.c
typedef struct {
    int estado;
    int pontos;
    int tempo_restante;
    Senha senha_atual;
} EstadoCliente;

// thread que fica constantemente esperando mensagens do servidor
void *receive_messages(void *arg);

// muda o valor de jogo.estado de forma segura (acesso usando mutex)
void definir_estado(int estado);

// retorna o valor de jogo.estado de forma segura (acesso usando mutex)
int obter_estado();

// envia uma mensagem para o servidor
int enviar_linha(const char *texto, int socket);

// printa o estado atual do jogo
void mostrar_status();

// processa uma senha recebida do servidor, que é do formato SENHA|'palavra'|'radical'|'dificuldade'
void processar_senha(char *mensagem);

// processa mensagem recebida do servidor
void processar_servidor(char *msg);

// verifica se a dica é válida, isto é, se existe no dicionário e se não contém o radical da senha
int verificar_dica_valida(char* dica);

#endif
