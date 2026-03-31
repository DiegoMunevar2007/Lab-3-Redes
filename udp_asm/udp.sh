mkdir -p output

nasm -f elf32 broker.asm -o output/broker.o
nasm -f elf32 publisher.asm -o output/publisher.o
nasm -f elf32 subscriber.asm -o output/subscriber.o

gcc -m32 output/broker.o -o output/broker
gcc -m32 output/publisher.o -o output/publisher
gcc -m32 output/subscriber.o -o output/subscriber

rm output/*.o

