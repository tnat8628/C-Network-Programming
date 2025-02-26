#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> // For inet_addr

#define PORT 8080
#define BUFFER_SIZE 1024

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    int n;

    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY; // Listen on all available interfaces

    // Bind the socket to the server address
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("UDP server is listening on port %d...\n", PORT);

    while (1) {
        // Receive data from client
        n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_len);
        if (n < 0) {
            perror("recvfrom failed");
            continue;
        }

        buffer[n] = '\0'; // Null-terminate the received data
        printf("Received from client: %s\n", buffer);

        // Check if the client wants to exit
        if (strcmp(buffer, "exit") == 0) {
            printf("Client requested to exit. Closing connection...\n");
            break;
        }

        // Prepare a response
        char response[BUFFER_SIZE];
        snprintf(response, BUFFER_SIZE, "Server received: %s", buffer);

        // Send response back to client
        n = sendto(sockfd, response, strlen(response), 0, (struct sockaddr *)&client_addr, client_len);
        if (n < 0) {
            perror("sendto failed");
            continue;
        }

        printf("Response sent to client: %s\n", response);
    }

    // Close the socket
    close(sockfd);
    printf("Server shutdown.\n");

    return 0;
}