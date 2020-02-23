#include "core/server.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

struct Server * p_server;

void sigint_handler (int signum) {
    if (p_server != NULL) {
        Server_exit (p_server);
        exit (0);
    }
}

int DEBUG;

int main (int argc, char ** argv) {
    if (argc != 2 && argc != 3) {
        printf ("Usage: %s <port> <mode:opt>", argv[0]);
        exit (EXIT_FAILURE);
    }
    if (argc == 3)
    if (!strcmp (argv[2], "DEBUG")) DEBUG = 1;
    signal (SIGINT, sigint_handler);
    int port = atoi (argv[1]);
    p_server = malloc (sizeof (struct Server));
    Server_init (p_server, port);
    Server_listen (p_server);
}
