import argparse
import asyncio

from aioquic.asyncio import QuicConnectionProtocol, connect
from aioquic.quic.configuration import QuicConfiguration
from aioquic.quic.events import StreamDataReceived


class SubscriberProtocol(QuicConnectionProtocol):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.queue: asyncio.Queue[str] = asyncio.Queue()

    def quic_event_received(self, event):
        if isinstance(event, StreamDataReceived) and event.end_stream:
            text = event.data.decode("utf-8", errors="ignore").strip()
            if text:
                self.queue.put_nowait(text)


async def main(host: str, port: int) -> None:
    config = QuicConfiguration(is_client=True, alpn_protocols=["hq-29"])
    config.verify_mode = False

    async with connect(
        host,
        port,
        configuration=config,
        create_protocol=SubscriberProtocol,
    ) as client:
        protocol = client
        print("Connected to broker")

        topic = input("Topic: ").strip()
        while not topic:
            topic = input("Topic: ").strip()

        subscribe = f"SUB {topic}".encode("utf-8")
        stream_id = protocol._quic.get_next_available_stream_id()
        protocol._quic.send_stream_data(stream_id, subscribe, end_stream=True)
        protocol.transmit()

        print(f"Subscribed to {topic}")
        print("Waiting messages...")

        while True:
            msg = await protocol.queue.get()
            print(f"Message: {msg}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="QUIC Pub/Sub subscriber")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=4433)
    args = parser.parse_args()

    asyncio.run(main(args.host, args.port))
