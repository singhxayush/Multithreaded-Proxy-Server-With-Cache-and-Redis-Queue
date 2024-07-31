#include "../include/http1.x_handler.h"
#include "../include/style.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define MAX_REQUEST_BYTES 4096	  // max allowed size of request - 4 KB in bytes
#define MAX_RESPONSE_BYTES 262144 // max allowed size of response - 256 KB in bytes

/*
	MAX_CLIENTS: This is the maximum number of pending connections the server will queue up.
	If this limit is reached, new connection attempts will be added to the redis queue
	until a connection is accepted and closed.
*/
#define MAX_CLIENTS 10
#define MAX_SIZE 200 * (1 << 20)		// size of the cache
#define MAX_ELEMENT_SIZE 10 * (1 << 20) // max size of an element in cache



typedef struct cache_element cache_element;
struct cache_element
{
	char *request;
	int req_len;

	HttpResponse *response;

	time_t lru_time_track; // lru_time_track stores the latest time the element is  accesed

	cache_element *next; // pointer to next element
};

cache_element *head; // pointer to the cache
int cache_size;		 // cache_size denotes the current size of the cache

cache_element *find(char *url);
int add_cache_element(char *req, int len, struct HttpResponse* res);
void remove_cache_element();

// Port number - gets it's value from argument provided by the user.
int port_number;

// Maintaining an auxiliary variable to track the semaphore's state.
int semaphore_value = MAX_CLIENTS;

// Socket descriptor of proxy server.
int proxy_socketId;

// Array to store the thread ids of clients.
pthread_t tid[MAX_CLIENTS];

// If client requests exceeds the max_clients this semaphore puts the
// Waiting threads to sleep and wakes them when traffic on queue decreases.
sem_t *semaphore;

// This method initializes the mutex statically,
// which is useful for global or file-scope mutexes.
// It avoids the need for calling pthread_mutex_init() at runtime.
pthread_mutex_t cache_lock = PTHREAD_MUTEX_INITIALIZER;	 // lock is used for locking the cache
pthread_mutex_t client_lock = PTHREAD_MUTEX_INITIALIZER; // lock is used to acquire lock for multiple clients inside thread_fn() function

int sendErrorMessage(int socket, int status_code)
{
	char str[1024];
	char currentTime[50];
	time_t now = time(0);

	struct tm data = *gmtime(&now);
	strftime(currentTime, sizeof(currentTime), "%a, %d %b %Y %H:%M:%S %Z", &data);

	switch (status_code)
	{
	case 400:
		snprintf(str, sizeof(str), "HTTP/1.1 400 Bad Request\r\nContent-Length: 95\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>400 Bad Request</TITLE></HEAD>\n<BODY><H1>400 Bad Rqeuest</H1>\n</BODY></HTML>", currentTime);
		printf("400 Bad Request\n");
		send(socket, str, strlen(str), 0);
		break;

	case 403:
		snprintf(str, sizeof(str), "HTTP/1.1 403 Forbidden\r\nContent-Length: 112\r\nContent-Type: text/html\r\nConnection: keep-alive\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>403 Forbidden</TITLE></HEAD>\n<BODY><H1>403 Forbidden</H1><br>Permission Denied\n</BODY></HTML>", currentTime);
		printf("403 Forbidden\n");
		send(socket, str, strlen(str), 0);
		break;

	case 404:
		snprintf(str, sizeof(str), "HTTP/1.1 404 Not Found\r\nContent-Length: 91\r\nContent-Type: text/html\r\nConnection: keep-alive\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>\n<BODY><H1>404 Not Found</H1>\n</BODY></HTML>", currentTime);
		printf("404 Not Found\n");
		send(socket, str, strlen(str), 0);
		break;

	case 500:
		snprintf(str, sizeof(str), "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 115\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>500 Internal Server Error</TITLE></HEAD>\n<BODY><H1>500 Internal Server Error</H1>\n</BODY></HTML>", currentTime);
		// printf("500 Internal Server Error\n");
		send(socket, str, strlen(str), 0);
		break;

	case 501:
		snprintf(str, sizeof(str), "HTTP/1.1 501 Not Implemented\r\nContent-Length: 103\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>404 Not Implemented</TITLE></HEAD>\n<BODY><H1>501 Not Implemented</H1>\n</BODY></HTML>", currentTime);
		printf("501 Not Implemented\n");
		send(socket, str, strlen(str), 0);
		break;

	case 505:
		snprintf(str, sizeof(str), "HTTP/1.1 505 HTTP Version Not Supported\r\nContent-Length: 125\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>505 HTTP Version Not Supported</TITLE></HEAD>\n<BODY><H1>505 HTTP Version Not Supported</H1>\n</BODY></HTML>", currentTime);
		printf("505 HTTP Version Not Supported\n");
		send(socket, str, strlen(str), 0);
		break;

	default:
		return -1;
	}
	return 1;
}

void print_data(const char *data, int len)
{
	printf("\n-------------|Received Data|-------------\n\n");
	for (int i = 0; i < len; ++i)
	{
		putchar(data[i]);
	}
	printf("\n--------------|End of Data|--------------\n\n");
}

void print_line()
{
	printf("------------------------------------------------\n");
}

int checkHTTPversion(char *msg)
{
	int version = -1;

	if (strncmp(msg, "HTTP/1.1", 8) == 0)
	{
		version = 1;
	}
	else if (strncmp(msg, "HTTP/1.0", 8) == 0)
	{
		version = 1; // Handling this similar to version 1.1
	}
	else
	{
		version = -1;
	}

	return version;
}


int handle_request(int clientSocket, const char *URL, char *tempReq)
{
    HttpResponse *tempRes = initialize_empty_http_response();
    if (!tempRes) return -1;

    fetch_url(URL, tempRes);
    send_response(clientSocket, tempRes);

    // Adding to the Cache
    add_cache_element(tempReq, strlen(tempReq), tempRes);
    free(tempRes->headers);
    free(tempRes->body);
    free(tempRes);
    return 0;
}



void *thread_fn(void *socketNew)
{
	int forClient = *(int *)socketNew;
	printf("--------|EXECUTING THREAD FOR CLIENT: %d|--------\n", forClient);

	// sleep(10);

	sem_wait(semaphore);

	pthread_mutex_lock(&client_lock);
	semaphore_value--; // Critical section: Update semaphore value
	pthread_mutex_unlock(&client_lock);

	printf("Semaphore value: %d\n", semaphore_value);

	int *t = (int *)(socketNew);
	int clientSocketID = *t;  // Socket is socket descriptor of the connected Client
	int bytesFromClient, len; // Bytes Transferred from the client (request sent by the client)

	char *buffer = (char *)calloc(MAX_REQUEST_BYTES, sizeof(char)); // Creating buffer of 4kb for a client request

	bzero(buffer, MAX_REQUEST_BYTES);									  // Making buffer zero
	bytesFromClient = recv(clientSocketID, buffer, MAX_REQUEST_BYTES, 0); // Receiving the Request of client into the proxy server and writing it into the buffer

	while (bytesFromClient > 0)
	{
		len = strlen(buffer);
		// loop until u find "\r\n\r\n" in the buffer
		if (strstr(buffer, "\r\n\r\n") == NULL)
		{
			bytesFromClient = recv(clientSocketID, buffer + len, MAX_REQUEST_BYTES - len, 0);
		}
		else
		{
			break;
		}
	}

	// print_line();
	// printf("printing buffer\n\n");
	char *tempReq = (char *)malloc(strlen(buffer) * sizeof(char) + 1);
	// tempReq, buffer both store the http request sent by client
	for (int i = 0; i < strlen(buffer); i++)
	{
		tempReq[i] = buffer[i];
		// printf("%c", buffer[i]);
	}
	// print_line();

	// Variables to store the URL
	char URL[1028]; // This size is enough for the expected URL
	int flag = 0, cnt = 0;

	// Extract the URL from the request
	for (int i = 0; tempReq[i] != '\0'; i++)
	{
		if (!flag && tempReq[i] == '/')
		{
			flag = 1; // Start recording after first '/'
		}
		else if (flag && (tempReq[i] == ' ' && strncmp(&tempReq[i + 1], "HTTP/1.1", 8) == 0))
		{
			URL[cnt] = '\0'; // Null-terminate the URL string
			break;
		}
		else if (flag)
		{
			URL[cnt++] = tempReq[i]; // Copy the URL character by character
		}
	}

	// checking for the request in cache
	struct cache_element *temp = find(tempReq);

	if (temp != NULL)
	{
		// request found in cache, sending the response to client from proxy's cache.
		send_response(clientSocketID, temp->response);
		printf("Data retrived from the Cache\n\n");
	}
	else if (bytesFromClient > 0)
	{
		// Request not found
		len = strlen(buffer);

		// Initialising an empty object to store parsed request
		struct ParsedRequest *request = ParsedRequest_create();

		// ParsedRequest_parse returns 0 on success and -1 on failure.
		// On success it stores parsed request in the request
		if (ParsedRequest_parse(request, buffer, len) < 0)
		{
			printf("Parsing failed\n");
		}
		else
		{
			bzero(buffer, MAX_REQUEST_BYTES);
			if (!strcmp(request->method, "GET"))
			{

				if (request->host && request->path && (checkHTTPversion(request->version) == 1)) // This means everything is correct so far, it's just a Cache Miss situation
				{
					bytesFromClient = handle_request(clientSocketID, URL, tempReq); // Handle GET request
					if (bytesFromClient == -1)
					{
						sendErrorMessage(clientSocketID, 500);
					}
				}
				else
				{
					sendErrorMessage(clientSocketID, 500); // 500 Internal Error
				}
			}
			else
			{
				printf("This code doesn't support any method other than GET\n");
			}
		}
		// freeing up the request pointer
		ParsedRequest_destroy(request);
	}
	else if (bytesFromClient < 0)
	{
		perror("Error in receiving from client.\n");
	}
	else if (bytesFromClient == 0)
	{
		printf("Client disconnected!\n");
	}

	shutdown(clientSocketID, SHUT_RDWR);
	close(clientSocketID);
	free(buffer);
	sem_post(semaphore);
	free(tempReq);

	pthread_mutex_lock(&client_lock);
	semaphore_value++; // Critical section: Update semaphore value
	pthread_mutex_unlock(&client_lock);
	printf("Semaphore value: %d\n", semaphore_value);

	return NULL;
}

int main(int argc, char *argv[])
{

	// system("clear");

	int client_socketId, client_len;			 // client_socketId to store the client socket id
	struct sockaddr_in server_addr, client_addr; // Address of client and server to be assigned

	// Initializing named-semaphore and lock
	semaphore = sem_open("/mysemaphore", O_CREAT, 0644, MAX_CLIENTS);
	if (semaphore == SEM_FAILED)
	{
		perror("sem_open");
		exit(EXIT_FAILURE);
	}

	pthread_mutex_init(&client_lock, NULL); // Initializing lock for client
	pthread_mutex_init(&cache_lock, NULL);	// Initializing lock for cache

	if (argc == 2) // Checking whether two arguments are received or not
	{
		port_number = atoi(argv[1]);
	}
	else
	{
		printf("Too few arguments\n");
		exit(1);
	}

	printf("Setting Proxy Server Port : %d\n", port_number);

	// creating the proxy socket
	proxy_socketId = socket(AF_INET, SOCK_STREAM, 0);

	if (proxy_socketId < 0)
	{
		perror("Failed to create socket.\n");
		exit(1);
	}

	int reuse = 1;
	/*
		setsockopt aims to enable socket address reuse. This is essential in scenarios
		where you want to bind a socket to a specific address and port that was
		previously used by another socket, possibly even by the same program.

		Why is Address Reuse Important?

		• Speeding up Server Restart: Imagine a server listening on a port and suddenly shuts down.
		If address reuse is not enabled, the server cannot immediately start again on that
		same port because the port might be temporarily reserved (in a state known as TIME_WAIT)
		by the operating system.

		• Multiple Instances:  In situations where you want to run multiple instances
		of the same server program on the same machine, address reuse allows you to bind
		them all to the same port.

	*/
	if (setsockopt(proxy_socketId, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
	{
		perror("setsockopt(SO_REUSEADDR) failed\n");
	}

	bzero((char *)&server_addr, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port_number); // Assigning port to the Proxy
	server_addr.sin_addr.s_addr = INADDR_ANY;  // Any available adress assigned

	// Binding the socket to the given port
	if (bind(proxy_socketId, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("Port is not free\n");
		exit(1);
	}
	printf("Binding ✅\n");

	// Proxy socket listening to the requests
	int listen_status = listen(proxy_socketId, MAX_CLIENTS);

	if (listen_status < 0)
	{
		perror("Error while Listening.\n");
		exit(1);
	}
	else
	{
		printf("\033[1;32mListening on \033[0m:: %d\n", port_number);
	}
	print_line();
	print_line();
	print_line();

	int i = 0;							 // Iterator for thread_id (tid) and Accepted Client_Socket for each thread
	int Connected_socketId[MAX_CLIENTS]; // This array stores socket descriptors of connected clients

	// Infinite Loop for accepting connections
	while (1)
	{

		bzero((char *)&client_addr, sizeof(client_addr)); // Clears struct client_addr
		client_len = sizeof(client_addr);

		// Accepting the connections
		client_socketId = accept(proxy_socketId, (struct sockaddr *)&client_addr, (socklen_t *)&client_len); // Accepts connection

		if (client_socketId < 0)
		{
			fprintf(stderr, "Error in Accepting connection !\n");
			exit(1);
		}
		else
		{
			Connected_socketId[i] = client_socketId; // Storing accepted client into array
		}

		// Getting IP address and port number of client
		struct sockaddr_in *client_pt = (struct sockaddr_in *)&client_addr;
		struct in_addr ip_addr = client_pt->sin_addr;
		char str[INET_ADDRSTRLEN]; // INET_ADDRSTRLEN: Default ip address size
		inet_ntop(AF_INET, &ip_addr, str, INET_ADDRSTRLEN);

		// printf("\n\046[1;33mNew Incoming Client👤\033[0m");
		printf(BHYEL "\nNew Incoming Client 👤\n" reset);

		printf("\n#CLIENT_PORT_NUM \t: %d\n", ntohs(client_addr.sin_port));
		printf("#CLIENT_IP_ADDR \t: %s\n", str);
		printf("#CLIENT_SOCKET_ID \t: %d\n", client_socketId);
		printf("#CLIENT_INDEX \t\t: %d\n\n", i);

		// printf("Socket values of index %d in main function is %d\n",i, client_socketId);

		/*
			pthread_create is a function in the POSIX Threads (Pthreads)
			library used to create a new thread within a process.

			#include <pthread.h>

			int pthread_create (pthread_t *thread,
								const pthread_attr_t *attr,
								void *(*start_routine) (void *),
								void *arg);

			Parameters

				- thread: A pointer to a pthread_t object. This object
				will be filled with the thread ID of the newly created
				thread upon successful return.

				- attr: A pointer to a pthread_attr_t object. This object
				specifies attributes for the new thread, such as its stack
				size, scheduling policy, etc. If NULL, default attributes are used.

				- start_routine: A pointer to the function that the new
				thread will execute. This function should return a void* value.

				- arg: A single argument to be passed to the start_routine function.

			Return Value

				- 0: On success.
				- A non-zero error code on failure.

			pthread_create is a blocking call, meaning it will not return
			until the thread is successfully created.


		*/
		pthread_create(&tid[i], NULL, thread_fn, (void *)&Connected_socketId[i]); // Creating a thread for each client accepted
		i++;
	}

	close(proxy_socketId); // Close socket
						   // Clean up
	sem_close(semaphore);
	sem_unlink("/mysemaphore");
	return 0;
}

// @brief:
// @param url
// @return
cache_element *find(char *req)
{
	printf("\n\n-----------------|SEARCH CACHE|-----------------\n");
	// Checks for url in the cache if found returns
	// pointer to the respective cache element or else returns NULL
	cache_element *curr = NULL;

	// sem_wait(&cache_lock);
	int temp_lock_val = pthread_mutex_lock(&cache_lock);
	printf("\033[1mCache Lock:\033[0m 🔐(acquired, %d)\n", temp_lock_val);
	if (head != NULL)
	{
		curr = head;
		while (curr != NULL)
		{
			if (!strcmp(curr->request, req))
			{
				// Cache HIT
				printf("✅ \033[1;32mCACHE HIT:\033[0m URL FOUND\n");

				// Updating the time_track
				printf("LRU Time Track Before  : %ld\n", curr->lru_time_track);
				curr->lru_time_track = time(NULL);
				printf("LRU Time Track Updated : %ld\n", curr->lru_time_track);
				break;
			}
			curr = curr->next;
		}
	}
	else
	{
		printf("❌ \033[1;31mCACHE MISS:\033[0m URL NOT FOUND\n");
	}

	temp_lock_val = pthread_mutex_unlock(&cache_lock);
	printf("\033[1mCache Lock:\033[0m 🔓(released, %d)\n", temp_lock_val);
	return curr;
}

// @brief Removes element from the cache
void remove_cache_element()
{
    printf("\n\n-----------------|REMOVE CACHE|-----------------\n");

    cache_element *prev = NULL;  // Previous element pointer
    cache_element *curr = head;  // Current element pointer
    cache_element *oldest = head; // Oldest element to remove

    int temp_lock_val = pthread_mutex_lock(&cache_lock);
    if (temp_lock_val != 0) {
        perror("pthread_mutex_lock failed");
        return;
    }
    printf("\033[1mCache Lock:\033[0m 🔐(acquired, %d)\n", temp_lock_val);

    if (head != NULL)
    {
        // Find the element with the oldest LRU time track
        while (curr != NULL)
        {
            if (curr->lru_time_track < oldest->lru_time_track)
            {
                oldest = curr;
                prev = (prev == NULL) ? head : prev->next;
            }
            curr = curr->next;
        }

        // Remove the oldest element
        if (oldest == head)
        {
            head = head->next;
        }
        else
        {
            prev->next = oldest->next;
        }

        // Update the cache size
        cache_size -= (oldest->req_len) + sizeof(cache_element) + strlen(oldest->request) + 1;

        // Free the memory for the cache element
        free(oldest->request);
        free(oldest->response);
        free(oldest);
    }

    temp_lock_val = pthread_mutex_unlock(&cache_lock);
    if (temp_lock_val != 0) {
        perror("pthread_mutex_unlock failed");
    }
    printf("\033[1mCache Lock:\033[0m 🔓(released, %d)\n", temp_lock_val);
}

// @brief Add new element to the cache
// @param data Data retrieved from "GET" request
// @param size Size of the date
// @param url String of the URL
// @return
int add_cache_element(char *req, int len, struct HttpResponse *res)
{
    printf("\n\n-----------------|UPDATE CACHE|-----------------\n");

    int temp_lock_val = pthread_mutex_lock(&cache_lock);
    if (temp_lock_val != 0) {
        perror("pthread_mutex_lock failed");
        return -1;
    }
    printf("\033[1mCache Lock:\033[0m 🔐(acquired, %d)\n", temp_lock_val);

    int element_size = len + 1 + strlen(req) + sizeof(cache_element); // Size of the new element which will be added to the cache

    if (element_size > MAX_ELEMENT_SIZE) {
        pthread_mutex_unlock(&cache_lock);
        printf("\033[1mCache Lock:\033[0m 🔓(released, %d)\n", temp_lock_val);
        printf("Element size exceeds maximum limit. Not adding to cache.\n");
        return 0;
    } else {
        while (cache_size + element_size > MAX_SIZE) {
            // Remove elements until there is enough space
            remove_cache_element();
        }

        cache_element *element = (cache_element *)malloc(sizeof(cache_element));
        if (!element) {
            perror("malloc failed for cache_element");
            pthread_mutex_unlock(&cache_lock);
            return -1;
        }

        element->request = (char *)malloc(len + 1);
        if (!element->request) {
            perror("malloc failed for request");
            free(element);
            pthread_mutex_unlock(&cache_lock);
            return -1;
        }

        // Deep copy the response
        element->response = (HttpResponse *)malloc(sizeof(HttpResponse));
        if (!element->response) {
            perror("malloc failed for HttpResponse");
            free(element->request);
            free(element);
            pthread_mutex_unlock(&cache_lock);
            return -1;
        }
        element->response->headers = (char *)malloc(res->headers_len);
        if (res->headers_len > 0 && !element->response->headers) {
            perror("malloc failed for headers");
            free(element->response);
            free(element->request);
            free(element);
            pthread_mutex_unlock(&cache_lock);
            return -1;
        }
        memcpy(element->response->headers, res->headers, res->headers_len);
        element->response->headers_len = res->headers_len;

        element->response->body = (char *)malloc(res->body_len);
        if (res->body_len > 0 && !element->response->body) {
            perror("malloc failed for body");
            free(element->response->headers);
            free(element->response);
            free(element->request);
            free(element);
            pthread_mutex_unlock(&cache_lock);
            return -1;
        }
        memcpy(element->response->body, res->body, res->body_len);
        element->response->body_len = res->body_len;

        strcpy(element->request, req);
        element->lru_time_track = time(NULL); // Update the time_track
        element->next = head;
        element->req_len = len;
        head = element;
        cache_size += element_size;

        temp_lock_val = pthread_mutex_unlock(&cache_lock);
        if (temp_lock_val != 0) {
            perror("pthread_mutex_unlock failed");
            return -1;
        }
        printf("Cache Updated ✅\n");
        printf("\033[1mCache Lock:\033[0m 🔓(released, %d)\n", temp_lock_val);
        return 1;
    }
}