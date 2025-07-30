import socket

host, port = "127.0.0.1", 1984
data = "1,2,3"

socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

try:
    socket.connect((host, port))
    print(f"Connected at {host}:{port}")
    socket.sendall(data.encode("utf-8"))
    response = socket.recv(1024).decode("utf-8")
    print(response)
finally:
    socket.close()