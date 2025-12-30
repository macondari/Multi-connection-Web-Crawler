// findpng3.h
#ifndef FINDPNG3_H
#define FINDPNG3_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <curl/curl.h>
#include <libxml/HTMLparser.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/uri.h>
#include <stdbool.h>

#include "queue.h"
#include "hash.h"
#include "utils.h"
#include "curl.h"

// Default values
#define DEFAULT_MAX_PNG 50
#define DEFAULT_CONCURRENT_CONN 1
#define MAX_WAIT_MSECS 1000

#define CT_HTML "text/html"
#define CT_PNG "image/png"

extern queue_t url_queue;
extern int max_png_urls;
extern int max_concurrent_connections;
extern char *log_filename;
extern char *seed_url;
extern char **png_urls_found;
extern size_t png_urls_capacity;
extern int found_png_count;
extern FILE *log_fp;

typedef struct {
    CURL *easy_handle;
    RECV_BUF recv_buf;
    char *original_url;
} EasyHandleContext;

#endif // FINDPNG3_H
