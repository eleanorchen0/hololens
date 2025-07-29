import socket

host = "10.29.211.183"
port = 65432

ids =
marker_data = {
    "ids" : ids,
    "position" : {"x" : 0, "y" : 0, "z" : 0},
    "rotation" : {"x" : 0, "y" : 0, "z" : 0, "w" : 0}
}

def connect_to_server(host, port):
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect(host, port)
            print(f"Connected to server at {host}:{port}")

            message = marker_data
            s.sendall(message.encode('utf-8'))
            print(f"Sent {message}")

            while True:
                data = s.recv(1024)
                if not data:
                    break
                print("Received from server:", data.decode('utf-8'))

    except ConnectionRefusedError:
        print("Could not connect to server")
    except Exception as e:
        print(e)

if __name__ == "__main__":
    connect_to_server(host, port)


