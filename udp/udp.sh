mkdir -p output

gcc broker_udp.c -o output/broker
gcc publisher_udp.c -o output/publisher
gcc subscriber_udp.c -o output/subscriber