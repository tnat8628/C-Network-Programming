#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/if_ether.h>
#include <netpacket/packet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <ctype.h>
#include <netinet/ip.h>

#define BUFFER_SIZE 65536


void print_buffer_in_hex(unsigned char *buffer, int length) {
    for (int i = 0; i < length; i++) {
        // Print offset address at the start of each line
        if (i % 16 == 0) {
            printf("%08x  ", i); // Print offset in 8-digit hexadecimal format
        }        
        // Print hexadecimal value
        printf("%02x ", buffer[i]);

        // Print ASCII characters after every 16 bytes
        if ((i + 1) % 16 == 0 || i == length - 1) {
            // Add padding for incomplete lines
            if ((i + 1) % 16 != 0) {
                for (int j = 0; j < 16 - ((i + 1) % 16); j++) {
                    printf("   "); // 3 spaces to align with "%02x "
                }
            }

            // Print ASCII characters
            printf(" | ");
            int start = (i / 16) * 16;
            for (int j = start; j <= i; j++) {
                if (isprint(buffer[j])) { // Check if the character is printable
                    printf("%c", buffer[j]);
                } else {
                    printf("."); // Print '.' for non-printable characters
                }
            }
            printf("\n");
        }
    }
}

void print_mac_address(unsigned char *mac) {
    printf("%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    printf("\n");
}

int main() {
    int sockfd;
    struct sockaddr_ll saddr;
    unsigned char buffer[BUFFER_SIZE];
    struct ifreq ifr;
    char *iface = "eth0"; // Change this to your network interface

    // Create a raw socket
    if ((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Get the index of the network interface
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface, IFNAMSIZ);
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the network interface
    memset(&saddr, 0, sizeof(saddr));
    saddr.sll_family = AF_PACKET;
    saddr.sll_protocol = htons(ETH_P_ALL);
    saddr.sll_ifindex = ifr.ifr_ifindex;
    if (bind(sockfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Capturing Ethernet frames on interface %s...\n", iface);

    while (1) {
        int recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, NULL, NULL);
        if (recv_len < 0) {
            perror("recvfrom");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        // Parse the Ethernet frame
        struct ethhdr *eth = (struct ethhdr *)buffer;

        printf("Source MAC: ");
        print_mac_address(eth->h_source);

        printf("Destination MAC: ");
        print_mac_address(eth->h_dest);

        printf("Ethernet Type: 0x%04x\n", ntohs(eth->h_proto));
        
        // Print payload
        unsigned char *payload = buffer + 14; // Payload offset
        
        printf("Payload:\n");
        print_buffer_in_hex(payload,64);
    
        printf("Total %d byte(s)\n",recv_len);
        printf("------------------\n");

    }

    close(sockfd);
    return 0;
}