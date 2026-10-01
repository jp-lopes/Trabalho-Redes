#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mega_senha.h"
#include "hash_palavras.h"

int partidas_count = 0;
Senha senhas_faceis[QTD_TOTAL_SENHAS_FACEIS];
Senha senhas_medias[QTD_TOTAL_SENHAS_MEDIAS];
Senha senhas_dificeis[QTD_TOTAL_SENHAS_DIFICEIS];

Partida* criar_partida(int id_client_1, int id_client_2)
{
    Partida* nova_partida = (Partida*)malloc(sizeof(Partida));

    if (nova_partida == NULL){
        printf("Erro na criação da partida.\n");
        return NULL;
    }

    nova_partida->id_client_1 = id_client_1;
    nova_partida->id_client_2 = id_client_2;
    nova_partida->id_partida = ++partidas_count;
    nova_partida->pontos = 0;
    nova_partida->id_cliente_adivinha = id_client_1;

    return nova_partida;
}

void carregar_senhas(char* path_senhas) 
{
    // abre arquivo com as senhas
    FILE* arq_senhas = fopen(path_senhas, "r");
    if(arq_senhas == NULL) {
        printf("Erro ao carregar senhas.\n");
        exit(-1);
    }
    
    char buffer[50];
    // pula cabeçalho
    if(fgets(buffer, sizeof(buffer), arq_senhas) == NULL){
        fclose(arq_senhas);
        return;
    }
    
    // le as senhas do arquivo 
    int i = 0, j = 0, k = 0;
    while(fgets(buffer, sizeof(buffer), arq_senhas) != NULL) {
        char *token_palavra = strtok(buffer, ",");
        char *token_radical = strtok(NULL, ","); 
        char *token_dificuldade = strtok(NULL, ",\n");
        
        if (token_palavra == NULL || token_radical == NULL || token_dificuldade == NULL) 
            break;

        char dificuldade = token_dificuldade[0];
        
        switch (dificuldade) {
            case '0':
                strcpy(senhas_faceis[i].palavra, token_palavra); 
                strcpy(senhas_faceis[i].radical, token_radical); 
                senhas_faceis[i].dificuldade = dificuldade;
                i++;
                break;
            case '1':
                strcpy(senhas_medias[j].palavra, token_palavra); 
                strcpy(senhas_medias[j].radical, token_radical); 
                senhas_medias[j].dificuldade = dificuldade;
                j++;
                break;
            case '2':
                strcpy(senhas_dificeis[k].palavra, token_palavra); 
                strcpy(senhas_dificeis[k].radical, token_radical); 
                senhas_dificeis[k].dificuldade = dificuldade;
                k++;
                break;
        }
    }
    srand(time(NULL)); //cria seed para sortear a senha baseada no horário atual
    fclose(arq_senhas);
}

Senha sortear_senha()
{
    char dificuldade = sortear_dificuldade(); // sorteia uma dificuldade com diferentes probabilidades
    int index;
    Senha sorteada;
    
    switch (dificuldade) {
        case '0':
            index = rand() % QTD_TOTAL_SENHAS_FACEIS;
            sorteada = senhas_faceis[index];
            break;
        case '1':
            index = rand() % QTD_TOTAL_SENHAS_MEDIAS;
            sorteada = senhas_medias[index];
            break; 
        case '2':
            index = rand() % QTD_TOTAL_SENHAS_DIFICEIS;
            sorteada = senhas_dificeis[index];
            break;
    }

    return sorteada;
}

char sortear_dificuldade() 
{
    // gera um número de 0 a 99
    int r = rand() % 100; 

    if (r < 50) {
        return '0'; // 50% de chance (0 a 49)
    } else if (r < 80) {
        return '1'; // 30% de chance (50 a 79)
    } else {
        return '2'; // 20% de chance (80 a 99)
    }
}

// verifica se a dica é válida, isto é, se existe no dicionário e se não contém o radical da senha
int verificar_dica_valida(char* dica, Senha s) 
{
    // formata string retirando acentos e letras maiúsculas
    formatar_string(dica);
    // verifica se não contém o radical da senha e se existe no dicionário
    return !verificar_radical_contido_na_palavra(dica,s) && hash_buscar(dica);
}

// verifica se a tentativa é válida, isto é, se existe no dicionário
int verificar_tentativa(char* tentativa)
{
    // formata string retirando acentos e letras maiúsculas
    formatar_string(tentativa);
    // verifica se a palavra existe no dicionário
    return hash_buscar(tentativa);
}

// retorna 1 se a pessoa acertou a senha, e 0 caso contrário
int compara_tentativa_e_senha(char* tentativa, Senha s)
{
    if (strcmp(tentativa, s.palavra) == 0) {
        return 1;
    }
    return 0;
}

// Verifica se a senha já foi utilizada nessa partida, retorna 1 se é repetida e 0 se não é repetida.
int verificar_senha_repetida(Partida p, Senha s)
{
    for(int i=0; i < p.pontos ;i++){
        if(strcmp(s.palavra, p.historico_de_senhas[i].palavra) == 0){
            return 1;
        }
    }
    return 0;
}

// Verifica se a palavra contém o radical da senha, retorna 1 se contiver e 0 se não contiver.
int verificar_radical_contido_na_palavra(char* palavra, Senha s)
{
    if (strstr(palavra, s.radical) != NULL) {
        return 1; // contém o radical
    }
    return 0; // não contém o radical
}

// Função que remove os acentos de uma string e converte para lowercase
void formatar_string(char *str) 
{
    char *sem_formatacao = "ABCDEFGHIJKLMNOPQRSTUVWXYZáàâãéèêíìîóòôõúùûçÁÀÂÃÉÈÊÍÌÎÓÒÔÕÚÙÛÇ";
    char *com_formatacao = "abcdefghijklmnopqrstuvwxyzaaaaeeeiiioooouuucaaaaeeeiiioooouuuc";

    for (int i = 0; str[i] != '\0'; i++) {
        for (int j = 0; sem_formatacao[j] != '\0'; j++) {
            if (str[i] == sem_formatacao[j]) {
                str[i] = com_formatacao[j];
                break;
            }
        }
    }
}



