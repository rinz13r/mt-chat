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
            case MSG : {
                struct Msg * msg = resp->data;
                if (pa_simple_write(s, msg->msg, (size_t) r, &error) < 0) {
                    fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
                    goto finish;
                }
                // ll delay = timediff (msg->ts, resp->ts);
                // time_t trecv = resp->ts.s;
                // struct tm ts = *localtime (&trecv);
                // char tstr[80];
                // strftime (tstr, sizeof (tstr), "%H:%M:%S", &ts);
                // printf ("[%s] <%s> : %s\n", tstr, msg->who, msg->msg);
                // tot += delay;
                ++n;
                break;
            }
            case NOTIF : {
                struct Notification * notif = resp->data;
                printf ("[SERVER] %s\n", notif->msg);
                break;
            }
        }
        // free (resp->data);
        // free (resp);
    }
    finish:
        if (pa_simple_drain(s, &error) < 0) {
            fprintf(stderr, __FILE__": pa_simple_drain() failed: %s\n", pa_strerror(error));
            goto finish;
        }
        if (s)
            pa_simple_free(s);
}

static ssize_t loop_write(int fd, const void*data, size_t size) {
    ssize_t ret = 0;
    while (size > 0) {
        ssize_t r;
        if ((r = write(fd, data, size)) < 0)
            return r;
        if (r == 0)
            break;
        ret += r;
        data = (const uint8_t*) data + r;
        size -= (size_t) r;
    }
    return ret;
}

void * write_handler (void * arg) {
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
        struct Msg msg;
        msg.grp = client_p->usr.room;
        // msg.ts = gettime ();
        ssize_t r;
        // if ((r = read(STDIN_FILENO, msg.msg, sizeof(msg.msg))) <= 0) {
        //     if (r == 0) /* EOF */
        //         break;
        //
        //     fprintf(stderr, __FILE__": read() failed: %s\n", strerror(errno));
        //     goto finish;
        // }
        if (pa_simple_read(s, msg.msg, sizeof(msg.msg), &error) < 0) {
            fprintf(stderr, __FILE__": pa_simple_read() failed: %s\n", pa_strerror(error));
            goto finish;
        }
        int req = MSG;
        int r1 = write (client_p->sock_fd, &req, sizeof (int));
        int r2 = write (client_p->sock_fd, &msg, sizeof (struct Msg));
        cnt++;
    }
    finish:
        if (s)
            pa_simple_free (s);
    printf ("cnt=%d\n", cnt);
}

int DEBUG;

int main (int argc, char ** argv) {
    if (argc != 5 && argc != 6) {
        printf ("Usage: %s <serv_ip> <port> <name> <channel> <stat:opt>\n", argv[0]);
        exit (EXIT_FAILURE);
    }
    // stat = 1;
    int talk;
    if (argc == 6) talk = 1;

    if (talk) {
        int fd;

        if ((fd = open("dump", O_RDONLY)) < 0) {
            fprintf(stderr, __FILE__": open() failed: %s\n", strerror(errno));
            goto finish;
        }

        if (dup2(fd, STDIN_FILENO) < 0) {
            fprintf(stderr, __FILE__": dup2() failed: %s\n", strerror(errno));
            goto finish;
        }

        close(fd);
    }
    client_p = malloc (sizeof (struct Client));
    Client_init (client_p, argv[1], atoi (argv[2]), argv[3], atoi (argv[4]));
    pthread_t tid;
    if (talk) pthread_create (&tid, NULL, write_handler, NULL);
    else pthread_create (&tid, NULL, read_handler, NULL);
    signal (SIGINT, sigint_handler);
    pthread_join (tid, NULL);
    Client_exit (client_p);
    finish:
    printf ("finished\n");
}
