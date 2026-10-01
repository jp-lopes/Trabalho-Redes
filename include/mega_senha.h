#ifndef MEGA_SENHA_H
#define MEGA_SENHA_H

#define PATH_SENHAS "data/palavras_senha_5000.csv"
#define QTD_TOTAL_SENHAS 5000
#define QTD_TOTAL_SENHAS_FACEIS 2000
#define QTD_TOTAL_SENHAS_MEDIAS 2000
#define QTD_TOTAL_SENHAS_DIFICEIS 1000

typedef struct {
    char palavra[20];
    char radical[20];
    char dificuldade;
} Senha;

typedef struct {
    int id_partida;
    int id_client_1;
    int id_client_2;
    int pontos;
    int id_cliente_adivinha;
    Senha senha_atual;
    Senha historico_de_senhas[100];
} Partida;

Partida* criar_partida(int id_client_1, int id_client_2);

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
