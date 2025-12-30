#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

// Timer helper to calculate elapsed time in seconds (as double)
double get_time_diff(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
}

// Save lines to a file (e.g., png_urls.txt or log.txt)
int write_lines_to_file(const char *filename, char **lines, int count) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("fopen");
        return -1;
    }
    for (int i = 0; i < count; ++i) {
        fprintf(fp, "%s\n", lines[i]);
    }
    fclose(fp);
    return 0;
}