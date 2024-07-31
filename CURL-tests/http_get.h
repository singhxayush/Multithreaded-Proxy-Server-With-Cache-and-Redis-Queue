#ifndef HTTP_GET_H
#define HTTP_GET_H

#include <stddef.h>

// Struct to hold the response data
typedef struct {
    char *headers; // Response headers
    char *body;    // Response body
    size_t headers_len; // Length of headers
    size_t body_len;    // Length of body
} HttpResponse;

// Function to perform a GET request
HttpResponse* http_get(const char *url);

// Function to free the memory allocated for HttpResponse
void free_http_response(HttpResponse *response);

#endif // HTTP_GET_H
