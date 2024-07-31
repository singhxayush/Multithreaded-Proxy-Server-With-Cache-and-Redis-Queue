#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

struct MemoryStruct {
    char *memory;
    size_t size;
};

struct CachedResponse {
    char *url;
    char *headers;
    char *body;
    size_t headers_len;
    size_t body_len;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) {
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

void fetch_url(const char *url, struct CachedResponse *cache) {
    CURL *curl_handle;
    CURLcode res;
    struct MemoryStruct chunk;
    struct MemoryStruct headers_chunk;

    chunk.memory = malloc(1);
    chunk.size = 0;
    headers_chunk.memory = malloc(1);
    headers_chunk.size = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);

    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)&headers_chunk);

    curl_easy_setopt(curl_handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

    res = curl_easy_perform(curl_handle);

    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    } else {
        // printf("%lu bytes retrieved\n", (unsigned long)chunk.size);
        cache->url = strdup(url);
        cache->headers = headers_chunk.memory;
        cache->headers_len = headers_chunk.size;
        cache->body = chunk.memory;
        cache->body_len = chunk.size;
    }

    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
}

void serve_cached_response(struct CachedResponse *cache) {
    // printf("HTTP/1.1 200 OK\r\n");
    printf("\r\n--------------------------------------------------\n");
    printf("%s", cache->headers);
    fwrite(cache->body, sizeof(char), cache->body_len, stdout);
    printf("\r\n--------------------------------------------------\n");
}

int main(void) {
    struct CachedResponse cache;

    fetch_url("https://www.cs.princeton.edu", &cache);
    serve_cached_response(&cache);

    free(cache.url);
    free(cache.headers);
    
    free(cache.body);

    return 0;
}
