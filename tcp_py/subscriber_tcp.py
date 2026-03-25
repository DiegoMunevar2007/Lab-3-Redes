import socket

PORT = 5000
BUFFER_SIZE = 1024
SERVER = ("127.0.0.1", PORT)

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(SERVER)

topic = input("Ingrese topic a suscribirse: ")

# Enviar sub
sock.sendall(f"SUB {topic}".encode())
print(f"Suscrito a {topic}")

while True:
    data = sock.recv(BUFFER_SIZE)
    if not data:
        break
    print(f"Mensaje recibido: {data.decode()}")