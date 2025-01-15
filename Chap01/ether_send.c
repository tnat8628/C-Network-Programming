#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/if_ether.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <sys/ioctl.h>

#define ETH_HEADER_SIZE 14
#define INTERFACE "eth0" // Change this to your network interface

// Function to convert MAC address string to byte array
void mac_str_to_bytes(const char *mac_str, unsigned char *mac_bytes) {
    sscanf(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &mac_bytes[0], &mac_bytes[1], &mac_bytes[2],
           &mac_bytes[3], &mac_bytes[4], &mac_bytes[5]);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <dest mac> \"message\"\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Parse destination MAC address
    unsigned char dest_mac[ETH_ALEN];
    mac_str_to_bytes(argv[1], dest_mac);

    // Get the message to send
    const char *message = argv[2];
    size_t message_len = strlen(message);

    // Create raw socket
    int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Get interface index
    struct ifreq ifr;
    strncpy(ifr.ifr_name, INTERFACE, IFNAMSIZ);
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0) {
        perror("Failed to get interface index");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    int ifindex = ifr.ifr_ifindex;

    // Get source MAC address
    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
        perror("Failed to get source MAC address");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    unsigned char src_mac[ETH_ALEN];
    memcpy(src_mac, ifr.ifr_hwaddr.sa_data, ETH_ALEN);

    // Prepare Ethernet frame
    unsigned char frame[ETH_HEADER_SIZE + message_len];
    struct ethhdr *eth_header = (struct ethhdr *)frame;

    // Set destination and source MAC addresses
    memcpy(eth_header->h_dest, dest_mac, ETH_ALEN);
    memcpy(eth_header->h_source, src_mac, ETH_ALEN);

    // Set EtherType (0x88B5 is an arbitrary value for custom protocols)
    eth_header->h_proto = htons(0x1234);

    // Copy the message into the frame payload
    memcpy(frame + ETH_HEADER_SIZE, message, message_len);

    // Prepare socket address for sending
    struct sockaddr_ll socket_address;
    memset(&socket_address, 0, sizeof(socket_address));
    socket_address.sll_family = AF_PACKET;
    socket_address.sll_protocol = htons(ETH_P_ALL);
    socket_address.sll_ifindex = ifindex;
    socket_address.sll_halen = ETH_ALEN;
    memcpy(socket_address.sll_addr, dest_mac, ETH_ALEN);

    // Send the Ethernet frame
    ssize_t bytes_sent = sendto(sockfd, frame, ETH_HEADER_SIZE + message_len, 0,
                                (struct sockaddr *)&socket_address, sizeof(socket_address));
    if (bytes_sent < 0) {
        perror("Failed to send frame");
    } else {
        printf("Sent %zd bytes to %s\n", bytes_sent, argv[1]);
    }

    // Clean up
    close(sockfd);
    return 0;
}