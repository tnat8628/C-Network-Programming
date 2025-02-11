#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Structure to pass client information to the thread
typedef struct {
    int client_socket;
    struct sockaddr_in client_addr;
} client_info_t;

void send_response(int client_socket, char *status_code, char *content_type, char *content) {
    char response_header[BUFFER_SIZE];
    sprintf(response_header,
            "HTTP/1.1 %s\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %zu\r\n"
            "\r\n",
            status_code, content_type, strlen(content));
    write(client_socket, response_header, strlen(response_header));
    write(client_socket, content, strlen(content));
}



void *handle_client(void *arg) {
    client_info_t *client_info = (client_info_t *)arg;
    int client_socket = client_info->client_socket;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    free(arg); // Free the client info structure

    // Read the request from the client
    bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1);
    if (bytes_read <= 0) {
        perror("read");
        close(client_socket);
        return NULL;
    }

    buffer[bytes_read] = '\0';
    printf("Received HTTP Request:\n%s\n", buffer);

    // Extract the requested path (very basic, no error checking for simplicity)
    // Parse the request
    char *method = strtok(buffer, " ");
    char *path = strtok(NULL, " ");
    char *version = strtok(NULL, "\r\n");

    if (method == NULL || path == NULL || version == NULL) {
        printf("Invalid HTTP request\n");
        return;
    }

    printf("Method: %s\n", method);
    printf("Path: %s\n", path);
    printf("Version: %s\n", version);

    // Parse headers
    char *header_line;
    int content_length = 0;
    char *content_type = NULL;
    if (strcmp(method, "GET") == 0) {
        // Handle GET requests
        if (strcmp(path, "/") == 0) {
            path = "index.html"; // Default file
        }
        // Check if the file exists
        if (access(path + 1, F_OK) == -1) { // +1 to skip the leading '/'
            send_response(client_socket, "404 Not Found", "text/plain", "404 Not Found");
        } else {
            // Open the file
            FILE *file = fopen(path + 1, "r");  // +1 to remove leading /
            if (file == NULL) {
                send_response(client_socket, "500 Internal Server Error", "text/plain", "500 Internal Server Error");
            } else {
                // Read the file content
                fseek(file, 0, SEEK_END);
                long fsize = ftell(file);
                fseek(file, 0, SEEK_SET);

                char *file_content = malloc(fsize + 1);
                fread(file_content, 1, fsize, file);
                fclose(file);
                file_content[fsize] = 0;


                send_response(client_socket, "200 OK", "text/html", file_content);  // Send HTML
                free(file_content);
            }
        }
    } else if (strcmp(method, "POST") == 0) {  // Handle POST requests
        // Check Content-Type header (very basic parsing - improve for production)
        puts(buffer);
        char *content_type_header = strstr(buffer, "Content-Type:");
        puts("POST request received.");
        if (!content_type_header) {
            printf("Content-Type header not found!\n");
            return NULL;
        }
        puts(content_type_header);
        printf("\n");
        content_type_header += strlen("Content-Type:"); // Move pointer past the key
        while (*content_type_header == ' ' || *content_type_header == '\t') {
            content_type_header++; // Skip leading whitespace
        }

        char *end_of_content_type = strchr(content_type_header, '\r'); // Find the end of the value (carriage return)
        if (end_of_content_type) {
            *end_of_content_type = '\0'; // Null-terminate the value
        }

        printf("Content-Type header found: %s\n", content_type_header);
        if (content_type_header) {

            if (strstr(content_type_header, "application/json")) {
                // Find the beginning of the JSON data (after the headers)
                char *json_start = strstr(buffer, "\r\n\r\n");
                if (json_start) {
                    json_start += 4; // Move past the "\r\n\r\n"

                    // Process the JSON data (example - replace with your JSON handling)
                    printf("Received JSON data:\n%s\n", json_start);


                    // Send a response (example)
                    send_response(client_socket, "200 OK", "application/json", "{\"message\": \"JSON received\"}");
                } else {
                    send_response(client_socket, "400 Bad Request", "text/plain", "Invalid request");
                }
            } else { // Content-Type is not JSON
                send_response(client_socket, "415 Unsupported Media Type", "text/plain", "Unsupported Media Type");
            }
        } else { //No Content-Type header
            send_response(client_socket, "400 Bad Request", "text/plain", "Invalid request");        }
    } else {
       send_response(client_socket, "405 Method Not Allowed", "text/plain", "Method Not Allowed");
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