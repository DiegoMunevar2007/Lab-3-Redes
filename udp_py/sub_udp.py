import socket

PORT = 5000
BUFFER_SIZE = 1024
SERVER = ("127.0.0.1", PORT)

socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

topic = input("Ingrese topic a suscribirse: ")

# Enviar suscripción
socket.sendto(f"SUB {topic}".encode(), SERVER)
print(f"Suscrito a {topic}")

while True:
    data, _ = socket.recvfrom(BUFFER_SIZE)
    print(f"Mensaje recibido: {data.decode()}")