#define DECL_QUEUE_SR

#include "core/client.h"
#include <pulse/simple.h>
#include "common.h"
#include "ntp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

extern int DEBUG;

#define CALL(n, msg) if ((n) < 0) {fprintf (stderr, "%s (%s:%d)\n", msg, __FILE__, __LINE__); exit (EXIT_FAILURE);}
#define LOG(...) do {if (DEBUG) {fprintf (stderr, __VA_ARGS__); fprintf (stderr, "\n");}} while (0);

void * listen_handler (void * arg);

void Client_init (struct Client * client, char * serv_ip, int port, char * name, int room) {
    client->response_q = queue_new ();
    pthread_mutex_init (&client->lock, NULL);

    pthread_mutex_lock (&client->lock);
    CALL (client->sock_fd = socket (AF_INET, SOCK_STREAM, 0), "socket");

    memset(&client->serveraddr_in, '0', sizeof (struct sockaddr_in));
    client->serveraddr_in.sin_family = AF_INET;
    client->serveraddr_in.sin_addr.s_addr = inet_addr (serv_ip);
    client->serveraddr_in.sin_port = htons(port);

    CALL (connect ( client->sock_fd,
                    (struct sockaddr *)&client->serveraddr_in,
                    sizeof(struct sockaddr_in)),

        "connect");

    LOG ("Connected to Server");
    ntp_init ();
    strcpy (client->usr.name, name);

    int req = JOIN_ROOM;
    CALL (write (client->sock_fd, &req, sizeof (int)), "write");

    struct User usr; strcpy (usr.name, name); usr.room = room;
    client->usr = usr;
    CALL (write (client->sock_fd, &room, sizeof (int)), "write");
    CALL (write (client->sock_fd, &usr, sizeof (struct User)), "write");

    pthread_mutex_unlock (&client->lock);

    pthread_create (&client->tidr, NULL, listen_handler, (client));
}

void Client_send (struct Client * client, unsigned char * buf, size_t len) {
    struct Msg msg;
    static int req = MSG;
    msg.grp = client->usr.room;
    msg.ts = gettime ();

    strcpy (msg.who, client->usr.name);
    memcpy (msg.msg, buf, BUFSIZE);
    // &msg.msg = &buf;
    pthread_mutex_lock (&client->lock);
    write (client->sock_fd, &req, sizeof (int));
    write (client->sock_fd, &msg, sizeof (struct Msg));
    pthread_mutex_unlock (&client->lock);
}

void * listen_handler (void * arg) {
    struct Client * client = (struct Client *)arg;
    struct queue * q = client->response_q;
    int req;
    while (read (client->sock_fd, &req, sizeof (int))) {
        // read (client->sock_fd, &req, sizeof (int));
        struct t_format ts = gettime ();
        switch (req) {
            case MSG : {
                struct Msg * msg = malloc (sizeof (struct Msg));
                int b = read (client->sock_fd, msg, sizeof (struct Msg));
                if (b < 0) {
                    printf ("b<0\n");
                    return NULL;
                }
                struct ServerResponse * resp = malloc (sizeof (struct ServerResponse));
                resp->type = req;
                resp->data = msg;
                resp->ts = ts;
                queue_push (q, resp);
                break;
            }
            case NOTIF : {
                struct Notification * notif = malloc (sizeof (struct Notification));
                int b = read (client->sock_fd, notif, sizeof (struct Notification));
                if (b < 0) return NULL;
                struct ServerResponse * resp = malloc (sizeof (struct ServerResponse));
                resp->type = req;
                resp->data = notif;
                queue_push (q, resp);
                break;
            }
            case BYE : {
                LOG ("Server quit.");
                return NULL;
            }
        }
    }
}

char * Msg_get_who (struct Msg * msg) {
    return msg->who;
}
char * Msg_get_msg (struct Msg * msg) {
    return msg->msg;
}
struct Msg * get_msg (void * resp) {
    struct Msg * msg = (struct Msg *)(((struct ServerResponse*)resp)->data);
    return msg;
}
struct Notification * get_notif (void * resp) {
    return (struct Notification*)((struct ServerResponse*)resp)->data;
}
char * Notif_get_msg (struct Notification * notif) {
    return notif->msg;
}
void Client_exit (struct Client * client) {
    int req = LEAVE_ROOM;
    pthread_mutex_lock (&client->lock);
    int w = write (client->sock_fd, &req, sizeof (req));
    if (w < 0) {}
    close (client->sock_fd);
    // pthread_mutex_unlock (&client->lock);
    LOG ("Client exited");
}
