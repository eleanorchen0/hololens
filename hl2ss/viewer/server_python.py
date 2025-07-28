import socket
import json

HOST = "127.0.0.1"
PORT = 65432

marker_data = {
    "id" : 1,
    "position" : {"x" : 0, "y" : 0, "z" : 0},
    "rotation" : {"x" : 0, "y" : 0, "z" : 0, "w" : 0},
}

json_data = json.dumps(marker_data)
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((HOST, PORT))
s.send(json_data)
s.close()
