# ssh_server/Dockerfile
FROM alpine:latest 
#chọn image cơ bản

# Cài đặt OpenSSH Server 
# --no-cache giúp giảm kích thước image bằng cách tránh lưu cache của gói cài đặt.
RUN apk add --no-cache openssh-server 
# Bật xác thực bằng mật khẩu
RUN echo 'PasswordAuthentication yes' >> /etc/ssh/sshd_config

# Tạo user seed
# Đặt thư mục home cho user là /home/seed
# -s /bin/sh: Sử dụng shell /bin/sh làm shell mặc định
# -D: Tạo user mà không cần đặt mật khẩu ngay lập tức
RUN adduser -h /home/seed -s /bin/sh -D seed
# Thiết lập mật khẩu
RUN echo -n 'seed:dees' | chpasswd

# Thêm thư mục .ssh và thiết lập quyền
# Tạo thư mục .ssh
RUN mkdir -p /home/seed/.ssh && \
    # Đặt quyền chỉ user seed có thể truy cập
    chmod 700 /home/seed/.ssh && \
    # Tạo file authorized_keys
    touch /home/seed/.ssh/authorized_keys && \
    # Chỉ user seed có thể đọc/ghi file
    chmod 600 /home/seed/.ssh/authorized_keys && \
    #Đảm bảo quyền sở hữu đúng
    chown -R seed:seed /home/seed/.ssh

# Copy script entrypoint vào container
COPY entrypoint.sh /

# Gán quyền thực thi cho entrypoint.sh
RUN chmod +x -v entrypoint.sh

# Định nghĩa entrypoint
# Khi container khởi động, nó sẽ chạy script entrypoint.sh
ENTRYPOINT [ "./entrypoint.sh" ]

# Mở port 22 (SSH)
EXPOSE 22