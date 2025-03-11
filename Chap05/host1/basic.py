import socket
print("creating socket")
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
print("connecting to server...")
dhost = "google.com"
dport = 80
s.connect((dhost, dport))
print(f"Connected to server {dhost} at port {dport}.")
s.close()