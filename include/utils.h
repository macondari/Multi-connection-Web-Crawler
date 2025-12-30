#ifndef UTILS_H
#define UTILS_H

#include <sys/time.h>

// Calculate time difference in seconds (as double)
double get_time_diff(struct timeval start, struct timeval end);

// Write an array of strings to a file (one line per string)
int write_lines_to_file(const char *filename, char **lines, int count);

#endif // UTILS_H