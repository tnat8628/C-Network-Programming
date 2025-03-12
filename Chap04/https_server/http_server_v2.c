#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define PORT 9443
#define BUFFER_SIZE 4096

SSL_CTX *init_ssl_context() {
    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        perror("SSL_CTX_new failed");
        exit(EXIT_FAILURE);
    }
    
    if (SSL_CTX_use_certificate_file(ctx, "cert.pem", SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    return ctx;
}

void *handle_request(void *arg) {
    SSL *ssl = (SSL *)arg;
    char buffer[BUFFER_SIZE] = {0};
    
    int bytes_read = SSL_read(ssl, buffer, BUFFER_SIZE - 1);
    if (bytes_read <= 0) {
        perror("SSL_read failed");
        SSL_shutdown(ssl);
        SSL_free(ssl);
        return NULL;
    }
    
    printf("Received request:\n%s\n", buffer);
    
    const char *body = "<html><body><h1>HTTPS Server</h1></body></html>";
    int content_length = strlen(body);
    
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s", content_length, body);
    
    int bytes_written = 0, total_written = 0;
    int response_length = strlen(response);
    
    while (total_written < response_length) {
        bytes_written = SSL_write(ssl, response + total_written, response_length - total_written);
        if (bytes_written <= 0) {
            perror("SSL_write failed");
            break;
        }
        total_written += bytes_written;
    }
    
    SSL_shutdown(ssl);
    SSL_free(ssl);
    return NULL;
}

int main() {
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    
    SSL_CTX *ctx = init_ssl_context();
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_socket, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("HTTPS Server listening on port %d...\n", PORT);
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        
        SSL *ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client_socket);
        
        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            close(client_socket);
            continue;
        }
        
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_request, ssl) != 0) {
            perror("Thread creation failed");
            SSL_shutdown(ssl);
            SSL_free(ssl);
        }
        pthread_detach(tid);
    }
    
    close(server_socket);
    SSL_CTX_free(ctx);
    return 0;
}
