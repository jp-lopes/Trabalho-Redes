#include <stdio.h>
#include <stdlib.h>
#include "mega_senha.h"

int n_partidas = 0;

Partida* criar_partida(int id_client_1, int id_client_2)
{
    Partida* nova_partida = (Partida*)malloc(sizeof(Partida));

    if (nova_partida == NULL){
        printf("Erro na criação da partida.\n");
        return NULL;
    }

    nova_partida->id_client_1 = id_client_1;
    nova_partida->id_client_2 = id_client_2;
    nova_partida->id_partida = ++n_partidas;
    nova_partida->pontos_client_1 = 0;
    nova_partida->pontos_client_2 = 0;

    return nova_partida;
}