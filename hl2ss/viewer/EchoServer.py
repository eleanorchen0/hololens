import socket

HOST = '127.0.0.1'  # Unity's IP
PORT = 65432

unity_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
unity_socket.connect((HOST, PORT))
print("[Python] Connected to Unity")

message = "Hello from Python!\n"
unity_socket.sendall(message.encode('utf-8'))

response = unity_socket.recv(1024)
print("[Python] Received:", response.decode('utf-8'))

unity_socket.close()