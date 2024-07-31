#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <curl/curl.h>

#define PORT 8080
#define BUFFER_SIZE 2048

// Structure to hold response data
struct MemoryStruct {
    char *memory;
    size_t size;
};

// Callback function to write data into our struct
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL) {
        // Out of memory!
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

// Function to handle each client connection
void *handle_client(void *client_socket) {
    int client_sock = *(int *)client_socket;
    free(client_socket);

    char buffer[BUFFER_SIZE];
    int bytes_read = read(client_sock, buffer, BUFFER_SIZE - 1);
    if (bytes_read < 0) {
        perror("read");
        close(client_sock);
        return NULL;
    }

    buffer[bytes_read] = '\0';

    // Parse the URL from the request
    char method[BUFFER_SIZE], url[BUFFER_SIZE], protocol[BUFFER_SIZE];
    sscanf(buffer, "%s %s %s", method, url, protocol);

    // Remove the leading "http://localhost:8080/" part from the URL
    char *remote_url = strstr(url, "http://localhost:8080/");
    if (remote_url) {
        remote_url += strlen("http://localhost:8080/");
    } else {
        // Handle bad URL format
        fprintf(stderr, "Invalid URL format\n");
        close(client_sock);
        return NULL;
    }

    // Initialize a CURL session
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;

    chunk.memory = malloc(1);  // Will be grown as needed by the realloc above
    chunk.size = 0;            // No data at this point

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, remote_url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        res = curl_easy_perform(curl);

        // Check for errors
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            // Write the response back to the client
            write(client_sock, chunk.memory, chunk.size);
        }

        // Clean up
        curl_easy_cleanup(curl);
        free(chunk.memory);
    }

    curl_global_cleanup();

    close(client_sock);
    return NULL;
}

int main() {
    int server_sock, client_sock, *new_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t sin_size = sizeof(struct sockaddr_in);

    // Create socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // Set up the server address struct
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(server_addr.sin_zero), '\0', 8);

    // Bind socket
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        close(server_sock);
        exit(1);
    }

    // Listen on the socket
    if (listen(server_sock, 10) == -1) {
        perror("listen");
        close(server_sock);
        exit(1);
    }

    printf("Proxy server listening on port %d\n", PORT);

    while (1) {
        // Accept a new connection
        if ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &sin_size)) == -1) {
            perror("accept");
            continue;
        }

        printf("New connection from %s\n", inet_ntoa(client_addr.sin_addr));

        // Handle the new connection in a new thread
        pthread_t client_thread;
        new_sock = malloc(sizeof(int));
        *new_sock = client_sock;
        if (pthread_create(&client_thread, NULL, handle_client, (void *)new_sock) != 0) {
            perror("pthread_create");
            close(client_sock);
            free(new_sock);
            continue;
        }

        // Detach the thread to handle client independently
        pthread_detach(client_thread);
    }

    close(server_sock);
    return 0;
}
