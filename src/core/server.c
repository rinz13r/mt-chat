#include "core/server.h"
#include "ds/list.h"
#include "ds/queue.h"
#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

extern int DEBUG;
#define LOG(...) do {if (DEBUG) {fprintf (stderr, __VA_ARGS__); fprintf (stderr, "\n");}} while (0);
#define CALL(n, msg) if ((n) < 0) {fprintf (stderr, "%s (%s:%d)\n", msg, __FILE__, __LINE__); exit (EXIT_FAILURE);}

void * connection_handler (void *);
void * broadcast_handler (void *);
void * notification_handler (void *);
void dict_init ();
void queue_init ();

void sigpipe_handler (int signum) {}

void Server_init (struct Server * server, int port) {
    CALL (server->sock_fd = socket (AF_INET, SOCK_STREAM, 0), "socket")

    memset (&server->serveraddr_in, '0', sizeof (struct sockaddr_in));
    server->serveraddr_in.sin_family = AF_INET;
    server->serveraddr_in.sin_addr.s_addr = htonl (INADDR_ANY);
    server->serveraddr_in.sin_port = htons(port);

    int true = 1;
    CALL (setsockopt(server->sock_fd, SOL_SOCKET, SO_REUSEADDR, &true, sizeof (int)), "setsockopt");
    CALL (bind(server->sock_fd, (struct sockaddr*)&server->serveraddr_in, sizeof(struct sockaddr_in)), "bind");
    CALL (listen(server->sock_fd, 100), "listen");

    signal (SIGPIPE, SIG_IGN);
    dict_init ();
    queue_init ();
}

void Server_listen (struct Server * server) {
    LOG ("Server running.")
    pthread_t tidb, tidn;
    pthread_create (&tidb, NULL, broadcast_handler, NULL);
    pthread_create (&tidn, NULL, notification_handler, NULL);
    while (1) {
        int *connfd = malloc (sizeof (int));
        CALL (*connfd = accept (server->sock_fd, (struct sockaddr*)NULL ,NULL), "accept");
        LOG ("New Connection.");
        pthread_t tid;
        CALL (pthread_create (&tid, NULL, connection_handler, connfd), "pthread");
        usleep (20*1000);
    }
}

#define MAX_GROUPS 100

DECL_LIST_INTERFACE(int);
DECL_LIST_IMPL(int);
struct list dict[MAX_GROUPS];
// pthread_mutex_t lock[MAX_GROUPS];


void close_connection (int fd) {
    int req = BYE;
    write (fd, &req, sizeof (int));
    LOG ("%d ", fd);
    close (fd);
}

void Server_exit (struct Server * server) {
    for (int i = 0; i < MAX_GROUPS; ++i) {
        list_foreach (&dict[i], close_connection);
        list_free (&dict[i]);
    }
    LOG ("Server exited");
}

struct ClientResponse {
    struct Msg * msg;
    int connfd;
};
struct ClientNotification {
    struct Notification notif;
    int grp;
    int connfd;
};
struct queue * msgq;
struct queue * notifq;

static void notifq_push (struct ClientNotification * notif) {
    queue_push (notifq, notif);
}
static void msgq_push (struct ClientResponse * resp) {
    queue_push (msgq, resp);
}

void queue_init () {
    msgq = queue_new ();
    notifq = queue_new ();
}

void dict_init () {
    for (size_t i = 0; i < MAX_GROUPS; ++i) {
        list_init (&dict[i]);
    }
}
void dict_insert (int key, int val) {
    list_insert (&dict[key], val);
}
void dict_remove (int key, int val) {
    list_remove (&dict[key], val);
}

#define READ_REQ() read (connfd, &req, sizeof (int))
#define READ(what, size) read (connfd, what, size)

void * connection_handler (void * arg) {
    LOG ("In %s\n", __func__);
    int connfd = *(int*)arg;
    int req, room;
    struct User usr;
    // SETUP
    {
        CALL (READ_REQ (), "read");
        switch (req) {
            case JOIN_ROOM : {
                READ(&room, sizeof (int));
                READ(&usr, sizeof (struct User));
                dict_insert (room, connfd);
                // LOG ("New usr [%s] in room[%d]\n", usr.name, room);
                struct ClientNotification * notif = malloc (sizeof (struct ClientNotification));
                // READ (&notif->notif.ts, sizeof (struct t_format));
                notif->grp = room;
                notif->connfd = connfd;
                sprintf (notif->notif.msg, "%s has joined the room", usr.name);
                notifq_push (notif);
                break;
            }
            case LEAVE_ROOM : {
                struct ClientNotification * notif = malloc (sizeof (struct ClientNotification));
                // READ (&notif->notif.ts, sizeof (struct t_format));
                notif->grp = room;
                notif->connfd = connfd;
                sprintf (notif->notif.msg, "%s has left the room", usr.name);
                notifq_push (notif);
                goto end;
            }

        }
    }

    // MAIN LOOP
    {
        while (READ_REQ()) {
            // READ_REQ ();
            switch (req) {
                case LEAVE_ROOM : {
                    dict_remove (room, connfd);
                    struct ClientNotification * notif = malloc (sizeof (struct ClientNotification));
                    // READ (&notif->notif.ts, sizeof (struct t_format));
                    notif->grp = room;
                    notif->connfd = connfd;
                    sprintf (notif->notif.msg, "%s has left the room", usr.name);
                    notifq_push (notif);
                    goto end;
                }
                case MSG : {
                    // LOG ("req=msg");
                    struct Msg * msg = malloc (sizeof (struct Msg));
                    CALL (READ (msg, sizeof (struct Msg)), "read");
                    struct ClientResponse * resp = malloc (sizeof (struct ClientResponse));
                    resp->msg = msg;
                    resp->connfd = connfd;
                    msgq_push (resp);
                    break;
                }
            }
        }
    }


    end:
    dict_remove (room, connfd);
    close (connfd);
    free (arg);
    LOG ("closed connection %d", connfd);
    return NULL;
}

void * broadcast_handler (void * arg) {
    while (1) {
        struct ClientResponse * resp = queue_pop (msgq);
        // LOG ("broadcating..");
        struct Msg * msg = resp->msg;
        int grp = msg->grp, connfd = resp->connfd;

        pthread_mutex_lock (&dict[grp].lock);
        int serv_op = MSG, tot = 0;
        struct node * curr = dict[grp].head;
        while (curr) {
            if (curr->val == connfd) {
                curr = curr->nxt;
                continue;
            }
            ++tot;
            int w1 = write (curr->val, &serv_op, sizeof (int));
            int w2 = write (curr->val, msg, sizeof (struct Msg));
            // if (w1 != sizeof (int)) {
            //     printf ("err=%d\n", errno);
            //     if (errno == EPIPE) {
            //         printf ("EPIPE\n");
            //     }
            //     printf ("w1=%d\n", w1);
            //     assert (w1 == sizeof (int));
            // }
            // if (w2 != sizeof (struct Msg)) {
            //     printf ("err=%d\n", errno);
            //     if (errno == EPIPE) {
            //         printf ("EPIPE\n");
            //     }
            //     printf ("w2=%d\n", w2);
            //     assert (w2 == sizeof (struct Msg));
            // }
            curr=curr->nxt;
        }
        free (msg);
        pthread_mutex_unlock (&dict[grp].lock);
    }
}

void * notification_handler (void * arg) {
    while (1) {
        struct ClientNotification * notif = queue_pop (notifq);
        int grp = notif->grp, connfd = notif->connfd;

        pthread_mutex_lock (&dict[grp].lock);
        int serv_op = NOTIF, tot = 0;
        struct node * curr = dict[grp].head;
        while (curr) {
            if (curr->val == connfd) {
                curr = curr->nxt;
                continue;
            }
            ++tot;
            write (curr->val, &serv_op, sizeof (int));
            write (curr->val, &notif->notif, sizeof (struct Notification));
            curr=curr->nxt;
        }
        free (notif);
        pthread_mutex_unlock (&dict[grp].lock);
    }
}

#undef READ_REQ
#undef READ
