#pragma once

#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>

#include "ds/queue.h"
#include "common.h"

struct ServerResponse {
    int type;
    void * data;
    struct t_format ts;
};

struct Client {
    int sock_fd;
    pthread_mutex_t lock; // for the socket
    pthread_t tidr;
    struct sockaddr_in serveraddr_in;
    struct queue * response_q;
    struct User usr;
};

void Client_init (struct Client * client, char * serv_ip, int port, char *, int);
void Client_send (struct Client * client, char * , size_t);
void Client_recv (struct Client * client, char * );
void Client_exit (struct Client * client);
