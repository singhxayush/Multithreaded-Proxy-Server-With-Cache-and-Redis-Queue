// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <curl/curl.h>
// #include <netinet/in.h>
// #include <sys/socket.h>
// #include <unistd.h>
// #include <pthread.h>

// #define PORT 8080
// #define BUFFER_SIZE 8192

// struct MemoryStruct {
//     char *memory;
//     size_t size;
// };

// struct CachedResponse {
//     char *url;
//     char *headers;
//     char *body;
//     size_t headers_len;
//     size_t body_len;
//     struct CachedResponse *next;
// };

// struct CachedResponse *cache_head = NULL;
// pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

// static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
//     size_t realsize = size * nmemb;
//     struct MemoryStruct *mem = (struct MemoryStruct *)userp;

//     char *ptr = realloc(mem->memory, mem->size + realsize + 1);
//     if (ptr == NULL) {
//         printf("Not enough memory (realloc returned NULL)\n");
//         return 0;
//     }

//     mem->memory = ptr;
//     memcpy(&(mem->memory[mem->size]), contents, realsize);
//     mem->size += realsize;
//     mem->memory[mem->size] = 0;

//     return realsize;
// }

// void fetch_url(const char *url, struct CachedResponse *cache) {
//     CURL *curl_handle;
//     CURLcode res;
//     struct MemoryStruct chunk;
//     struct MemoryStruct headers_chunk;

//     chunk.memory = malloc(1);
//     chunk.size = 0;
//     headers_chunk.memory = malloc(1);
//     headers_chunk.size = 0;

//     curl_global_init(CURL_GLOBAL_DEFAULT);
//     curl_handle = curl_easy_init();
//     curl_easy_setopt(curl_handle, CURLOPT_URL, url);

//     curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
//     curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

//     curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, WriteMemoryCallback);
//     curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)&headers_chunk);

//     curl_easy_setopt(curl_handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

//     res = curl_easy_perform(curl_handle);

//     if (res != CURLE_OK) {
//         fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
//     } else {
//         cache->url = strdup(url);
//         cache->headers = headers_chunk.memory;
//         cache->headers_len = headers_chunk.size;
//         cache->body = chunk.memory;
//         cache->body_len = chunk.size;
//     }

//     curl_easy_cleanup(curl_handle);
//     curl_global_cleanup();
// }

// void serve_cached_response(struct CachedResponse *cache, int client_socket) {
//     send(client_socket, cache->headers, cache->headers_len, 0);
//     send(client_socket, "\r\n", 2, 0); // Separate headers from body
//     send(client_socket, cache->body, cache->body_len, 0);
// }

// struct CachedResponse *find_cached_response(const char *url) {
//     struct CachedResponse *current = cache_head;
//     while (current != NULL) {
//         if (strcmp(current->url, url) == 0) {
//             return current;
//         }
//         current = current->next;
//     }
//     return NULL;
// }

// void add_to_cache(struct CachedResponse *new_cache) {
//     pthread_mutex_lock(&cache_mutex);
//     new_cache->next = cache_head;
//     cache_head = new_cache;
//     pthread_mutex_unlock(&cache_mutex);
// }

// void *handle_client_request(void *arg) {
//     int client_socket = *(int *)arg;
//     free(arg);
//     char buffer[BUFFER_SIZE];
//     int bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
//     if (bytes_read < 0) {
//         perror("Failed to read from client");
//         close(client_socket);
//         return NULL;
//     }
//     buffer[bytes_read] = '\0';

//     // Parse the GET request URL
//     char method[BUFFER_SIZE], url[BUFFER_SIZE], protocol[BUFFER_SIZE];
//     sscanf(buffer, "%s %s %s", method, url, protocol);
    
//     // Remove the leading "/" from the URL
//     if (url[0] == '/') {
//         memmove(url, url+1, strlen(url));
//     }

//     struct CachedResponse *cached_response = find_cached_response(url);
//     if (cached_response) {
//         printf("Cache hit for URL: %s\n", url);
//         serve_cached_response(cached_response, client_socket);
//     } else {
//         printf("Cache miss for URL: %s\n", url);
//         struct CachedResponse *new_cache = malloc(sizeof(struct CachedResponse));
//         fetch_url(url, new_cache);
//         add_to_cache(new_cache);
//         serve_cached_response(new_cache, client_socket);
//     }

//     close(client_socket);
//     return NULL;
// }

// int main(void) {
//     int server_fd, client_socket;
//     struct sockaddr_in address;
//     int opt = 1;
//     int addrlen = sizeof(address);

//     if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
//         perror("socket failed");
//         exit(EXIT_FAILURE);
//     }

//     if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
//         perror("setsockopt");
//         close(server_fd);
//         exit(EXIT_FAILURE);
//     }

//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(PORT);

//     if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
//         perror("bind failed");
//         close(server_fd);
//         exit(EXIT_FAILURE);
//     }

//     if (listen(server_fd, 3) < 0) {
//         perror("listen");
//         close(server_fd);
//         exit(EXIT_FAILURE);
//     }

//     printf("Proxy server listening on port %d\n", PORT);

//     while (1) {
//         if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
//             perror("accept");
//             continue;
//         }

//         pthread_t thread_id;
//         int *client_socket_ptr = malloc(sizeof(int));
//         *client_socket_ptr = client_socket;
//         if (pthread_create(&thread_id, NULL, handle_client_request, client_socket_ptr) != 0) {
//             perror("pthread_create");
//             free(client_socket_ptr);
//             close(client_socket);
//         }
//         pthread_detach(thread_id);
//     }

//     return 0;
// }















// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <curl/curl.h>
// #include <netinet/in.h>
// #include <sys/socket.h>
// #include <unistd.h>
// #include <pthread.h>

// #define PORT 8080
// #define BUFFER_SIZE 8192

// struct MemoryStruct {
//     char *memory;
//     size_t size;
// };

// struct CachedResponse {
//     char *url;
//     char *headers;
//     char *body;
//     size_t headers_len;
//     size_t body_len;
//     struct CachedResponse *next;
// };

// struct CachedResponse *cache_head = NULL;
// pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

// static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
//     size_t realsize = size * nmemb;
//     struct MemoryStruct *mem = (struct MemoryStruct *)userp;

//     char *ptr = realloc(mem->memory, mem->size + realsize + 1);
//     if (ptr == NULL) {
//         printf("Not enough memory (realloc returned NULL)\n");
//         return 0;
//     }

//     mem->memory = ptr;
//     memcpy(&(mem->memory[mem->size]), contents, realsize);
//     mem->size += realsize;
//     mem->memory[mem->size] = 0;

//     return realsize;
// }

// void fetch_url(const char *url, struct CachedResponse *cache) {
//     CURL *curl_handle;
//     CURLcode res;
//     struct MemoryStruct chunk;
//     struct MemoryStruct headers_chunk;

//     chunk.memory = malloc(1);
//     chunk.size = 0;
//     headers_chunk.memory = malloc(1);
//     headers_chunk.size = 0;

//     curl_global_init(CURL_GLOBAL_DEFAULT);
//     curl_handle = curl_easy_init();
//     curl_easy_setopt(curl_handle, CURLOPT_URL, url);

//     curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
//     curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

//     curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, WriteMemoryCallback);
//     curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)&headers_chunk);

//     curl_easy_setopt(curl_handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

//     res = curl_easy_perform(curl_handle);

//     if (res != CURLE_OK) {
//         fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
//         free(chunk.memory);
//         free(headers_chunk.memory);
//     } else {
//         cache->url = strdup(url);
//         cache->headers = headers_chunk.memory;
//         cache->headers_len = headers_chunk.size;
//         cache->body = chunk.memory;
//         cache->body_len = chunk.size;
//     }

//     curl_easy_cleanup(curl_handle);
//     curl_global_cleanup();
// }

// void serve_cached_response(struct CachedResponse *cache, int client_socket) {
//     send(client_socket, cache->headers, cache->headers_len, 0);
//     send(client_socket, "\r\n", 2, 0); // Separate headers from body
//     send(client_socket, cache->body, cache->body_len, 0);
// }

// struct CachedResponse *find_cached_response(const char *url) {
//     struct CachedResponse *current = cache_head;
//     while (current != NULL) {
//         if (strcmp(current->url, url) == 0) {
//             return current;
//         }
//         current = current->next;
//     }
//     return NULL;
// }

// void add_to_cache(struct CachedResponse *new_cache) {
//     pthread_mutex_lock(&cache_mutex);
//     new_cache->next = cache_head;
//     cache_head = new_cache;
//     pthread_mutex_unlock(&cache_mutex);
// }

// void *handle_client_request(void *arg) {
//     int client_socket = *(int *)arg;
//     free(arg);
//     char buffer[BUFFER_SIZE];
//     int bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
//     if (bytes_read < 0) {
//         perror("Failed to read from client");
//         close(client_socket);
//         return NULL;
//     }
//     buffer[bytes_read] = '\0';

//     // Parse the GET request URL
//     char method[BUFFER_SIZE], url[BUFFER_SIZE], protocol[BUFFER_SIZE];
//     sscanf(buffer, "%s %s %s", method, url, protocol);
    
//     // Remove the leading "/" from the URL
//     if (url[0] == '/') {
//         memmove(url, url+1, strlen(url));
//     }

//     struct CachedResponse *cached_response = find_cached_response(url);
//     if (cached_response) {
//         printf("Cache hit for URL: %s\n", url);
//         serve_cached_response(cached_response, client_socket);
//     } else {
//         printf("Cache miss for URL: %s\n", url);
//         struct CachedResponse *new_cache = malloc(sizeof(struct CachedResponse));
//         fetch_url(url, new_cache);
//         if (new_cache->body != NULL) {
//             add_to_cache(new_cache);
//             serve_cached_response(new_cache, client_socket);
//         } else {
//             free(new_cache);
//         }
//     }

//     close(client_socket);
//     return NULL;
// }

// int main(void) {
//     int server_fd, client_socket;
//     struct sockaddr_in address;
//     int opt = 1;
//     int addrlen = sizeof(address);

//     if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
//         perror("socket failed");
//         exit(EXIT_FAILURE);
//     }

//     if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
//         perror("setsockopt");
//         close(server_fd);
//         exit(EXIT_FAILURE);
//     }

//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(PORT);

//     if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
//         perror("bind failed");
//         close(server_fd);
//         exit(EXIT_FAILURE);
//     }

//     if (listen(server_fd, 3) < 0) {
//         perror("listen");
//         close(server_fd);
//         exit(EXIT_FAILURE);
//     }

//     printf("Proxy server listening on port %d\n", PORT);

//     while (1) {
//         if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
//             perror("accept");
//             continue;
//         }

//         pthread_t thread_id;
//         int *client_socket_ptr = malloc(sizeof(int));
//         *client_socket_ptr = client_socket;
//         if (pthread_create(&thread_id, NULL, handle_client_request, client_socket_ptr) != 0) {
//             perror("pthread_create");
//             free(client_socket_ptr);
//             close(client_socket);
//         }
//         pthread_detach(thread_id);
//     }

//     return 0;
// }
















// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <curl/curl.h>
// #include <netinet/in.h>
// #include <sys/socket.h>
// #include <unistd.h>
// #include <pthread.h>

// #define PORT 8080
// #define BUFFER_SIZE 8192

// struct MemoryStruct {
//     char *memory;
//     size_t size;
// };

// struct CachedResponse {
//     char *url;
//     char *headers;
//     char *body;
//     size_t headers_len;
//     size_t body_len;
//     struct CachedResponse *next;
// };

// struct CachedResponse *cache_head = NULL;
// pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

// static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
//     size_t realsize = size * nmemb;
//     struct MemoryStruct *mem = (struct MemoryStruct *)userp;

//     char *ptr = realloc(mem->memory, mem->size + realsize + 1);
//     if (ptr == NULL) {
//         printf("Not enough memory (realloc returned NULL)\n");
//         return 0;
//     }

//     mem->memory = ptr;
//     memcpy(&(mem->memory[mem->size]), contents, realsize);
//     mem->size += realsize;
//     mem->memory[mem->size] = 0;

//     return realsize;
// }

// void fetch_url(const char *url, struct CachedResponse *cache) {
//     CURL *curl_handle;
//     CURLcode res;
//     struct MemoryStruct chunk;
//     struct MemoryStruct headers_chunk;

//     chunk.memory = malloc(1);
//     chunk.size = 0;
//     headers_chunk.memory = malloc(1);
//     headers_chunk.size = 0;

//     curl_global_init(CURL_GLOBAL_DEFAULT);
//     curl_handle = curl_easy_init();
//     curl_easy_setopt(curl_handle, CURLOPT_URL, url);

//     curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
//     curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

//     curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, WriteMemoryCallback);
//     curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)&headers_chunk);

//     curl_easy_setopt(curl_handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

//     res = curl_easy_perform(curl_handle);

//     if (res != CURLE_OK) {
//         fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
//         free(chunk.memory);
//         free(headers_chunk.memory);
//     } else {
//         cache->url = strdup(url);
//         cache->headers = headers_chunk.memory;
//         cache->headers_len = headers_chunk.size;
//         cache->body = chunk.memory;
//         cache->body_len = chunk.size;
//     }

//     curl_easy_cleanup(curl_handle);
//     curl_global_cleanup();
// }

// void serve_cached_response(struct CachedResponse *cache, int client_socket) {
//     send(client_socket, "HTTP/1.1 200 OK\r\n", 17, 0);
//     send(client_socket, cache->headers, cache->headers_len, 0);
//     send(client_socket, "\r\n", 2, 0); // Separate headers from body
//     send(client_socket, cache->body, cache->body_len, 0);
// }

// struct CachedResponse *find_cached_response(const char *url) {
//     struct CachedResponse *current = cache_head;
//     while (current != NULL) {
//         if (strcmp(current->url, url) == 0) {
//             return current;
//         }
//         current = current->next;
//     }
//     return NULL;
// }

// void add_to_cache(struct CachedResponse *new_cache) {
//     pthread_mutex_lock(&cache_mutex);
//     new_cache->next = cache_head;
//     cache_head = new_cache;
//     pthread_mutex_unlock(&cache_mutex);
// }

// void *handle_client_request(void *arg) {
//     int client_socket = *(int *)arg;
//     free(arg);
//     char buffer[BUFFER_SIZE];
//     int bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
//     if (bytes_read < 0) {
//         perror("Failed to read from client");
//         close(client_socket);
//         return NULL;
//     }
//     buffer[bytes_read] = '\0';

//     // Parse the GET request URL
//     char method[BUFFER_SIZE], url[BUFFER_SIZE], protocol[BUFFER_SIZE];
//     sscanf(buffer, "%s %s %s", method, url, protocol);
    
//     // Remove the leading "/" from the URL
//     if (url[0] == '/') {
//         memmove(url, url+1, strlen(url));
//     }

//     struct CachedResponse *cached_response = find_cached_response(url);
//     if (cached_response) {
//         printf("Cache hit for URL: %s\n", url);
//         serve_cached_response(cached_response, client_socket);
//     } else {
//         printf("Cache miss for URL: %s\n", url);
//         struct CachedResponse *new_cache = malloc(sizeof(struct CachedResponse));
//         fetch_url(url, new_cache);
//         if (new_cache->body != NULL) {
//             add_to_cache(new_cache);
//             serve_cached_response(new_cache, client_socket);
//         } else {
//             free(new_cache);
//         }
//     }

//     close(client_socket);
//     return NULL;
// }

// int main(void) {
//     int server_fd, client_socket;
//     struct sockaddr_in address;
//     int opt = 1;
//     int addrlen = sizeof(address);

//     if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
//         perror("socket failed");
//         exit(EXIT_FAILURE);
//     }

//     if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
//         perror("setsockopt");
//         close(server_fd);
//         exit(EXIT_FAILURE);
//     }

//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(PORT);

//     if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
//         perror("bind failed");
//         close(server_fd);
//         exit(EXIT_FAILURE);
//     }

//     if (listen(server_fd, 3) < 0) {
//         perror("listen");
//         close(server_fd);
//         exit(EXIT_FAILURE);
//     }

//     printf("Proxy server listening on port %d\n", PORT);

//     while (1) {
//         if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
//             perror("accept");
//             continue;
//         }

//         pthread_t thread_id;
//         int *client_socket_ptr = malloc(sizeof(int));
//         *client_socket_ptr = client_socket;
//         if (pthread_create(&thread_id, NULL, handle_client_request, client_socket_ptr) != 0) {
//             perror("pthread_create");
//             free(client_socket_ptr);
//             close(client_socket);
//         }
//         pthread_detach(thread_id);
//     }

//     return 0;
// }




















// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <curl/curl.h>
// #include <netinet/in.h>
// #include <sys/socket.h>
// #include <unistd.h>
// #include <pthread.h>

// #define PORT 8080
// #define BUFFER_SIZE 8192

// struct MemoryStruct {
//     char *memory;
//     size_t size;
// };

// struct CachedResponse {
//     char *url;
//     char *headers;
//     char *body;
//     size_t headers_len;
//     size_t body_len;
//     struct CachedResponse *next;
// };

// struct CachedResponse *cache_head = NULL;
// pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

// static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
//     size_t realsize = size * nmemb;
//     struct MemoryStruct *mem = (struct MemoryStruct *)userp;

//     char *ptr = realloc(mem->memory, mem->size + realsize + 1);
//     if (ptr == NULL) {
//         printf("Not enough memory (realloc returned NULL)\n");
//         return 0;
//     }

//     mem->memory = ptr;
//     memcpy(&(mem->memory[mem->size]), contents, realsize);
//     mem->size += realsize;
//     mem->memory[mem->size] = 0;

//     return realsize;
// }

// static size_t WriteHeaderCallback(void *contents, size_t size, size_t nmemb, void *userp) {
//     size_t realsize = size * nmemb;
//     struct MemoryStruct *mem = (struct MemoryStruct *)userp;

//     char *ptr = realloc(mem->memory, mem->size + realsize + 1);
//     if (ptr == NULL) {
//         printf("Not enough memory (realloc returned NULL)\n");
//         return 0;
//     }

//     mem->memory = ptr;
//     memcpy(&(mem->memory[mem->size]), contents, realsize);
//     mem->size += realsize;
//     mem->memory[mem->size] = 0;

//     return realsize;
// }

// void fetch_url(const char *url, struct CachedResponse *cache) {
//     CURL *curl_handle;
//     CURLcode res;
//     struct MemoryStruct chunk;
//     struct MemoryStruct headers_chunk;

//     chunk.memory = malloc(1);
//     chunk.size = 0;
//     headers_chunk.memory = malloc(1);
//     headers_chunk.size = 0;

//     curl_global_init(CURL_GLOBAL_DEFAULT);
//     curl_handle = curl_easy_init();
//     curl_easy_setopt(curl_handle, CURLOPT_URL, url);

//     curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
//     curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

//     curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, WriteHeaderCallback);
//     curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)&headers_chunk);

//     curl_easy_setopt(curl_handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

//     res = curl_easy_perform(curl_handle);

//     if (res != CURLE_OK) {
//         fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
//         free(chunk.memory);
//         free(headers_chunk.memory);
//     } else {
//         cache->url = strdup(url);
//         cache->headers = headers_chunk.memory;
//         cache->headers_len = headers_chunk.size;
//         cache->body = chunk.memory;
//         cache->body_len = chunk.size;
//     }

//     curl_easy_cleanup(curl_handle);
//     curl_global_cleanup();
// }

// void serve_cached_response(struct CachedResponse *cache, int client_socket) {
//     // Send status line
//     send(client_socket, "HTTP/1.1 200 OK\r\n", 17, 0);
//     // Send headers
//     send(client_socket, cache->headers, cache->headers_len, 0);
//     // Send a blank line to separate headers from body
//     send(client_socket, "\r\n", 2, 0);
//     // Send body
//     send(client_socket, cache->body, cache->body_len, 0);

//     printf("\r\n--------------------------------------------------\n");
//     printf("%s", cache->headers);
//     fwrite(cache->body, sizeof(char), cache->body_len, stdout);
//     printf("\r\n--------------------------------------------------\n");
// }

// struct CachedResponse *find_cached_response(const char *url) {
//     struct CachedResponse *current = cache_head;
//     while (current != NULL) {
//         if (strcmp(current->url, url) == 0) {
//             return current;
//         }
//         current = current->next;
//     }
//     return NULL;
// }

// void add_to_cache(struct CachedResponse *new_cache) {
//     pthread_mutex_lock(&cache_mutex);
//     new_cache->next = cache_head;
//     cache_head = new_cache;
//     pthread_mutex_unlock(&cache_mutex);
// }

// void *handle_client_request(void *arg) {
//     int client_socket = *(int *)arg;
//     free(arg);
//     char buffer[BUFFER_SIZE];
//     int bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
//     if (bytes_read < 0) {
//         perror("Failed to read from client");
//         close(client_socket);
//         return NULL;
//     }
//     buffer[bytes_read] = '\0';

//     // Parse the GET request URL
//     char method[BUFFER_SIZE], url[BUFFER_SIZE], protocol[BUFFER_SIZE];
//     sscanf(buffer, "%s %s %s", method, url, protocol);
    
//     // Remove the leading "/" from the URL
//     if (url[0] == '/') {
//         memmove(url, url+1, strlen(url));
//     }

//     struct CachedResponse *cached_response = find_cached_response(url);
//     if (cached_response) {
//         printf("Cache hit for URL: %s\n", url);
//         serve_cached_response(cached_response, client_socket);
//     } else {
//         printf("Cache miss for URL: %s\n", url);
//         struct CachedResponse *new_cache = malloc(sizeof(struct CachedResponse));
//         fetch_url(url, new_cache);
//         if (new_cache->body != NULL) {
//             add_to_cache(new_cache);
//             serve_cached_response(new_cache, client_socket);
//         } else {
//             free(new_cache);
//         }
//     }

//     close(client_socket);
//     return NULL;
// }

// int main(void) {
//     int server_fd, client_socket;
//     struct sockaddr_in address;
//     int opt = 1;
//     int addrlen = sizeof(address);

//     if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
//         perror("socket failed");
//         exit(EXIT_FAILURE);
//     }

//     if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
//         perror("setsockopt");
//         close(server_fd);
//         exit(EXIT_FAILURE);
//     }

//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(PORT);

//     if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
//         perror("bind failed");
//         close(server_fd);
//         exit(EXIT_FAILURE);
//     }

//     if (listen(server_fd, 3) < 0) {
//         perror("listen");
//         close(server_fd);
//         exit(EXIT_FAILURE);
//     }

//     printf("Proxy server listening on port %d\n", PORT);

//     while (1) {
//         if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
//             perror("accept");
//             continue;
//         }

//         pthread_t thread_id;
//         int *client_socket_ptr = malloc(sizeof(int));
//         *client_socket_ptr = client_socket;
//         if (pthread_create(&thread_id, NULL, handle_client_request, client_socket_ptr) != 0) {
//             perror("pthread_create");
//             free(client_socket_ptr);
//             close(client_socket);
//         }
//         pthread_detach(thread_id);
//     }

//     return 0;
// }










#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 8192

// Structure to store memory for CURL callbacks
struct MemoryStruct {
    char *memory;
    size_t size;
};

// Structure to represent a cached response
struct CachedResponse {
    char *url;
    char *headers;
    char *body;
    size_t headers_len;
    size_t body_len;
    struct CachedResponse *next;
};

// Head of the linked list of cached responses
struct CachedResponse *cache_head = NULL;

// Mutex to protect the cache
pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

// Callback function for writing data to memory
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    // Reallocate memory to store the data
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) {
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }

    // Update the memory pointer and copy the data
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

// Callback function for writing headers to memory
static size_t WriteHeaderCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    // Reallocate memory to store the headers
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) {
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }

    // Update the memory pointer and copy the headers
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

// Fetch the content of a URL and store it in the cache
void fetch_url(const char *url, struct CachedResponse *cache) {
    CURL *curl_handle;
    CURLcode res;
    struct MemoryStruct chunk;
    struct MemoryStruct headers_chunk;

    // Allocate memory for storing the data and headers
    chunk.memory = malloc(1);
    chunk.size = 0;
    headers_chunk.memory = malloc(1);
    headers_chunk.size = 0;

    // Initialize CURL
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);

    // Set the write functions for data and headers
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, WriteHeaderCallback);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)&headers_chunk);

    // Set the HTTP version
    curl_easy_setopt(curl_handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

    // Perform the request
    res = curl_easy_perform(curl_handle);

    // Check for errors
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        free(chunk.memory);
        free(headers_chunk.memory);
    } else {
        // Store the fetched data in the cache
        cache->url = strdup(url);
        cache->headers = headers_chunk.memory;
        cache->headers_len = headers_chunk.size;
        cache->body = chunk.memory;
        cache->body_len = chunk.size;
    }

    // Clean up CURL resources
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
}

// Serve a cached response to a client
void serve_cached_response(struct CachedResponse *cache, int client_socket) {
    // Send the status line
    send(client_socket, "HTTP/1.1 200 OK\r\n", 17, 0);
    // Send the headers
    send(client_socket, cache->headers, cache->headers_len, 0);
    // Send a blank line to indicate the end of headers
    send(client_socket, "\r\n", 2, 0);
    // Send the body
    send(client_socket, cache->body, cache->body_len, 0);

    // Print the response for debugging
    printf("\r\n--------------------------------------------------\n");
    printf("%s", cache->headers);
    fwrite(cache->body, sizeof(char), cache->body_len, stdout);
    printf("\r\n--------------------------------------------------\n");
}

// Find a cached response for a given URL
struct CachedResponse *find_cached_response(const char *url) {
    struct CachedResponse *current = cache_head;
    // Iterate through the cache list
    while (current != NULL) {
        // Check if the URL matches
        if (strcmp(current->url, url) == 0) {
            return current;
        }
        current = current->next;
    }
    // Not found in cache
    return NULL;
}

// Add a new cached response to the cache
void add_to_cache(struct CachedResponse *new_cache) {
    // Lock the cache mutex for thread safety
    pthread_mutex_lock(&cache_mutex);

    // Add the new cache entry to the head of the list
    new_cache->next = cache_head;
    cache_head = new_cache;

    // Unlock the cache mutex
    pthread_mutex_unlock(&cache_mutex);
}

// Handle a client request
void *handle_client_request(void *arg) {
    // Get the client socket from the argument
    int client_socket = *(int *)arg;
    free(arg);
    char buffer[BUFFER_SIZE];
    int bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    // Check for read errors
    if (bytes_read < 0) {
        perror("Failed to read from client");
        close(client_socket);
        return NULL;
    }
    // Null-terminate the buffer
    buffer[bytes_read] = '\0';

    // Parse the GET request URL
    char method[BUFFER_SIZE], url[BUFFER_SIZE], protocol[BUFFER_SIZE];
    sscanf(buffer, "%s %s %s", method, url, protocol);

    // Remove the leading "/" from the URL
    if (url[0] == '/') {
        memmove(url, url+1, strlen(url));
    }

    // Check if the URL is in the cache
    struct CachedResponse *cached_response = find_cached_response(url);
    if (cached_response) {
        printf("Cache hit for URL: %s\n", url);
        // Serve the cached response
        serve_cached_response(cached_response, client_socket);
    } else {
        printf("Cache miss for URL: %s\n", url);
        // Fetch the URL and add it to the cache
        struct CachedResponse *new_cache = malloc(sizeof(struct CachedResponse));
        fetch_url(url, new_cache);
        if (new_cache->body != NULL) {
            add_to_cache(new_cache);
            // Serve the newly fetched response
            serve_cached_response(new_cache, client_socket);
        } else {
            free(new_cache);
        }
    }

    // Close the client socket
    close(client_socket);
    return NULL;
}

int main(void) {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Proxy server listening on port %d\n", PORT);

    while (1) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }

        pthread_t thread_id;
        int *client_socket_ptr = malloc(sizeof(int));
        *client_socket_ptr = client_socket;
        if (pthread_create(&thread_id, NULL, handle_client_request, client_socket_ptr) != 0) {
            perror("pthread_create");
            free(client_socket_ptr);
            close(client_socket);
        }
        pthread_detach(thread_id);
    }

    return 0;
}
