Ola estimados monitores. 
Compilar en C es un dolor de cabeza ¿Saben qué es mejor? Tener que compilar además también en Assembly. :D 

Horror.

Por eso, aquí hay unas breves instrucciones para compilar y ejecutar cada versión del laboratorio, correspondientes a cada protocolo en C y en ASM. Así no les da tanto dolor de cabeza como a nosotros.

# UDP C
# TCP C

# UDP ASM
# TCP ASM

Instrucciones breves para compilar y ejecutar `broker`, `publisher` y `subscriber` en Linux (32 bits con NASM).

## Requisitos

```bash
sudo apt update
sudo apt install -y nasm gcc-multilib libc6-dev-i386
```

## Compilar

Desde esta carpeta (`tcp_asm`):

```bash
mkdir -p output

nasm -f elf32 broker.asm -o output/broker.o
gcc -m32 output/broker.o -o output/broker

nasm -f elf32 publisher.asm -o output/publisher.o
gcc -m32 output/publisher.o -o output/publisher

nasm -f elf32 subscriber.asm -o output/subscriber.o
gcc -m32 output/subscriber.o -o output/subscriber
```

## Ejecutar

En 3 terminales distintas, dentro de `tcp_asm`:

```bash
./output/broker
./output/subscriber
./output/publisher
```

## Flujo de prueba rapido

1. En `subscriber`, suscribirse a un topic (por ejemplo: `demo`).
2. En `publisher`, publicar en el mismo topic (`demo`) un mensaje (por ejemplo: `hola`).
3. Verificar que el `subscriber` reciba el mensaje.

