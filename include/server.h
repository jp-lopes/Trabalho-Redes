#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024

// Envia uma mensagem para todos os clientes, exceto o cliente que enviou a mensagem.
void broadcast_message(const char *message, int sender_socket);

// Remove um cliente da lista de clientes.
void remove_client(int socket);

 // Função executada pela thread de cada cliente.
void *handle_client(void *arg);



#endif 