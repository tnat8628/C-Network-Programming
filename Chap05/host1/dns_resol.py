import socket

# Lấy tên máy chủ hiện tại
hostname = socket.gethostname()
print("Tên máy chủ hiện tại:", hostname)

# Lấy IP từ tên máy chủ
ip_local = socket.gethostbyname(hostname)
print("Địa chỉ IP cục bộ:", ip_local)

# Lấy thông tin chi tiết từ tên miền
try:
    details = socket.gethostbyname_ex("google.com")
    print("Thông tin google.com:", details)
except socket.gaierror as e:
    print("Lỗi phân giải google.com:", e)

# Phân giải ngược từ IP
try:
    reverse_info = socket.gethostbyaddr("8.8.8.8")
    print("Thông tin từ 8.8.8.8:", reverse_info)
except socket.herror as e:
    print("Lỗi phân giải ngược:", e)