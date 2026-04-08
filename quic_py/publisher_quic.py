import argparse
import asyncio

from aioquic.asyncio import connect
from aioquic.quic.configuration import QuicConfiguration


async def main(host: str, port: int) -> None:
    config = QuicConfiguration(is_client=True, alpn_protocols=["hq-29"])
    config.verify_mode = False

    async with connect(host, port, configuration=config) as client:
        print("Connected to broker")

        topic = input("Topic: ").strip()
        while not topic:
            topic = input("Topic: ").strip()

        while True:
            message = input("Message: ").strip()
            if not message:
                continue

            payload = f"PUB {topic} {message}".encode("utf-8")
            stream_id = client._quic.get_next_available_stream_id()
            client._quic.send_stream_data(stream_id, payload, end_stream=True)
            client.transmit()
            print("Sent")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="QUIC Pub/Sub publisher")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=4433)
    args = parser.parse_args()

    asyncio.run(main(args.host, args.port))
