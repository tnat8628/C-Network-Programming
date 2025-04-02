import socket
print("creating socket")
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
print("connecting to server...")
dhost = "google.com"
dport = 80
s.connect((dhost, dport))

#send http request
request = "GET / HTTP/1.1\r\nHost: google.com\r\n\r\n"
s.send(request.encode())
print(f"Connected to server {dhost} at port {dport}.")
response = s.recv(4096)
print(response.decode())
s.close()