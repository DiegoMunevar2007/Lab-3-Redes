import socket

PORT = 5000
BUFFER_SIZE = 1024

# Diccionario: topic -> lista de (ip, port)
topics = {}

# Crear socket UDP
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(("0.0.0.0", PORT))

print(f"Broker UDP escuchando en el puerto {PORT}...")

while True:
    data, addr = sock.recvfrom(BUFFER_SIZE) # Se bloquea hasta recibir un mensaje
    msg = data.decode() # Decodificar bytes a string
    print(f"Mensaje recibido de {addr}: {msg}")

    if msg.startswith("SUB"): # Si es una suscripción, se extrae el topic y se agrega el cliente a la lista de suscriptores
        _, topic = msg.split(maxsplit=1)
        if topic not in topics:
            topics[topic] = []
        if addr not in topics[topic]:
            topics[topic].append(addr)
        print(f"Cliente {addr} suscrito a {topic}")

    elif msg.startswith("PUB"): # Si es una publicación, se extrae el topic y el mensaje, y se envía a todos los suscriptores de ese topic
        parts = msg.split(maxsplit=2)
        if len(parts) < 3:
            continue
        _, topic, message = parts
        if topic in topics:
            for subscriber in topics[topic]:
                sock.sendto(message.encode(), subscriber) # Se envía el mensaje (codificado a bytes) a cada suscriptor
            print(f"Publicado '{message}' a {len(topics[topic])} suscriptores de {topic}")