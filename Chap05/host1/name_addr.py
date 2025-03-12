import socket

host = "microsoft.com"
addr = socket.gethostbyname(host)
#print(f"IP address of {host} is {addr}")

name,lst,addr_list = socket.gethostbyname_ex(host)
print(f"IP address of {name} is:")
for addr in addr_list:
    print(f"{addr}")