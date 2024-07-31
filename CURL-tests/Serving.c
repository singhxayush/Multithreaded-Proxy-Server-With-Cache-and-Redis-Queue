#include <nghttp2/nghttp2.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define MAKE_NV(NAME, VALUE)                                                    \
    {                                                                           \
        (uint8_t *)NAME, (uint8_t *)VALUE, sizeof(NAME) - 1, sizeof(VALUE) - 1, \
        NGHTTP2_NV_FLAG_NONE}

static int send_response(nghttp2_session *session, int32_t stream_id, struct MemoryStruct *chunk)
{
    nghttp2_nv hdrs[] = {
        MAKE_NV(":status", "200"),
        MAKE_NV("content-type", "text/html")};

    nghttp2_data_provider data_prd;
    data_prd.read_callback = NULL; // Define a callback to read data

    int rv = nghttp2_submit_response(session, stream_id, hdrs, 2, &data_prd);
    if (rv != 0)
    {
        return rv;
    }

    return 0;
}

static ssize_t data_source_read_callback(nghttp2_session *session,
                                         int32_t stream_id, uint8_t *buf,
                                         size_t length, uint32_t *data_flags,
                                         nghttp2_data_source *source,
                                         void *user_data)
{
    struct MemoryStruct *chunk = (struct MemoryStruct *)source->ptr;

    if (chunk->size < length)
    {
        length = chunk->size;
        *data_flags = NGHTTP2_DATA_FLAG_EOF;
    }

    memcpy(buf, chunk->memory, length);
    chunk->memory += length;
    chunk->size -= length;

    return length;
}

int main(void)
{
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    struct MemoryStruct chunk;
    nghttp2_session_callbacks *callbacks;
    nghttp2_session *session;

    // Fetch content to be served
    fetch_url("https://www.example.com", &chunk);

    // Set up the server socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);
    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_fd, 10);

    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);

    // Initialize nghttp2 session
    nghttp2_session_callbacks_new(&callbacks);
    nghttp2_session_server_new(&session, callbacks, NULL);

    // Send response
    send_response(session, 1, &chunk);

    nghttp2_session_del(session);
    nghttp2_session_callbacks_del(callbacks);
    close(client_fd);
    close(server_fd);

    free(chunk.memory);

    return 0;
}
