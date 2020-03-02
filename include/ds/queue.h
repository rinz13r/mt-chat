#pragma once

#include <stdlib.h>
#include <pthread.h>

#define MAX_QUEUE_SIZE 1000000

struct queue {
    void * arr[MAX_QUEUE_SIZE];
    size_t l, r, size;
    pthread_mutex_t qlock;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
};

struct queue * queue_new ();
size_t next (size_t pos);
void queue_push (struct queue * q, void * msg);
void * queue_pop (struct queue * q);
int queue_empty (struct queue * q);
int queue_full (struct queue * q);
