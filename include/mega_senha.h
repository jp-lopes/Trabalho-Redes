#ifndef MEGA_SENHA_H
#define MEGA_SENHA_H

#define PATH_DICIONARIO "data/dicionario_validacao_80000.csv"
#define PATH_SENHAS "data/palavras_senha_5000.csv"

typedef struct {
    int id_partida;
    int id_client_1;
    int id_client_2;
    int pontos_client_1;
    int pontos_client_2;
} Partida ;

Partida* criar_partida(int id_client_1, int id_client_2);



#endif
