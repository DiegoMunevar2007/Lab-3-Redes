mkdir -p output

gcc broker_tcp.c -o output/broker
gcc publisher_tcp.c -o output/publisher
gcc subscriber_tcp.c -o output/subscriber