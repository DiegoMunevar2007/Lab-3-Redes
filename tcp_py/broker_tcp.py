import asyncio
import socket
import threading
HOST = "0.0.0.0"
PORT = 5000
BUFFER_SIZE = 1024

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.bind((HOST, PORT))
sock.listen(5)

topics = {}  # topic -> (IP, puerto)

print(f"Broker TCP escuchando en el puerto {PORT}...")
    
    
def manejar_cliente(conn, addr):
    while True:
        try:
            data = conn.recv(BUFFER_SIZE)
            if not data:
                break

            msg = data.decode()
            print(f"Mensaje recibido de {addr}: {msg}")

            if msg.startswith("SUB"):
                _, topic = msg.split(maxsplit=1)
                topics.setdefault(topic, [])
                if conn not in topics[topic]:
                    topics[topic].append(conn)

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
                            pass

        except:
            break

    print(f"Cliente desconectado: {addr}")
    conn.close()

    for topic in topics:
        if conn in topics[topic]:
            topics[topic].remove(conn)


    
while True:
    conn, addr = sock.accept()
    print(f"Cliente conectado: {addr}")
    threading.Thread(target=manejar_cliente, args=(conn, addr), daemon=True).start()