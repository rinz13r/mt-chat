#pragma once

#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

struct Server {
    int sock_fd;
    struct sockaddr_in serveraddr_in;
};

void Server_init (struct Server * server, int port);
void Server_listen (struct Server * server);
void Server_exit (struct Server * server);
