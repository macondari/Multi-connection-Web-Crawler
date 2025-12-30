// -------------------- queue.h --------------------
#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

#define MAX_URL_LEN 1024

typedef struct {
    char **items;
    int capacity;
    int size;
    int front;
    int rear;
    pthread_mutex_t mtx;
    pthread_cond_t not_empty;
} queue_t;

void queue_init(queue_t *q, int capacity);
void queue_destroy(queue_t *q);
void queue_push(queue_t *q, const char *url);
char *queue_pop(queue_t *q);
int queue_empty(queue_t *q);

#endif
