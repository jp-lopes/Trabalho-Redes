#ifndef MEGA_SENHA_H
#define MEGA_SENHA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "hash_palavras.h"
#include "funcoes_server.h"

#define PATH_SENHAS "data/palavras_senha_5000.csv"
#define QTD_TOTAL_SENHAS 5000
#define QTD_TOTAL_SENHAS_FACEIS 2000
#define QTD_TOTAL_SENHAS_MEDIAS 2000
#define QTD_TOTAL_SENHAS_DIFICEIS 1000
#define TAM_PALAVRA 50

typedef struct {
    char palavra[TAM_PALAVRA];
    char radical[TAM_PALAVRA];
    char dificuldade;
} Senha;

void carregar_senhas(char* path_senhas);

Senha sortear_senha();

char sortear_dificuldade();

int verificar_dica_valida(char* dica, Senha s);

int verificar_tentativa(char* tentativa);

int compara_tentativa_e_senha(char* tentativa, Senha s);

int verificar_senha_repetida(Partida p, Senha s);

int verificar_radical_contido_na_palavra(char* palavra, Senha s);

void formatar_string(char *str);

#endif
