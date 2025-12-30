#ifndef CURL_H
#define CURL_H

#include <curl/curl.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>

#define BUF_SIZE 1048576  /* 1024*1024 = 1M */
#define BUF_INC  524288   /* 1024*512  = 0.5M */

#define CT_PNG  "image/png"
#define CT_HTML "text/html"

typedef struct recv_buf2 {
    char *buf;       /* memory to hold received data */
    size_t size;     /* size of valid data in buf in bytes*/
    size_t max_size; /* max capacity of buf in bytes*/
    int seq;         /* sequence number extracted from HTTP header, or -1 if invalid */
} RECV_BUF;

htmlDocPtr mem_getdoc(char *buf, int size, const char *url);
xmlXPathObjectPtr getnodeset(xmlDocPtr doc, xmlChar *xpath);
int find_http(char *buf, int size, int follow_relative_links, const char *base_url);

size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata);
size_t write_cb_curl3(char *p_recv, size_t size, size_t nmemb, void *p_userdata);

int recv_buf_init(RECV_BUF *ptr, size_t max_size);
int recv_buf_cleanup(RECV_BUF *ptr);

void cleanup(CURL *curl, RECV_BUF *ptr);

int write_file(const char *path, const void *in, size_t len);

CURL *easy_handle_init(RECV_BUF *ptr, const char *url);

int process_data(CURL *curl_handle, RECV_BUF *p_recv_buf);

#endif // CURL_H
