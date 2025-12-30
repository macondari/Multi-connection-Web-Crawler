#include "queue.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h> // For perror and exit

void queue_init(queue_t *q, int capacity) {
    q->items = malloc(sizeof(char *) * capacity);
    if (q->items == NULL) {
        perror("malloc queue items");
        exit(EXIT_FAILURE); // Exit if initial allocation fails
    }
    q->capacity = capacity;
    q->size = 0;
    q->front = 0;
    q->rear = 0;
}

void queue_destroy(queue_t *q) {
    // Free all URLs still in the queue before freeing the array itself
    for (int i = 0; i < q->size; i++) {
        free(q->items[(q->front + i) % q->capacity]);
    }
    free(q->items);
}

void queue_push(queue_t *q, const char *url) {
    // Check if the queue needs resizing (dynamic array)
    if (q->size == q->capacity) {
        int new_cap = q->capacity * 2;
        char **new_items = malloc(sizeof(char *) * new_cap);
        if (new_items == NULL) {
            perror("malloc new_items during queue resize");
            exit(EXIT_FAILURE); // Handle allocation failure
        }

        // Copy existing items to the new, larger array
        for (int i = 0; i < q->size; i++) {
            new_items[i] = q->items[(q->front + i) % q->capacity];
        }

        free(q->items); // Free the old array
        q->items = new_items;
        q->capacity = new_cap;
        q->front = 0; // Reset front to 0
        q->rear = q->size; // Rear is now at the end of existing items
    }

    // Duplicate the URL string and add it to the queue
    q->items[q->rear] = strdup(url);
    if (q->items[q->rear] == NULL) {
        perror("strdup url for queue");
        exit(EXIT_FAILURE); // Handle string duplication failure
    }

    q->rear = (q->rear + 1) % q->capacity; // Move rear pointer
    q->size++; // Increment size
}

char *queue_pop(queue_t *q) {
    if (q->size == 0) {
        return NULL; // Queue is empty, return NULL 
    }

    char *url = q->items[q->front]; // Get the URL from the front
    q->front = (q->front + 1) % q->capacity; // Move front pointer
    q->size--; // Decrement size

    return url;
}

int queue_empty(queue_t *q) {
    // Simple check, no mutex needed as only 'main' accesses
    return (q->size == 0);
}