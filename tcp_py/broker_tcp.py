import socket

HOST = "0.0.0.0"
PORT = 5000
BUFFER_SIZE = 1024

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.bind((HOST, PORT))
sock.listen(5)

topics = {}  # topic -> (IP, puerto)

print(f"Broker TCP escuchando en el puerto {PORT}...")
    
while True:
    conn, addr = sock.accept()
    print(f"Cliente conectado: {addr}")
    # De pronto thread????? Pq solo está aceptando un cliente a la vez
    while True:
        try:
            data = conn.recv(BUFFER_SIZE)
            if not data:
                break

            msg = data.decode()
            print(f"Mensaje recibido de {addr}: {msg}")

            if msg.startswith("SUB"):
                _, topic = msg.split(maxsplit=1)
                if topic not in topics:
                    topics[topic] = []
                if conn not in topics[topic]:
                    topics[topic].append(conn)
                print(f"Cliente {addr} suscrito a {topic}")

            elif msg.startswith("PUB"):
                parts = msg.split(maxsplit=2)
                if len(parts) < 3:
                    continue

                _, topic, message = parts

                if topic in topics:
                    for subscriber in topics[topic]:
                        try:
                            subscriber.sendall(message.encode())
                        except:
                            pass  # cliente muerto

                    print(f"Publicado '{message}' a {len(topics[topic])} suscriptores de {topic}")

        except:
            break

    print(f"Cliente desconectado: {addr}")
    conn.close()

    # limpiar suscripciones
    for topic in topics:
        if conn in topics[topic]:
            topics[topic].remove(conn)

