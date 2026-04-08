import argparse
import asyncio
from collections import defaultdict
from pathlib import Path

from aioquic.asyncio import QuicConnectionProtocol, serve
from aioquic.quic.configuration import QuicConfiguration
from aioquic.quic.events import StreamDataReceived


class BrokerState:
    def __init__(self) -> None:
        self.subscribers = defaultdict(set)

    def subscribe(self, topic: str, client: "BrokerProtocol") -> None:
        self.subscribers[topic].add(client)

    def unsubscribe_all(self, client: "BrokerProtocol") -> None:
        for topic in list(self.subscribers.keys()):
            clients = self.subscribers[topic]
            if client in clients:
                clients.remove(client)
            if not clients:
                del self.subscribers[topic]

    async def publish(self, topic: str, payload: str) -> None:
        clients = list(self.subscribers.get(topic, set()))
        print(f"Publish topic={topic} to {len(clients)} subscribers: {payload}")
        for client in clients:
            await client.send_message(payload)


class BrokerProtocol(QuicConnectionProtocol):
    def __init__(self, *args, state: BrokerState, **kwargs):
        super().__init__(*args, **kwargs)
        self.state = state

    def quic_event_received(self, event):
        if isinstance(event, StreamDataReceived):
            if not event.end_stream:
                return

            text = event.data.decode("utf-8", errors="ignore").strip()
            if not text:
                return

            if text.startswith("SUB "):
                topic = text[4:].strip()
                if topic:
                    self.state.subscribe(topic, self)
                    print(f"Subscriber registered on topic {topic}")
                return

            if text.startswith("PUB "):
                parts = text.split(" ", 2)
                if len(parts) < 3:
                    return
                _, topic, payload = parts
                if topic and payload:
                    asyncio.create_task(self.state.publish(topic, payload))
                return

    async def send_message(self, payload: str) -> None:
        stream_id = self._quic.get_next_available_stream_id()
        self._quic.send_stream_data(stream_id, payload.encode("utf-8"), end_stream=True)
        self.transmit()

    def connection_lost(self, exc):
        self.state.unsubscribe_all(self)
        super().connection_lost(exc)


async def main(host: str, port: int, cert_path: Path, key_path: Path) -> None:
    if not cert_path.exists() or not key_path.exists():
        raise FileNotFoundError(
            f"Missing cert or key. Expected {cert_path} and {key_path}."
        )

    config = QuicConfiguration(is_client=False, alpn_protocols=["hq-29"])
    config.load_cert_chain(certfile=str(cert_path), keyfile=str(key_path))

    state = BrokerState()

    print(f"QUIC broker listening on {host}:{port}")
    await serve(
        host,
        port,
        configuration=config,
        create_protocol=lambda *a, **k: BrokerProtocol(*a, state=state, **k),
    )

    await asyncio.Event().wait()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="QUIC Pub/Sub broker")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=4433)
    parser.add_argument("--cert", default="../quic/cert.crt")
    parser.add_argument("--key", default="../quic/cert.key")
    args = parser.parse_args()

    asyncio.run(main(args.host, args.port, Path(args.cert), Path(args.key)))
