#ifndef HASH_H
#define HASH_H

#include <stdbool.h>
#include <stddef.h>

// Initialize the visited URLs hash table (size is the initial number of entries)
int visited_init(size_t size);

// Check if URL has been visited before (returns true if visited)
bool visited_check(const char *url);

// Add URL to visited set (returns 0 on success, non-zero on failure)
int visited_add(const char *url);

// Cleanup the hash table resources
void visited_destroy(void);

#endif // HASH_H
