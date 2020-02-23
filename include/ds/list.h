#pragma once

#include <stdlib.h>
#include <stdio.h>

#define DECL_LIST_INTERFACE(Tp) struct node {\
    Tp val;\
    struct node * nxt;\
};\
struct node * node_new (Tp val, struct node * nxt);\
struct list {\
    struct node * head;\
    pthread_mutex_t lock;\
};\
void list_init (struct list * list);\
void list_print (struct list * list, void (*print) (Tp));\
void list_insert (struct list * list, Tp val);\
void list_remove (struct list * list, Tp val);\
void list_free (struct list * list);\

#define DECL_LIST_IMPL(Tp)\
struct node * node_new (Tp val, struct node * nxt) {\
    struct node * n = malloc (sizeof (struct node));\
    n->val = val;\
    n->nxt = nxt;\
    return n;\
}\
void list_init (struct list * list) {\
    pthread_mutex_init (&list->lock, NULL);\
}\
void list_print (struct list * list, void (*print) (Tp)) {\
    struct node * curr = list->head;\
    while (curr) {\
        print (curr->val);\
        curr = curr->nxt;\
    }\
    printf ("\n");\
}\
void list_insert (struct list * list, int val) {\
    pthread_mutex_lock (&list->lock);\
    struct node * n = node_new (val, list->head);\
    list->head = n;\
    pthread_mutex_unlock (&list->lock);\
}\
void list_remove (struct list * list, int val) {\
    pthread_mutex_lock (&list->lock);\
    struct node * curr = list->head, *prev = NULL;\
    while (curr) {\
        if (curr->val == val) break;\
        prev = curr;\
        curr = curr->nxt;\
    }\
    if (curr != NULL) {\
        if (prev == NULL) {\
            list->head = curr->nxt;\
            free (curr);\
        } else {\
            prev->nxt = curr->nxt;\
            free (curr);\
        }\
    }\
    pthread_mutex_unlock (&list->lock);\
}\
void list_free (struct list * list) {\
    pthread_mutex_lock (&list->lock);\
    struct node * curr = list->head, *prev = NULL;\
    while (curr) {\
        prev = curr;\
        curr = curr->nxt;\
        free (prev);\
    }\
    pthread_mutex_unlock (&list->lock);\
}\
void list_foreach (struct list * list, void (*func) (Tp val)) {\
    pthread_mutex_lock (&list->lock);\
    struct node * curr = list->head, *prev = NULL;\
    while (curr) {\
        func (curr->val);\
        curr = curr->nxt;\
    }\
    pthread_mutex_unlock (&list->lock);\
}
