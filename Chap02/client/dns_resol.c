
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define DNS_SERVER_IP "8.8.8.8"  // Google's public DNS server
#define DNS_SERVER_PORT 53
#define BUFFER_SIZE 512

void error(const char *msg) {
    perror(msg);
    exit(1);
}

typedef struct {
    unsigned short id; // identification number

    unsigned char rd :1; // recursion desired
    unsigned char tc :1; // truncated message
    unsigned char aa :1; // authoritative answer
    unsigned char opcode :4; // purpose of message
    unsigned char qr :1; // query/response flag

    unsigned char rcode :4; // response code
    unsigned char cd :1; // checking disabled
    unsigned char ad :1; // authenticated data
    unsigned char z :1; // its z! reserved
    unsigned char ra :1; // recursion available

    unsigned short q_count; // number of question entries
    unsigned short ans_count; // number of answer entries
    unsigned short auth_count; // number of authority entries
    unsigned short add_count; // number of resource entries
} DNS_HEADER;

typedef struct {
    unsigned short qtype;
    unsigned short qclass;
} QUESTION;

#pragma pack(push, 1)
typedef struct {
    unsigned short type;
    unsigned short _class;
    unsigned int ttl;
    unsigned short data_len;
} R_DATA;
#pragma pack(pop)

void change_to_dns_name_format(unsigned char* dns, unsigned char* host) {
    int lock = 0, i;
    strcat((char*)host, ".");
    for (i = 0; i < strlen((char*)host); i++) {
        if (host[i] == '.') {
            *dns++ = i - lock;
            for (; lock < i; lock++) {
                *dns++ = host[lock];
            }
            lock++;
        }
    }
    *dns++ = '\0';
}

unsigned char* read_name(unsigned char* reader, unsigned char* buffer, int* count) {
    unsigned char *name;
    unsigned int p = 0, jumped = 0, offset;
    int i, j;

    *count = 1;
    name = (unsigned char*)malloc(256);

    name[0] = '\0';

    while (*reader != 0) {
        if (*reader >= 192) {
            offset = (*reader) * 256 + *(reader + 1) - 49152;
            reader = buffer + offset - 1;
            jumped = 1;
        } else {
            name[p++] = *reader;
        }

        reader = reader + 1;

        if (jumped == 0) {
            *count = *count + 1;
        }
    }

    name[p] = '\0';

    if (jumped == 1) {
        *count = *count + 1;
    }

    for (i = 0; i < (int)strlen((const char*)name); i++) {
        p = name[i];
        for (j = 0; j < (int)p; j++) {
            name[i] = name[i + 1];
            i = i + 1;
        }
        name[i] = '.';
    }
    name[i - 1] = '\0';
    return name;
}

void print_dns_response(unsigned char *buffer, DNS_HEADER *dns, unsigned char *qname) {
    unsigned char *reader = &buffer[sizeof(DNS_HEADER) + (strlen((const char*)qname) + 1) + sizeof(QUESTION)];
    printf("Server:  dns.google\n");
    printf("Address:  %s\n\n", DNS_SERVER_IP);

    printf("Non-authoritative answer:\n");
    printf("Name:    %s\n", qname);

    for (int i = 0; i < ntohs(dns->ans_count); i++) {
        reader = reader + 2;
        R_DATA *rdata = (R_DATA*)reader;
        reader = reader + sizeof(R_DATA);

        if (ntohs(rdata->type) == 1) { // If the response is an IPv4 address
            struct sockaddr_in a;
            memcpy(&a.sin_addr.s_addr, reader, sizeof(a.sin_addr.s_addr));
            printf("Address:  %s\n", inet_ntoa(a.sin_addr));
            reader = reader + ntohs(rdata->data_len);
        } else if (ntohs(rdata->type) == 28) { // If the response is an IPv6 address
            char ipv6[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, reader, ipv6, INET6_ADDRSTRLEN);
            printf("Address:  %s\n", ipv6);
            reader = reader + ntohs(rdata->data_len);
        } else {
            reader = reader + ntohs(rdata->data_len);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3 || strcmp(argv[1], "nslookup") != 0) {
        fprintf(stderr, "Usage: %s nslookup <hostname>\n", argv[0]);
        exit(1);
    }

    int sockfd;
    struct sockaddr_in dest;
    unsigned char buffer[BUFFER_SIZE], *qname;
    DNS_HEADER *dns = NULL;
    QUESTION *qinfo = NULL;

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        error("Socket creation failed");
    }

    // DNS server address
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(DNS_SERVER_PORT);
    if (inet_pton(AF_INET, DNS_SERVER_IP, &dest.sin_addr) <= 0) {
        error("Invalid DNS server address");
    }

    // Set up the DNS query
    dns = (DNS_HEADER *)&buffer;
    dns->id = (unsigned short) htons(getpid());
    dns->qr = 0; // This is a query
    dns->opcode = 0; // This is a standard query
    dns->aa = 0; // Not Authoritative
    dns->tc = 0; // This message is not truncated
    dns->rd = 1; // Recursion Desired
    dns->ra = 0; // Recursion not available
    dns->z = 0;
    dns->ad = 0;
    dns->cd = 0;
    dns->rcode = 0;
    dns->q_count = htons(1); // we have only 1 question
    dns->ans_count = 0;
    dns->auth_count = 0;
    dns->add_count = 0;

    // Point to the query portion
    qname = (unsigned char*)&buffer[sizeof(DNS_HEADER)];
    unsigned char hostname[256];
    strcpy((char*)hostname, argv[2]);
    change_to_dns_name_format(qname, hostname);

    qinfo = (QUESTION*)&buffer[sizeof(DNS_HEADER) + (strlen((const char*)qname) + 1)];
    qinfo->qtype = htons(1); // Type A query (IPv4)
    qinfo->qclass = htons(1); // Class IN (Internet)

    // Send the DNS query
    if (sendto(sockfd, buffer, sizeof(DNS_HEADER) + (strlen((const char*)qname) + 1) + sizeof(QUESTION), 0, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
        error("sendto failed");
    }

    // Receive the DNS response
    socklen_t len = sizeof(dest);
    int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&dest, &len);
    if (n < 0) {
        error("recvfrom failed");
    }

    // Print the response
    dns = (DNS_HEADER*) buffer;
    print_dns_response(buffer, dns, qname);

    close(sockfd);
    return 0;
}
