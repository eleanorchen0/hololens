import socket

host, port = "127.0.0.1", 1984

data = "1,1,1,1,1,1,1"

server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind((host, port))
server_socket.listen(1)

print(f"Python Server listening on {host}:{port}")
conn, addr = server_socket.accept()
print(f"Connection from {addr}")

try:
    conn.sendall(data.encode('utf-8'))  # Echo back
finally:
    conn.close()
    server_socket.close()