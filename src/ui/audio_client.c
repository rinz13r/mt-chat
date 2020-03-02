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
#include <fcntl.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pulse/simple.h>
#include <pulse/error.h>
// #define BUFSIZE 1024

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
    printf ("In read_handler\n");
    struct queue * q = client_p->response_q;
    static const pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = 44100,
        .channels = 2
    };

    pa_simple *s = NULL;
    int ret = 1, error;
    if (!(s = pa_simple_new(NULL, NULL, PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &error))) {
        fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
        goto finish;
    }
    for (;;) {
        ssize_t r = BUFSIZE;
        struct ServerResponse * resp = queue_pop (q);
        switch (resp->type) {
            case VMSG : {
                struct VoiceMsg * msg = resp->data;
                if (pa_simple_write(s, msg->msg, (size_t) r, &error) < 0) {
                    fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
                    goto finish;
                }
                break;
            }
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
    finish:
        if (pa_simple_drain(s, &error) < 0) {
            fprintf(stderr, __FILE__": pa_simple_drain() failed: %s\n", pa_strerror(error));
            goto finish;
        }
        if (s)
            pa_simple_free(s);
}

void * voice_write_handler (void * arg) {
    printf ("In voice_write_handler\n");
    static const pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = 44100,
        .channels = 2
    };
    pa_simple *s = NULL;
    int error;
    if (!(s = pa_simple_new(NULL, NULL, PA_STREAM_RECORD, NULL, "record", &ss, NULL, NULL, &error))) {
        fprintf(stderr, __FILE__", %d: pa_simple_new() failed: %s\n", __LINE__, pa_strerror(error));
        goto finish;
    }
    int cnt = 0;
    for (;;) {
        struct VoiceMsg msg;
        msg.grp = client_p->usr.room;
        ssize_t r;
        if (pa_simple_read(s, msg.msg, sizeof(msg.msg), &error) < 0) {
            fprintf(stderr, __FILE__": pa_simple_read() failed: %s\n", pa_strerror(error));
            goto finish;
        }
        Client_send_vmsg (client_p, &msg);
        // int req = VMSG;
        // int r1 = write (client_p->sock_fd, &req, sizeof (int));
        // int r2 = write (client_p->sock_fd, &msg, sizeof (struct VoiceMsg));
        cnt++;
    }
    finish:
        if (s)
            pa_simple_free (s);
    printf ("cnt=%d\n", cnt);
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
    int talk = 0;
    if (argc == 6) talk = 1;

    client_p = malloc (sizeof (struct Client));
    Client_init (client_p, argv[1], atoi (argv[2]), argv[3], atoi (argv[4]));

    pthread_t tid_vw, tid_w, tid_r;
    if (talk) pthread_create (&tid_vw, NULL, voice_write_handler, NULL);
    pthread_create (&tid_r, NULL, read_handler, NULL);
    pthread_create (&tid_w, NULL, write_handler, NULL);


    signal (SIGINT, sigint_handler);
    sleep (100);

    Client_exit (client_p);
    return 0;
}
