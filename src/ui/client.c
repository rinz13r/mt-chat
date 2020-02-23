#include "core/client.h"
#include "common.h"
#include "ntp.h"

#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

struct Client * client_p;

int stat;
int tot = 0, n = 0;

void sigint_handler (int signum) {
    if (client_p != NULL) {
        if (stat) {
            printf ("Avg delay = [%lf]us\n", ((double)tot)/n);
        }
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
                time_t trecv = resp->ts.s;
                struct tm ts = *localtime (&trecv);
                char tstr[80];
                strftime (tstr, sizeof (tstr), "%H:%M:%S", &ts);
                printf ("[%s] <%s> : %s\n", tstr, msg->who, msg->msg);
                tot += delay;
                ++n;
                break;
            }
            case NOTIF : {
                struct Notification * notif = resp->data;
                printf ("[SERVER] %s\n", notif->msg);
                break;
            }
        }
        free (resp->data);
        free (resp);
    }
}

void * write_handler (void * arg) {
    char msg[256];
    while (1) {
        read (0, msg, 256);
        Client_send (client_p, msg, strlen (msg));
        memset (msg, '\0', 256);
    }
}

int DEBUG;

int main (int argc, char ** argv) {
    if (argc != 5 && argc != 6) {
        printf ("Usage: %s <serv_ip> <port> <name> <channel> <stat:opt>\n", argv[0]);
        exit (EXIT_FAILURE);
    }
    stat = 1;
    if (argc == 6) stat = 1;
    client_p = malloc (sizeof (struct Client));
    Client_init (client_p, argv[1], atoi (argv[2]), argv[3], atoi (argv[4]));
    pthread_t tid;
    pthread_create (&tid, NULL, write_handler, NULL);
    pthread_create (&tid, NULL, read_handler, NULL);
    signal (SIGINT, sigint_handler);
    pthread_join (tid, NULL);
}
