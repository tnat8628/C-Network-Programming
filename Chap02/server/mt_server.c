#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024

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
    char buffer[BUFFER_SIZE];
    int bytes_read;

    // Free the client info structure as it's no longer needed
    free(arg);

    // Get the client IP and port
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_addr.sin_port);

    // Read data from the client
    while ((bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0'; // Null-terminate the string
        printf("Received from %s:%d: %s\n", client_ip, client_port, buffer);

        // Echo the data back to the client
        write(client_socket, buffer, bytes_read);
    }

    if (bytes_read == 0) {
        printf("Client %s:%d disconnected\n", client_ip, client_port);
    } else {
        perror("read");
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