ECE252 Lab 5
Michaela and Deyora

FindPNG3: Multi-connection Web Crawler

This C project implements a  web crawler using libcurl’s multi interface to perform concurrent HTTP requests. Starting from a seed URL, it explores web pages, extracts and collects PNG image URLs, and logs all visited links. The crawler handles various HTTP responses and content types, uses thread-safe data structures for URL management, and supports customizable command-line options for concurrency level, maximum PNG URLs to find, and optional logging. The program outputs a list of discovered PNG URLs, an optional log of all visited URLs, and reports total execution time.