import socket

HOST = "127.0.0.1"
PORT = 5000
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((HOST, PORT))

while True:
    topic = input("Ingrese el tema a publicar: ")
    message = input("Ingrese el mensaje a publicar: ")
    full_message = f"PUB {topic} {message}"
    sock.sendall(full_message.encode())
    print(f"Publicado {message} en el tema {topic}")
