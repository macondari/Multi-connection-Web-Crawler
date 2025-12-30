// findpng3.c
#include "findpng3.h"

// Define global variables declared in the header
queue_t url_queue;
int max_png_urls = DEFAULT_MAX_PNG;
int max_concurrent_connections = DEFAULT_CONCURRENT_CONN;
char *log_filename = NULL;
char *seed_url = NULL;
char **png_urls_found = NULL;
size_t png_urls_capacity = 0;
int found_png_count = 0;
FILE *log_fp = NULL;


int main(int argc, char **argv) {
    struct timeval start_time, end_time;
    int opt;

    // Parse command line arguments
    while ((opt = getopt(argc, argv, "t:m:v:")) != -1) {
        switch (opt) {
            case 't':
                max_concurrent_connections = atoi(optarg);
                if (max_concurrent_connections <= 0) max_concurrent_connections = DEFAULT_CONCURRENT_CONN;
                break;
            case 'm':
                max_png_urls = atoi(optarg);
                if (max_png_urls <= 0) max_png_urls = DEFAULT_MAX_PNG;
                break;
            case 'v':
                log_filename = strdup(optarg);
                if (!log_filename) {
                    perror("strdup log_filename");
                    exit(EXIT_FAILURE);
                }
                break;
            default:
                fprintf(stderr, "Usage: %s [-t NUM_CONNECTIONS] [-m MAX_PNGS] [-v LOGFILE] SEED_URL\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Error: No seed URL provided.\n");
        fprintf(stderr, "Usage: %s [-t NUM_CONNECTIONS] [-m MAX_PNGS] [-v LOGFILE] SEED_URL\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    seed_url = argv[optind];

    // Initialize cURL global environment
    curl_global_init(CURL_GLOBAL_ALL);

    // Initialize cURL multi handle
    CURLM * curlMulti = curl_multi_init();
    if (! curlMulti) {
        fprintf(stderr, "Error: curl_multi_init() failed.\n");
        curl_global_cleanup();
        exit(EXIT_FAILURE);
    }

    // Initialize queue, hash table, and PNG URL storage
    queue_init(&url_queue, 100);
    int visited_size = max_png_urls * max_concurrent_connections * 100; //estimating hash table size
    if (visited_size < 10000) visited_size = 10000; //minimum hash to avoid collisions
    if (visited_init(visited_size) != 0) { // Hash table size heuristic

        fprintf(stderr, "Failed to initialize visited hash table.\n");
        queue_destroy(&url_queue);
        curl_multi_cleanup( curlMulti);
        curl_global_cleanup();
        exit(EXIT_FAILURE);
    }

    png_urls_capacity = max_png_urls > 0 ? max_png_urls : DEFAULT_MAX_PNG;
    png_urls_found = malloc(sizeof(char *) * png_urls_capacity);
    if (!png_urls_found) {
        perror("malloc png_urls_found");
        visited_destroy();
        queue_destroy(&url_queue);
        curl_multi_cleanup( curlMulti);
        curl_global_cleanup();
        exit(EXIT_FAILURE);
    }

    // Open log file if specified
    if (log_filename) {
        log_fp = fopen(log_filename, "w");
        if (!log_fp) {
            perror("fopen log file");
            free(log_filename);
            log_filename = NULL; // Prevent double free or use of invalid fp
        }
    }

    // Add seed URL to queue and visited set
    if (!visited_check(seed_url)) {
        if (visited_add(seed_url) == 0) {
            queue_push(&url_queue, seed_url);
        } else {
            fprintf(stderr, "Failed to add seed URL '%s' to visited set.\n", seed_url);
            if (log_fp) fclose(log_fp);
            if (log_filename) free(log_filename);
            visited_destroy();
            queue_destroy(&url_queue);
            free(png_urls_found);
            curl_multi_cleanup( curlMulti);
            curl_global_cleanup();
            exit(EXIT_FAILURE);
        }
    } else {
        fprintf(stderr, "Seed URL '%s' was already marked visited. Exiting.\n", seed_url);
        if (log_fp) fclose(log_fp);
        if (log_filename) free(log_filename);
        visited_destroy();
        queue_destroy(&url_queue);
        free(png_urls_found);
        curl_multi_cleanup(curlMulti);
        curl_global_cleanup();
        exit(EXIT_FAILURE);
    }

    gettimeofday(&start_time, NULL);

    int still_running_transfers = 0;
    int active_easy_handles = 0;

    // start of main event loop for curl --- this is the main crawling loop
    do {
        // 1. Add new transfers to the multi handle if capacity allows and URLs are available
        while (active_easy_handles < max_concurrent_connections && !queue_empty(&url_queue) && found_png_count < max_png_urls) {
            char *url_to_fetch = queue_pop(&url_queue); 

            if (url_to_fetch) {
                CURL *easyHandle = curl_easy_init();
                if (!easyHandle) {
                    fprintf(stderr, "Error: curl_easy_init() failed for %s\n", url_to_fetch);
                    free(url_to_fetch);
                    continue;
                }

                EasyHandleContext *ctx = malloc(sizeof(EasyHandleContext));
                if (!ctx) {
                    perror("malloc EasyHandleContext");
                    curl_easy_cleanup(easyHandle);
                    free(url_to_fetch);
                    continue;
                }
                ctx->easy_handle = easyHandle;
                ctx->original_url = url_to_fetch;
                if (recv_buf_init(&ctx->recv_buf, BUF_SIZE) != 0) {
                    fprintf(stderr, "Error: recv_buf_init failed for URL: %s\n", url_to_fetch);
                    free(url_to_fetch);
                    free(ctx);
                    curl_easy_cleanup(easyHandle);
                    continue;
                }

                // Set standard cURL options for the easy handle
                curl_easy_setopt(easyHandle, CURLOPT_WRITEFUNCTION, write_cb_curl3);
                curl_easy_setopt(easyHandle, CURLOPT_WRITEDATA, (void *)&ctx->recv_buf);
                curl_easy_setopt(easyHandle, CURLOPT_HEADERFUNCTION, header_cb_curl);
                curl_easy_setopt(easyHandle, CURLOPT_HEADERDATA, (void *)&ctx->recv_buf);
                curl_easy_setopt(easyHandle, CURLOPT_URL, ctx->original_url);
                curl_easy_setopt(easyHandle, CURLOPT_PRIVATE, ctx);
                curl_easy_setopt(easyHandle, CURLOPT_USERAGENT, "ece252 lab5 crawler");
                curl_easy_setopt(easyHandle, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(easyHandle, CURLOPT_MAXREDIRS, 5L);
                curl_easy_setopt(easyHandle, CURLOPT_ACCEPT_ENCODING, "");
                curl_easy_setopt(easyHandle, CURLOPT_COOKIEFILE, "");
                curl_easy_setopt(easyHandle, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
                curl_easy_setopt(easyHandle, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
                curl_easy_setopt(easyHandle, CURLOPT_UNRESTRICTED_AUTH, 1L);
                curl_easy_setopt(easyHandle, CURLOPT_NOSIGNAL, 1L); // Important for multi-threaded apps (even if single control thread)

                // Add the easy handle to the multi stack
                curl_multi_add_handle( curlMulti, easyHandle);
                active_easy_handles++;

                // Log URL as visited
                if (log_fp) {
                    fprintf(log_fp, "%s\n", url_to_fetch); // No mutex needed
                    fflush(log_fp);
                }
            }
        }

        //check if need to break out of the loop
        if (active_easy_handles == 0 && queue_empty(&url_queue) && found_png_count < max_png_urls) {
             break;
        }

        //Perform pending transfers
        CURLMcode mc = curl_multi_perform( curlMulti, &still_running_transfers);
        if (mc != CURLM_OK) {
            fprintf(stderr, "curl_multi_perform() failed: %s\n", curl_multi_strerror(mc));
            break;
        }

        //Wait for activity on file descriptors or timeout
        if (still_running_transfers) {
            int numfds = 0;
            mc = curl_multi_wait( curlMulti, NULL, 0, MAX_WAIT_MSECS, &numfds);
            if (mc != CURLM_OK) {
                fprintf(stderr, "curl_multi_wait() failed: %s\n", curl_multi_strerror(mc));
                break;
            }
        }

        //Process completed transfers
        CURLMsg *msg;
        int msgs_left;
        while ((msg = curl_multi_info_read( curlMulti, &msgs_left))) {
            if (msg->msg == CURLMSG_DONE) {
                CURL *easyHandle = msg->easy_handle;
                EasyHandleContext *ctx;
                curl_easy_getinfo(easyHandle, CURLINFO_PRIVATE, &ctx);

                CURLcode res = msg->data.result;
                long response_code = 0;
                char *content_type = NULL;
                char *effective_url = NULL;

                if (res == CURLE_OK) {
                    curl_easy_getinfo(easyHandle, CURLINFO_RESPONSE_CODE, &response_code);
                    curl_easy_getinfo(easyHandle, CURLINFO_CONTENT_TYPE, &content_type);
                    curl_easy_getinfo(easyHandle, CURLINFO_EFFECTIVE_URL, &effective_url);
                    if (!effective_url) effective_url = ctx->original_url;

                    if (response_code >= 400) {
                        // fprintf(stderr, "URL %s returned HTTP error %ld\n", effective_url, response_code);
                    } else if (content_type && strstr(content_type, CT_HTML)) {
                        htmlDocPtr doc = mem_getdoc(ctx->recv_buf.buf, (int)ctx->recv_buf.size, effective_url);
                        if (doc) {
                            xmlChar *xpath = (xmlChar*) "//a/@href";
                            xmlXPathObjectPtr result = getnodeset(doc, xpath);
                            if (result) {
                                xmlNodeSetPtr nodeset = result->nodesetval;
                                for (int i = 0; i < nodeset->nodeNr; i++) {
                                    xmlChar *href = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode, 1);
                                    if (href) {
                                        xmlChar *absolute_href = xmlBuildURI(href, (xmlChar *) effective_url);
                                        if (absolute_href != NULL && (strncmp((const char *)absolute_href, "http://", 7) == 0 || strncmp((const char *)absolute_href, "https://", 8) == 0)) {
                                            if (!visited_check((const char *)absolute_href)) {
                                                if (visited_add((const char *)absolute_href) == 0) {
                                                    // No mutex for found_png_count check
                                                    if (found_png_count < max_png_urls) {
                                                        queue_push(&url_queue, (const char *)absolute_href);
                                                    }
                                                }
                                            }
                                        }
                                        xmlFree(absolute_href);
                                        xmlFree(href);
                                    }
                                }
                                xmlXPathFreeObject(result);
                            }
                            xmlFreeDoc(doc);
                        } else {
                            // fprintf(stderr, "Warning: Failed to parse HTML from %s\n", effective_url);
                        }
                    } else if (content_type && strstr(content_type, CT_PNG)) {
                        // No mutex for found_png_count / png_urls_found access
                        if (found_png_count < max_png_urls) {
                             if (ctx->recv_buf.size >= 8 && memcmp(ctx->recv_buf.buf, "\x89PNG\r\n\x1a\n", 8) == 0) {
                                if (found_png_count == png_urls_capacity) {
                                    size_t new_capacity = (png_urls_capacity == 0) ? 10 : png_urls_capacity * 2;
                                    char **new_array = realloc(png_urls_found, sizeof(char *) * new_capacity);
                                    if (new_array == NULL) {
                                        perror("realloc png_urls_found");
                                    } else {
                                        png_urls_found = new_array;
                                        png_urls_capacity = new_capacity;
                                    }
                                }
                                if (found_png_count < png_urls_capacity) {
                                    png_urls_found[found_png_count] = strdup(effective_url);
                                    if (png_urls_found[found_png_count] == NULL) {
                                        perror("strdup png url");
                                    } else {
                                        found_png_count++;
                                    }
                                }
                            }
                        }
                    }
                }

                // Cleanup for this completed easy handle
                recv_buf_cleanup(&ctx->recv_buf);
                free(ctx->original_url);
                free(ctx);
                curl_multi_remove_handle( curlMulti, easyHandle);
                curl_easy_cleanup(easyHandle);
                active_easy_handles--;
            }
        }
    } while (active_easy_handles > 0 || (!queue_empty(&url_queue) && found_png_count < max_png_urls));

    gettimeofday(&end_time, NULL);
    double elapsed = get_time_diff(start_time, end_time);

    if (write_lines_to_file("png_urls.txt", png_urls_found, found_png_count) != 0) {
        fprintf(stderr, "Error writing png_urls.txt\n");
    }

    printf("findpng3 execution time: %.6f seconds\n", elapsed);

    // CLEANUP RESOURCES----------------------------------------------------------------------------------------
    queue_destroy(&url_queue); // queue_destroy handles its own memory
    visited_destroy();         // visited_destroy handles its own memory

    // Free dynamically allocated png URLs
    for (int i = 0; i < found_png_count; i++) {
        free(png_urls_found[i]);
    }
    free(png_urls_found);

    if (log_fp) {
        fclose(log_fp);
    }

    if (log_filename) free(log_filename);

    curl_multi_cleanup( curlMulti);
    curl_global_cleanup();
    xmlCleanupParser();

    return 0;
}