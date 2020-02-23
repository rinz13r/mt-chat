#include "ds/queue.h"

struct queue * queue_new () {
    struct queue * q = malloc (sizeof (struct queue));
    q->l = q->r = 0;
    q->size = MAX_QUEUE_SIZE + 1;
    pthread_mutex_init (&q->qlock, NULL);
    pthread_cond_init (&q->not_full, NULL);
    pthread_cond_init (&q->not_empty, NULL);
    return q;
}

size_t next (size_t pos) {
    return (pos+1)%(MAX_QUEUE_SIZE + 1);
}

void queue_push (struct queue * q, void * entry) {
    pthread_mutex_lock (&q->qlock);
    while (queue_full (q)) {
        pthread_cond_wait (&q->not_full, &q->qlock);
    }
    q->arr[q->r] = entry;
    q->r = next (q->r);
    pthread_mutex_unlock (&q->qlock);
    pthread_cond_signal(&q->not_empty);
}

void * queue_pop (struct queue * q) {
    pthread_mutex_lock (&q->qlock);
    while (queue_empty (q)) {
        pthread_cond_wait (&q->not_empty, &q->qlock);
    }
    void * ret = q->arr[q->l];
    q->l = next (q->l);
    pthread_mutex_unlock (&q->qlock);
    pthread_cond_signal (&q->not_full);
    return ret;
}

int queue_empty (struct queue * q) {
    return (q->l == q->r);
}
int queue_full (struct queue * q) {
    return next (q->r) == q->l;
}
