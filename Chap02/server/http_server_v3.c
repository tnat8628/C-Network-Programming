#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// HTTP response headers
const char* HTTP_200_OK = "HTTP/1.1 200 OK\r\n";
const char* HTTP_404_NOT_FOUND = "HTTP/1.1 404 Not Found\r\n";
const char* CONTENT_TYPE_HTML = "Content-Type: text/html\r\n";
const char* CONTENT_TYPE_JSON = "Content-Type: application/json\r\n";

// Sample HTML content for the home page
const char* INDEX_HTML = "<!DOCTYPE html>\n"
    "<html>\n"
    "<head><title>Simple HTTP Server</title></head>\n"
    "<body>\n"
    "<h1>Welcome to Simple HTTP Server</h1>\n"
    "<p>This is a basic HTTP server implementation.</p>\n"
    "</body>\n"
    "</html>\n";

// Function to parse HTTP headers
void parse_http_headers(char* buffer, char* method, char* path) {
    char* token = strtok(buffer, " \t\r\n");
    if (token) {
        strcpy(method, token);
        token = strtok(NULL, " \t\r\n");
        if (token) {
            strcpy(path, token);
        }
    }
}
// Structure to pass client information to the thread
typedef struct {
    int client_socket;
    struct sockaddr_in client_addr;
} client_info_t;

// Function to handle client communication
void *handle_client(void *arg) {
    client_info_t *client_info = (client_info_t *)arg;
    int client_socket = client_info->client_socket;
    struct sockaddr_in client_addr = client_info->client_addr;
    char buffer[BUFFER_SIZE] = {0};
    char method[16] = {0};
    char path[256] = {0};
    int total_bytes = 0;

    // Free the client info structure
    free(arg);

    // Read the initial request
    int bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_read <= 0) {
        close(client_socket);
        return NULL;
    }
    buffer[bytes_read] = '\0';

    // Parse request line
    char *first_line = strtok(buffer, "\r\n");
    if (first_line) {
        sscanf(first_line, "%s %s", method, path);
    }

    printf("Method: %s\n", method);
    printf("Path: %s\n", path);
    printf("Full request:\n%s\n", buffer);

    if (strcmp(method, "POST") == 0) {
        if (strcmp(path, "/api/data") == 0) {
            // Search for Content-Length header in both cases
            char *content_length_str = strstr(buffer, "Content-Length:");
            if (!content_length_str) {
                content_length_str = strstr(buffer, "content-length:");
            }

            if (content_length_str) {
                int content_length = 0;
                sscanf(content_length_str, "%*[^:]: %d", &content_length);
                printf("Content-Length: %d\n", content_length);

                // Find the request body
                char *body = strstr(buffer, "\r\n\r\n");
                if (body) {
                    body += 4;  // Skip \r\n\r\n
                    printf("Request body: %s\n", body);

                    // Send success response
                    const char *response_json = "{\"status\":\"success\",\"message\":\"Data received\"}";
                    char response[BUFFER_SIZE];
                    snprintf(response, BUFFER_SIZE,
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: application/json\r\n"
                            "Content-Length: %zu\r\n"
                            "Access-Control-Allow-Origin: *\r\n"
                            "Connection: close\r\n"
                            "\r\n"
                            "%s",
                            strlen(response_json), response_json);

                    if (write(client_socket, response, strlen(response)) < 0) {
                        perror("write failed");
                    }
                } else {
                    printf("No body found in request\n");
                }
            } else {
                printf("No Content-Length header found\n");
                const char *error_json = "{\"status\":\"error\",\"message\":\"Missing Content-Length header\"}";
                char response[BUFFER_SIZE];
                snprintf(response, BUFFER_SIZE,
                        "HTTP/1.1 400 Bad Request\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: %zu\r\n"
                        "Connection: close\r\n"
                        "\r\n"
                        "%s",
                        strlen(error_json), error_json);
                write(client_socket, response, strlen(response));
            }
        }
    }

    else if (strcmp(method, "GET") == 0) {
        if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
            // Serve home page
            char response[BUFFER_SIZE];
            sprintf(response, "%s%sContent-Length: %lu\r\n\r\n%s",
                    HTTP_200_OK,
                    CONTENT_TYPE_HTML,
                    strlen(INDEX_HTML),
                    INDEX_HTML);
            write(client_socket, response, strlen(response));
        }
        else if (strcmp(path, "/api/data") == 0) {
            // Sample JSON response
            const char* json_data = "{\"message\": \"Hello from the server!\"}";
            char response[BUFFER_SIZE];
            sprintf(response, "%s%sContent-Length: %lu\r\n\r\n%s",
                    HTTP_200_OK,
                    CONTENT_TYPE_JSON,
                    strlen(json_data),
                    json_data);
            write(client_socket, response, strlen(response));
        }
        else {
            // 404 Not Found
            const char* not_found = "<html><body><h1>404 Not Found</h1></body></html>";
            char response[BUFFER_SIZE];
            sprintf(response, "%s%sContent-Length: %lu\r\n\r\n%s",
                    HTTP_404_NOT_FOUND,
                    CONTENT_TYPE_HTML,
                    strlen(not_found),
                    not_found);
            write(client_socket, response, strlen(response));
        }
    }

    close(client_socket);
    return NULL;
}

int main() {
    int server_socket, new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    pthread_t tid;

    // Create socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Bind socket to address and port
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 10) == -1) {
        perror("listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // Accept connections in a loop
    while (1) {
        if ((new_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len)) == -1) {
            perror("accept");
            continue;
        }

        printf("New connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Allocate memory for the client info structure
        client_info_t *client_info = malloc(sizeof(client_info_t));
        if (!client_info) {
            perror("malloc");
            close(new_socket);
            continue;
        }

        // Populate the client info structure
        client_info->client_socket = new_socket;
        client_info->client_addr = client_addr;

        // Create a new thread to handle the client
        if (pthread_create(&tid, NULL, handle_client, client_info) != 0) {
            perror("pthread_create");
            free(client_info);
            close(new_socket);
        }

        // Detach the thread to allow it to clean up independently
        pthread_detach(tid);
    }

    // Close the server socket (this will never be reached in this example)
    close(server_socket);

    return 0;
}