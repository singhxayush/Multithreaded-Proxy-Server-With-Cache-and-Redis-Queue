This C library, `proxy_parse.c`, is designed to parse HTTP requests. It provides a structure (`ParsedRequest`) to hold the parsed components of an HTTP request and functions to manipulate and work with these components.

Here's a breakdown of the library's functionality and the purpose of each function:

**Core Data Structure: `ParsedRequest`**

* This structure is the heart of the library. It stores all the essential information extracted from an HTTP request.

* **Members:**
    * `buf`: A pointer to the raw HTTP request string.
    * `method`: The HTTP method (e.g., "GET", "POST").
    * `protocol`: The protocol (e.g., "http").
    * `host`: The hostname of the server.
    * `port`: The port number (optional).
    * `path`: The requested resource path.
    * `version`: The HTTP version (e.g., "HTTP/1.1").
    * `headers`: An array of `ParsedHeader` structures to store request headers.
    * `headerslen`: The allocated size of the `headers` array.
    * `headersused`: The number of headers currently stored in the `headers` array.

**Functions:**

* **`ParsedRequest_create()`:**
    * Allocates memory for a new `ParsedRequest` structure and initializes its members.

* **`ParsedRequest_destroy()`:**
    * Frees the memory allocated for a `ParsedRequest` structure and its components.

* **`ParsedRequest_parse(struct ParsedRequest *parse, const char *buf, int buflen)`:**
    * Parses a raw HTTP request string (`buf`) and populates the `ParsedRequest` structure with the extracted information.
    * It handles the request line (method, path, protocol, version) and parses the headers.

* **`ParsedRequest_unparse(struct ParsedRequest *pr, char *buf, size_t buflen)`:**
    * Reconstructs the entire HTTP request string from a `ParsedRequest` structure.

* **`ParsedRequest_unparse_headers(struct ParsedRequest *pr, char *buf, size_t buflen)`:**
    * Reconstructs only the headers from a `ParsedRequest` structure.

* **`ParsedRequest_totalLen(struct ParsedRequest *pr)`:**
    * Calculates the total length of the reconstructed HTTP request (including headers).

* **`ParsedHeader_create(struct ParsedRequest *pr)`:**
    * Initializes the `headers` array within a `ParsedRequest` structure.

* **`ParsedHeader_set(struct ParsedRequest *pr, const char *key, const char *value)`:**
    * Adds or updates a header in the `headers` array.

* **`ParsedHeader_get(struct ParsedRequest *pr, const char *key)`:**
    * Retrieves a header by its key.

* **`ParsedHeader_remove(struct ParsedRequest *pr, const char *key)`:**
    * Removes a header by its key.

* **`ParsedHeader_destroy(struct ParsedRequest *pr)`:**
    * Frees the memory allocated for the headers within a `ParsedRequest` structure.

* **`ParsedHeader_parse(struct ParsedRequest *pr, char *line)`:**
    * Parses a single header line from the raw HTTP request.

* **`ParsedHeader_printHeaders(struct ParsedRequest *pr, char *buf, size_t len)`:**
    * Formats the headers into a string.

* **`ParsedHeader_lineLen(struct ParsedHeader *ph)`:**
    * Calculates the length of a single header line.

* **`ParsedHeader_headersLen(struct ParsedRequest *pr)`:**
    * Calculates the total length of all headers.

* **`ParsedHeader_destroyOne(struct ParsedHeader *ph)`:**
    * Frees the memory allocated for a single header.

**Utility Functions:**

* **`debug(const char *format, ...)`:**
    * A debugging function that prints messages to stderr if the `DEBUG` macro is defined.

**How It Works:**

1. **Parsing:** The `ParsedRequest_parse()` function takes a raw HTTP request string and breaks it down into its components: method, path, protocol, version, and headers.

2. **Storing:** The parsed information is stored in the `ParsedRequest` structure.

3. **Accessing:** You can access the parsed data using the structure's members (e.g., `pr->method`, `pr->host`, etc.).

4. **Modifying:** You can modify headers using `ParsedHeader_set()` and `ParsedHeader_remove()`.

5. **Reconstruction:** The `ParsedRequest_unparse()` function can reconstruct the entire HTTP request string from the parsed data.

**Example Usage:**

```c
#include "proxy_parse.h"

int main() {
    char *request = "GET /index.html HTTP/1.1\r\nHost: www.example.com\r\n\r\n";
    struct ParsedRequest *pr = ParsedRequest_create();
    ParsedRequest_parse(pr, request, strlen(request));

    // Access parsed data
    printf("Method: %s\n", pr->method);
    printf("Host: %s\n", pr->host);

    // ...

    ParsedRequest_destroy(pr);
    return 0;
}
```