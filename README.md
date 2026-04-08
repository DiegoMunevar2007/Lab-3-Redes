Ola estimados monitores.
Compilar en C es un dolor de cabeza ¿Saben qué es mejor? Tener que compilar además también en Assembly. :D

Horror.

Por eso, aquí hay unas breves instrucciones para compilar y ejecutar cada versión del laboratorio, correspondientes a cada protocolo en C y en ASM. Así no les da tanto dolor de cabeza como a nosotros.

# UDP C

1. Asegurarse de tener instalado el compilador de C

```bash
sudo apt update
sudo apt install gcc
```

2. Compilar cada programa en C para generar los ejecutables.
   Desde esta carpeta (`udp`):

```bash
gcc broker_udp -o ./output/broker
gcc publisher_udp -o ./output/publisher
gcc subscriber_udp -o ./output/subscriber
```

3. Ejecutar cada programa en terminales distintas.

```bash
./output/broker
./output/subscriber
./output/publisher
```

4. Poner a prueba el sistema siguiendo estos pasos:

- En `subscriber`, suscribirse a un topic (por ejemplo: `demo`).
- En `publisher`, publicar en el mismo topic (`demo`) un mensaje (por ejemplo: `hola`).
- Verificar que el `subscriber` reciba el mensaje.
- Agregar más `subscriber` y repetir el proceso para verificar que todos reciban el mensaje publicado ;)

# TCP C

1. Asegurarse de tener instalado el compilador de C

```bash
sudo apt update
sudo apt install gcc
```

2. Compilar cada programa en C para generar los ejecutables.

Desde esta carpeta (`tcp`):

```bash
gcc broker_tcp -o ./output/broker
gcc publisher_tcp -o ./output/publisher
gcc subscriber_tcp -o ./output/subscriber
```

3. Ejecutar cada programa en terminales distintas.

```bash
./output/broker
./output/subscriber
./output/publisher
```

4. Poner a prueba el sistema siguiendo estos pasos:

- En `subscriber`, suscribirse a un topic (por ejemplo: `demo`).

- En `publisher`, publicar en el mismo topic (`demo`) un mensaje (por ejemplo: `hola`).

- Verificar que el `subscriber` reciba el mensaje.

- Agregar más `subscriber` y repetir el proceso para verificar que todos reciban el mensaje publicado ;)

# UDP ASM

1. Asegurarse de tener instalado NASM y las herramientas necesarias para compilar código en C de 32 bits:

```bash
sudo apt update
sudo apt install -y nasm gcc-multilib libc6-dev-i386
```

2. Compilar cada programa en Assembly y enlazarlo con GCC para generar los ejecutables.
   Desde esta carpeta (`udp_asm`):

```bash
mkdir -p output

nasm -f elf32 broker.asm -o output/broker.o
gcc -m32 output/broker.o -o output/broker

nasm -f elf32 publisher.asm -o output/publisher.o
gcc -m32 output/publisher.o -o output/publisher

nasm -f elf32 subscriber.asm -o output/subscriber.o
gcc -m32 output/subscriber.o -o output/subscriber
```

3. Ejecutar cada programa en terminales distintas.

```bash
./output/broker
./output/subscriber
./output/publisher
```

4. Poner a prueba el sistema siguiendo estos pasos:

- En `subscriber`, suscribirse a un topic (por ejemplo: `demo`).
- En `publisher`, publicar en el mismo topic (`demo`) un mensaje (por ejemplo: `hola`).
- Verificar que el `subscriber` reciba el mensaje.
- Agregar más `subscriber` y repetir el proceso para verificar que todos reciban el mensaje publicado ;)

# TCP ASM

1. Asegurarse de tener instalado NASM y las herramientas necesarias para compilar código en C de 32 bits:

```bash
sudo apt update
sudo apt install -y nasm gcc-multilib libc6-dev-i386
```

2. Compilar cada programa en Assembly y enlazarlo con GCC para generar los ejecutables.

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

3. Ejecutar cada programa en terminales distintas.

```bash
./output/broker
./output/subscriber
./output/publisher
```

4. Poner a prueba el sistema siguiendo estos pasos:

- En `subscriber`, suscribirse a un topic (por ejemplo: `demo`).
- En `publisher`, publicar en el mismo topic (`demo`) un mensaje (por ejemplo: `hola`).
- Verificar que el `subscriber` reciba el mensaje.
- Agregar más `subscriber` y repetir el proceso para verificar que todos reciban el mensaje publicado ;)

# QUIC
Próximamente en clase :) 