#define _GNU_SOURCE
#include <search.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "hash.h"


// Linked list node to store keys for cleanup
typedef struct key_node {
    char *key;
    struct key_node *next;
} key_node_t;

//static variables for hash table and linked list of keys
static int hash_created = 0;
static key_node_t *keys_head = NULL;

// Add key to linked list for later free
static void add_key_node(char *key) {
    key_node_t *node = malloc(sizeof(key_node_t));
    if (!node) {
        perror("malloc key_node");
        exit(EXIT_FAILURE); // Exiting on critical allocation failure
    }
    node->key = key;
    node->next = keys_head;
    keys_head = node;
}

int visited_init(size_t size) {
    if (hash_created) return 0; // Already initialized

    if (hcreate(size) == 0) {
        perror("hcreate"); // Print error if hash table creation fails
        return -1; // Failed
    }
    hash_created = 1;
    return 0;
}

bool visited_check(const char *url) {
    ENTRY item, *found;

    item.key = (char *)url;
    item.data = NULL; // Initialize data to avoid potential issues, though FIND doesn't use it
    found = hsearch(item, FIND);

    return (found != NULL);
}

int visited_add(const char *url) {
    ENTRY item, *found;
    char *key_copy = strdup(url);
    if (!key_copy) {
        perror("strdup url for hash");
        return -1; // Failed to duplicate string
    }

    item.key = key_copy;
    item.data = NULL; // Data field is not used for visited set, set to NULL

    found = hsearch(item, ENTER);
    if (found == NULL) {
        free(key_copy); // Free the copy if hsearch fails (e.g., table full)
        perror("hsearch ENTER");
        return -1;
    }

    if (found->key == key_copy) {
        // This is a new insertion, add key_copy to our cleanup list
        add_key_node(key_copy);
    } else {
        // Key already existed in the table, free duplicate
        free(key_copy);
    }

    return 0;
}

void visited_destroy(void) {
    if (hash_created) {
        // Free keys stored in our linked list
        key_node_t *curr = keys_head;
        while (curr != NULL) {
            key_node_t *next = curr->next;
            free(curr->key); // Free the actual key string
            free(curr);       // Free the node itself
            curr = next;
        }
        keys_head = NULL; // Reset head pointer

        // Destroy the hash table
        hdestroy();
        hash_created = 0;
    }
}