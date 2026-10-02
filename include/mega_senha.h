#ifndef MEGA_SENHA_H
#define MEGA_SENHA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "hash_palavras.h"

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

// abre arquivo de senhas e carrega todas na memória, separadas por dificuldade
void carregar_senhas(char* path_senhas);

// retorna uma senha aleatória
Senha sortear_senha();

// sorteia uma dificuldade com diferentes probabilidades
char sortear_dificuldade();

// verifica se a tentativa é válida, isto é, se existe no dicionário
int verificar_tentativa(char* tentativa);

// retorna 1 se a pessoa acertou a senha, e 0 caso contrário
int compara_tentativa_e_senha(char* tentativa, Senha s);

// Verifica se a palavra contém o radical da senha, retorna 1 se contiver e 0 se não contiver
int verificar_radical_contido_na_palavra(char* palavra, Senha s);

// Função que remove os acentos de uma string e converte para letras minúsculas
void formatar_string(char *str);

#endif
