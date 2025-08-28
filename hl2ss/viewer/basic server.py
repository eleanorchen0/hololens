#------------------------------------------------------------------------------
import socket

unity_host, unity_port = "0.0.0.0", 1984

server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind((unity_host, unity_port))
server_socket.listen(1)

print(f"Python Server listening on {unity_host}:{unity_port}")
conn, addr = server_socket.accept()
print(f"Connection from {addr}")

coordinates = {1, 1, 1}

conn.sendall(coordinates.encode('utf-8'))
server_socket.close()
conn.close()
#------------------------------------------------------------------------------