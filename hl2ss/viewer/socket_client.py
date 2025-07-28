import socket
import sys
import selectors
import types

host = "127.0.0.1"
port = 65432

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((host, port))
    s.sendall(b"Hello, world")
    data = s.recv(1024)

print(f"Received {data!r}")