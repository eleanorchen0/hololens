import socket
import json

unity_ip = "10.29.244.211"
unity_port = 65432

unity_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
try:
    unity_socket.connect((unity_ip, unity_port))
    print(f"Connected to server at {unity_ip}:{unity_port}")
    
except Exception as e:
    print(f"Failed to connect : {e}")
    unity_socket = None