#include "http_get.h"
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Callback function for writing received data into memory
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    HttpResponse *mem = (HttpResponse *)userp;

    char *ptr = realloc(mem->body, mem->body_len + realsize + 1);
    if (ptr == NULL)
        return 0; // Out of memory

    mem->body = ptr;
    memcpy(&(mem->body[mem->body_len]), contents, realsize);
    mem->body_len += realsize;
    mem->body[mem->body_len] = 0;

    return realsize;
}

// Callback function for writing received headers into memory
static size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
    size_t realsize = size * nitems;
    HttpResponse *mem = (HttpResponse *)userdata;

    char *ptr = realloc(mem->headers, mem->headers_len + realsize + 1);
    if (ptr == NULL)
        return 0; // Out of memory

    mem->headers = ptr;
    memcpy(&(mem->headers[mem->headers_len]), buffer, realsize);
    mem->headers_len += realsize;
    mem->headers[mem->headers_len] = 0;

    return realsize;
}

// Function to perform a GET request
HttpResponse* http_get(const char *url) {
    CURL *curl;
    CURLcode res;

    HttpResponse *response = malloc(sizeof(HttpResponse));
    if (!response) return NULL;

    response->headers = malloc(1); // Will be grown as needed by realloc
    response->body = malloc(1);    // Will be grown as needed by realloc
    response->headers_len = 0;
    response->body_len = 0;

    if (!response->headers || !response->body) {
        free_http_response(response);
        return NULL;
    }

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)response);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)response);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L); // Disable redirection

        // Perform the request
        res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            free_http_response(response);
            response = NULL;
        }

        // Cleanup
        curl_easy_cleanup(curl);
    }

    return response;
}

// Function to free the memory allocated for HttpResponse
void free_http_response(HttpResponse *response) {
    if (response) {
        free(response->headers);
        free(response->body);
        free(response);
    }
}

int main() {
    const char *url = "http://www.cs.princeton.edu";
    HttpResponse *response = http_get(url);

    if (response) {
        printf("Headers:\n%s\n", response->headers);
        printf("Body:\n%s\n", response->body);
        free_http_response(response);
    } else {
        printf("Failed to fetch the URL\n");
    }

    return 0;
}
