#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <ctype.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <sys/socket.h>

#define DNS_SERVER "127.0.0.1"
#define DNS_PORT 5053
#define SPOOF_IP "6.6.6.6"

// DNS header structure
struct dns_header {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
};

// DNS question section structure
struct dns_question {
    uint16_t qtype;
    uint16_t qclass;
};

// DNS resource record structure
struct dns_rr {
    uint16_t type;
    uint16_t class;
    uint32_t ttl;
    uint16_t rdlength;
    unsigned char rdata[];
};


int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    char buffer[1024];
    socklen_t len = sizeof(cliaddr);

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(53);

    if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    while(1) {
        // Receive DNS query from client
        int n = recvfrom(sockfd, buffer, 1024, 0, 
                       (struct sockaddr*)&cliaddr, &len);

        // Forward to DNSMASQ
        struct sockaddr_in dns_addr;
        memset(&dns_addr, 0, sizeof(dns_addr));
        dns_addr.sin_family = AF_INET;
        dns_addr.sin_port = htons(DNS_PORT);
        inet_pton(AF_INET, DNS_SERVER, &dns_addr.sin_addr);

        int forward_sock = socket(AF_INET, SOCK_DGRAM, 0);
        sendto(forward_sock, buffer, n, 0, 
              (struct sockaddr*)&dns_addr, sizeof(dns_addr));

        // Get DNS response
        char response[1024];
        socklen_t dns_len = sizeof(dns_addr);
        int m = recvfrom(forward_sock, response, 1024, 0, 
                        (struct sockaddr*)&dns_addr, &dns_len);
        close(forward_sock);
        
        unsigned char *dns_payload = (unsigned char *)(response);
        if (is_dns_response(dns_payload)){
            printf("DNS Response Detected, original:\n");
            print_buffer_in_hex(dns_payload, 96);
        }    
        

        /* ------------------------------------------
           Students implement DNS spoofing here:
           - Parse DNS response
           - Modify IP address in response
           - Recalculate DNS checksum if needed
        ------------------------------------------ */

        // Send modified response back to client
        sendto(sockfd, response, m, 0, 
              (struct sockaddr*)&cliaddr, len);
    }

    close(sockfd);
    return 0;
}