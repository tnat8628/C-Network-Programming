#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> // For inet_addr

#define PORT 8080
#define SERVER_IP "172.20.0.100" // Replace with the server's IP address
#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    int n;

    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);  // Use inet_addr

    while (1) {
        printf("Enter message: ");
        fflush(stdout);
        fgets(buffer, BUFFER_SIZE, stdin); // Use fgets to avoid buffer overflow
    
        buffer[strcspn(buffer, "\n")] = 0; // Remove trailing newline from fgets

        if (strcmp(buffer, "exit") == 0) {
            puts("Exiting...");
            break;
        }
        // Send data to server
        n = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (n < 0) {
            perror("sendto failed");
            continue;
        }

        // Receive data from server
        n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, NULL, NULL); // We don't need the server address again
        if (n < 0) {
            perror("recvfrom failed");
            continue;
        }

        buffer[n] = '\0';
        printf("Received from server: %s\n", buffer);

        if (strncmp(buffer, "exit", 4) == 0) { // Exit if server sends "exit"
            break;
        }
    }

    close(sockfd);

    return 0;
}

