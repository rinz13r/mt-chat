#include "core/client.h"
#include "common.h"
#include "ntp.h"

#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

ll tot = 0, n = 0;
struct Client * client_p;

void sigint_handler (int signum) {
    if (client_p != NULL) {
        Client_exit (client_p);
        exit (0);
    }
}

void * read_handler (void * arg) {
    struct queue * q = client_p->response_q;
    while (1) {
        struct ServerResponse * resp = queue_pop (q);
        switch (resp->type) {
            case MSG : {
                struct Msg * msg = resp->data;
                ll delay = timediff (msg->ts, resp->ts);
                if (n < 100) {
                    tot += delay;
                    ++n;
                }
                break;
            }
        }
        free (resp->data);
        free (resp);
    }
}

void * write_handler (void * arg) {
    char msg[256] = "asdf";
    while (1) {
        Client_send (client_p, msg, strlen (msg));
        usleep (1000);
    }
}

int DEBUG;

int main (int argc, char ** argv) {
    if (argc != 5 && argc != 6) {
        printf ("Usage: %s <serv_ip> <port> <name> <channel> <stat:opt>\n", argv[0]);
        exit (EXIT_FAILURE);
    }
    client_p = malloc (sizeof (struct Client));
    Client_init (client_p, argv[1], atoi (argv[2]), argv[3], atoi (argv[4]));
    pthread_t tidr, tidw;
    pthread_create (&tidw, NULL, write_handler, NULL);
    pthread_create (&tidr, NULL, read_handler, NULL);
    signal (SIGINT, sigint_handler);
    pthread_join (tidw, NULL);
    pthread_join (tidr, NULL);
}
