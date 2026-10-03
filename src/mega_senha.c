#include "mega_senha.h"

Senha senhas_faceis[QTD_TOTAL_SENHAS_FACEIS];
Senha senhas_medias[QTD_TOTAL_SENHAS_MEDIAS];
Senha senhas_dificeis[QTD_TOTAL_SENHAS_DIFICEIS];

// abre arquivo de senhas e carrega todas na memória, separadas por dificuldade
void carregar_senhas(char* path_senhas) {
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

// retorna uma senha aleatória
Senha sortear_senha() {
    char dificuldade = sortear_dificuldade(); 
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

// sorteia uma dificuldade com diferentes probabilidades
char sortear_dificuldade() {
    int r = rand() % 100; // gera um número de 0 a 99

    if (r < 50) {
        return '0'; // 50% de chance (0 a 49)
    } else if (r < 80) {
        return '1'; // 30% de chance (50 a 79)
    } else {
        return '2'; // 20% de chance (80 a 99)
    }
}

// verifica se a tentativa é válida, isto é, se existe no dicionário
int verificar_tentativa(char* tentativa) {
    // formata string retirando acentos e letras maiúsculas
    char tentativa_formatada[TAM_PALAVRA];
    strcpy(tentativa_formatada, tentativa);
    formatar_string(tentativa_formatada);
    // verifica se a palavra existe no dicionário
    return hash_buscar(tentativa_formatada);
}

// retorna 1 se a pessoa acertou a senha, e 0 caso contrário
int compara_tentativa_e_senha(char* tentativa, Senha s) {
    // formata strings retirando acentos e letras maiúsculas
    char senha_formatada[TAM_PALAVRA];
    strcpy(senha_formatada, s.palavra);
    formatar_string(senha_formatada);

    char tentativa_formatada[TAM_PALAVRA];
    strcpy(tentativa_formatada, tentativa);
    formatar_string(tentativa_formatada);

    // compara strings formatadas
    if (strcmp(tentativa_formatada, senha_formatada) == 0) return 1;
    return 0;
}

// Verifica se a palavra contém o radical da senha, retorna 1 se contiver e 0 se não contiver.
int verificar_radical_contido_na_palavra(char* palavra, Senha s) {
    if (strstr(palavra, s.radical) != NULL) return 1; // contém o radical
    return 0; // não contém o radical
}

// Função que remove os acentos de uma string e converte para letras minúsculas
void formatar_string(char *str)
{
    unsigned char *origem = (unsigned char *)str;
    unsigned char *destino = (unsigned char *)str;

    while (*origem != '\0') {
        // Caracteres ASCII possuem apenas um byte.
        if (*origem < 0x80) {
            unsigned char c = *origem;
            // Converte A-Z para a-z.
            if (c >= 'A' && c <= 'Z')
                c = c - 'A' + 'a';
            *destino++ = c;
            origem++;
            continue;
        }
        // Muitos caracteres portugueses em UTF-8 começam pelo byte hexadecimal C3.
        if (origem[0] == 0xC3 && origem[1] != '\0') {
            unsigned char segundo = origem[1];
            switch (segundo) {
                // Variações de A/a.
                case 0x80: 
                case 0x81:
                case 0x82:
                case 0x83:
                case 0x84:
                case 0xA0:
                case 0xA1:
                case 0xA2:
                case 0xA3:
                case 0xA4:
                    *destino++ = 'a';
                    break;
                // Variações de E/e.
                case 0x88:
                case 0x89:
                case 0x8A:
                case 0x8B:
                case 0xA8:
                case 0xA9:
                case 0xAA:
                case 0xAB:
                    *destino++ = 'e';
                    break;
                // Variações de I/i.
                case 0x8C:
                case 0x8D:
                case 0x8E:
                case 0x8F:
                case 0xAC:
                case 0xAD:
                case 0xAE:
                case 0xAF:
                    *destino++ = 'i';
                    break;
                // Variações de O/o.
                case 0x92:
                case 0x93:
                case 0x94:
                case 0x95:
                case 0x96:
                case 0xB2:
                case 0xB3:
                case 0xB4:
                case 0xB5:
                case 0xB6:
                    *destino++ = 'o';
                    break;
                // Variações de U/u.
                case 0x99:
                case 0x9A:
                case 0x9B:
                case 0x9C:
                case 0xB9:
                case 0xBA:
                case 0xBB:
                case 0xBC:
                    *destino++ = 'u';
                    break;
                // Ç e ç.
                case 0x87:
                case 0xA7:
                    *destino++ = 'c';
                    break;
                default:
                    // Caso apareça algum UTF-8 que não tratamos, preservamos os dois bytes originais.
                    *destino++ = origem[0];
                    *destino++ = origem[1];
                    break;
            }
            origem += 2;
            continue;
        }
        // Byte desconhecido: simplesmente copia.
        *destino++ = *origem++;
    }
    // Finaliza a nova string.
    *destino = '\0';
}



