import socket

# Server setup
HOST = '127.0.0.1'  # If Unity is running on same machine
PORT = 65432

server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind((HOST, PORT))
server_socket.listen()

print(f"[Python] Listening on {HOST}:{PORT}")

conn, addr = server_socket.accept()
print(f"[Python] Connected by {addr}")

while True:
    data = conn.recv(1024)
    if not data:
        break
    print(f"[Python] Received: {data.decode('utf-8')}")
    conn.sendall(data)  # Echo back