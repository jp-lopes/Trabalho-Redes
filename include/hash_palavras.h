#ifndef HASH_PALAVRAS_H
#define HASH_PALAVRAS_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASH_TABLE_SIZE 250007u

#define PATH_DICIONARIO "data/dicionario_validacao_80000.csv"


typedef struct HashNode {
    char *palavra;
    struct HashNode *prox;
} HashNode;

/* Le o CSV e monta a tabela hash.
   Retorna 1 em caso de sucesso e 0 em caso de erro. */
int hash_inicializar(const char *caminho_csv);

/* Insere uma palavra na tabela.
   Retorna 1 se inseriu e 0 se a palavra ja existia ou ocorreu erro. */
int hash_inserir(const char *palavra);

/* Retorna 1 se a palavra existe e 0 caso contrario. */
int hash_buscar(const char *palavra);

/* Retorna o indice calculado pela funcao hash. */
size_t hash_indice(const char *palavra);

/* Libera toda a memoria alocada. */
void hash_liberar(void);

/* Quantidade de palavras atualmente carregadas. */
size_t hash_quantidade(void);

#endif
