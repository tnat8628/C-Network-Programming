import socket
print("creating socket")
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  #tạo socket, AF_INET: IPv4, SOCK_STREAM: TCP
print("connecting to server...")
dhost = "google.com"
dport = 80
res = s.connect_ex((dhost, dport))
if res == 0:
    print(f"Connected to server {dhost} at port {dport}.")
else:
    print(f"Failed to connect to server {dhost} at port {dport}.")
s.close()