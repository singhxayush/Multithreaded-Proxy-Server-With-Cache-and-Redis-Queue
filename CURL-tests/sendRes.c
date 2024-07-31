#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <curl/curl.h>


#define PORT 8080

struct MemoryStruct {
    char *memory;
    size_t size;
};

struct CachedResponse {
    char *url;
    char *headers;
    size_t headers_len;
    char *body;
    size_t body_len;
};

void send_response(int client_socket, struct CachedResponse* cache)
{
    char buffer[1024];

    // Construct the status line
    sprintf(buffer, "HTTP/1.1 200 OK\r\n");
    send(client_socket, buffer, strlen(buffer), 0);

    // Add headers
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char date_str[100];
    strftime(date_str, sizeof(date_str), "%a, %d %b %Y %H:%M:%S GMT", tm);

    sprintf(buffer, "Date: %s\r\n", date_str);
    send(client_socket, buffer, strlen(buffer), 0);

    sprintf(buffer, "Server: MySimpleServer/1.0\r\n");
    send(client_socket, buffer, strlen(buffer), 0);

    sprintf(buffer, "Content-Type: text/html\r\n");
    send(client_socket, buffer, strlen(buffer), 0);

    char* body = cache->body;
    int contentLen = cache->body_len;

    sprintf(buffer, "Content-Length: %d\r\n\r\n", contentLen);
    send(client_socket, buffer, strlen(buffer), 0);

    for(int i=0; i<contentLen; i+=1024)
    {
        for(int j=0; j<1024; j++)
        {
            buffer[j] = body[i+j];
        }
        send(client_socket, buffer, 1024, 0);
    }
}

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

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        exit(1);
    }

    // Bind socket to address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(1);
    }

    printf("binding successful...\n");

    // Listen for connections
    if (listen(server_socket, 5) == -1) {
        perror("listen");
        exit(1);
    }

    printf("listening on port :: 8080\n");
    struct CachedResponse cache;
    fetch_url("https://www.cs.princeton.edu", &cache);
    // printf("%s\n", cache.body);

    while (1)
    {
        // Accept connection
        socklen_t client_len = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket == -1) {
            perror("accept");
            continue;
        }

        send_response(client_socket, &cache);
    }
    close(client_socket);
    close(server_socket);
    return 0;
}
