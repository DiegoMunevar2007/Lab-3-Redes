import socket

PORT = 5000
SERVER = ("127.0.0.1", PORT)

socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # AF_INET: IPv4, SOCK_DGRAM: UDP

topico = input("Ingrese topic a publicar: ")
mensaje = input("Ingrese mensaje: ")

# Enviar publicación
socket.sendto(f"PUB {topico} {mensaje}".encode(), SERVER)
print("Mensaje enviado")