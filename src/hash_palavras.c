#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "hash_palavras.h"

/*
 * Seed escolhida previamente testando esta funcao sobre o
 * dicionario do projeto.
 */
#define HASH_SEED UINT64_C(0x9e3779b97f4a7c15)

/*
 * Cada posicao da tabela aponta para o inicio de uma lista encadeada.
 * Se duas palavras tiverem o mesmo indice, ambas ficam nessa lista.
 */
static HashNode *tabela[HASH_TABLE_SIZE] = {0};

static size_t quantidade_palavras = 0;

/*
 * Mistura final dos bits do hash.
 * Ajuda a distribuir melhor os valores antes do modulo HASH_TABLE_SIZE.
 */
static uint64_t avalanche(uint64_t h)
{
    h ^= h >> 33;
    h *= UINT64_C(0xff51afd7ed558ccd);

    h ^= h >> 33;
    h *= UINT64_C(0xc4ceb9fe1a85ec53);

    h ^= h >> 33;

    return h;
}

/*
 * FNV-1a de 64 bits seguido de avalanche.
 *
 * A funcao recebe uma palavra e devolve uma posicao entre
 * 0 e HASH_TABLE_SIZE - 1.
 */
size_t hash_indice(const char *palavra)
{
    uint64_t h = UINT64_C(1469598103934665603) ^ HASH_SEED;
    const unsigned char *p = (const unsigned char *)palavra;

    while (*p != '\0') {
        h ^= (uint64_t)(*p);
        h *= UINT64_C(1099511628211);
        p++;
    }

    h = avalanche(h);

    return (size_t)(h % HASH_TABLE_SIZE);
}

/*
 * Insere uma palavra usando encadeamento separado.
 *
 * tabela[indice]
 *       |
 *       v
 *   palavra -> palavra -> palavra -> NULL
 */
int hash_inserir(const char *palavra)
{
    HashNode *atual;
    HashNode *novo;
    char *copia;
    size_t indice;
    size_t tamanho;

    if (palavra == NULL || palavra[0] == '\0')
        return 0;

    indice = hash_indice(palavra);

    /*
     * Primeiro verifica se a palavra ja existe.
     */
    atual = tabela[indice];

    while (atual != NULL) {
        if (strcmp(atual->palavra, palavra) == 0)
            return 0;

        atual = atual->prox;
    }

    /*
     * Como a linha usada pelo fgets sera reutilizada, precisamos
     * guardar uma copia propria da palavra dentro da tabela.
     */
    tamanho = strlen(palavra) + 1;

    copia = (char *)malloc(tamanho);

    if (copia == NULL)
        return 0;

    memcpy(copia, palavra, tamanho);

    novo = (HashNode *)malloc(sizeof(HashNode));

    if (novo == NULL) {
        free(copia);
        return 0;
    }

    /*
     * Insere no inicio da lista.
     */
    novo->palavra = copia;
    novo->prox = tabela[indice];

    tabela[indice] = novo;

    quantidade_palavras++;

    return 1;
}

/*
 * Procura uma palavra.
 *
 * Primeiro calcula o indice. Depois percorre somente a lista
 * encadeada daquela posicao.
 */
int hash_buscar(const char *palavra)
{
    HashNode *atual;
    size_t indice;

    if (palavra == NULL || palavra[0] == '\0')
        return 0;

    indice = hash_indice(palavra);
    atual = tabela[indice];

    while (atual != NULL) {
        if (strcmp(atual->palavra, palavra) == 0)
            return 1;

        atual = atual->prox;
    }

    return 0;
}

/*
 * Le o CSV:
 *
 * palavra
 * cachorro
 * computador
 * ...
 *
 * Se o CSV tiver outras colunas, somente o texto antes da
 * primeira virgula sera considerado a palavra.
 */
int hash_inicializar(const char *caminho_csv)
{
    FILE *arquivo;
    char linha[512];

    if (caminho_csv == NULL)
        return 0;

    arquivo = fopen(caminho_csv, "r");

    if (arquivo == NULL) {
        perror("Erro ao abrir o dicionario");
        return 0;
    }

    /*
     * Remove qualquer conteudo anterior caso a funcao seja
     * chamada novamente.
     */
    hash_liberar();

    /*
     * Le e ignora o cabecalho.
     */
    if (fgets(linha, sizeof(linha), arquivo) == NULL) {
        fclose(arquivo);
        return 0;
    }

    while (fgets(linha, sizeof(linha), arquivo) != NULL) {

        /*
         * Remove \n e \r do final da linha.
         */
        linha[strcspn(linha, "\r\n")] = '\0';

        /*
         * Caso o arquivo possua colunas extras:
         *
         * cachorro,12345
         *
         * vira apenas:
         *
         * cachorro
         */

        if (linha[0] == '\0')
            continue;

        /*
         * hash_inserir faz uma copia da palavra.
         * Portanto, e seguro reutilizar 'linha' no proximo fgets.
         */
        hash_inserir(linha);
    }

    fclose(arquivo);

    return 1;
}

size_t hash_quantidade(void)
{
    return quantidade_palavras;
}

/*
 * Libera os nos e tambem as strings copiadas do CSV.
 */
void hash_liberar(void)
{
    size_t i;

    for (i = 0; i < HASH_TABLE_SIZE; i++) {
        HashNode *atual = tabela[i];

        while (atual != NULL) {
            HashNode *proximo = atual->prox;

            free(atual->palavra);
            free(atual);

            atual = proximo;
        }

        tabela[i] = NULL;
    }

    quantidade_palavras = 0;
}
