#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 5000
#define MAX_SUBS 100

// Estructura equivalente al diccionario topics{} del broker Python:
// topics = { topic: [(ip, port), ...] }
// Aquí cada entrada guarda la dirección del suscriptor y su topic
typedef struct {
    struct sockaddr_in addr;
    char topic[50];
} Subscriber;

Subscriber subscribers[MAX_SUBS]; // Lista de suscriptores registrados
int sub_count = 0;                 // Cantidad actual de suscriptores

int main() {
    // Gracias https://www.geeksforgeeks.org/computer-networks/udp-client-server-using-connect-c-implementation/ por tanto
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[1024];

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);
    server_addr.sin_family = AF_INET;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        printf("Error creando socket\n");
        return 1;
    }

    // El broker hace bind() para quedarse escuchando en el puerto — equivalente a sock.bind() en Python
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("\n Error : Bind Failed \n");
        exit(0);
    }

    printf("Broker UDP escuchando en el puerto %d...\n", PORT);
    fflush(stdout); // Forzar escritura del buffer para que el mensaje se vea de inmediato

    while (1) {
        // Equivalente a: data, addr = sock.recvfrom(BUFFER_SIZE)
        socklen_t client_len = sizeof(client_addr);
        int n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                         (struct sockaddr *)&client_addr, &client_len);
        if (n < 0) {
            printf("Error recibiendo mensaje\n");
            fflush(stdout);
            continue;
        }
        buffer[n] = '\0'; 
        printf("Mensaje recibido de %s:%d: %s\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buffer);
        fflush(stdout); // Forzar escritura para que el log aparezca de inmediato

        if (strncmp(buffer, "SUB", 3) == 0) {
            // Equivalente a: _, topic = msg.split(maxsplit=1)
            char topic[50];
            sscanf(buffer, "SUB %49s", topic);

            // Verificar si el cliente ya está suscrito a ese topic — equivalente a: if addr not in topics[topic]
            int already = 0;
            for (int i = 0; i < sub_count; i++) {
                if (strcmp(subscribers[i].topic, topic) == 0 &&
                    subscribers[i].addr.sin_addr.s_addr == client_addr.sin_addr.s_addr &&
                    subscribers[i].addr.sin_port == client_addr.sin_port) {
                    already = 1;
                    break;
                }
            }

            // Registrar nuevo suscriptor — equivalente a: topics[topic].append(addr)
            if (!already && sub_count < MAX_SUBS) {
                subscribers[sub_count].addr = client_addr;
                strcpy(subscribers[sub_count].topic, topic);
                sub_count++;
                printf("Cliente %s:%d suscrito a %s\n",
                       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), topic);
                fflush(stdout); // Forzar escritura para que el log aparezca de inmediato
            }

        } else if (strncmp(buffer, "PUB", 3) == 0) {
            // Equivalente a: _, topic, message = parts
            char topic[50];
            char message[1024];
            sscanf(buffer, "PUB %49s %[^\n]", topic, message); // Leer hasta el salto de línea para permitir espacios en el mensaje

            // Reenviar a cada suscriptor del topic — equivalente a: for subscriber in topics[topic]: sock.sendto(...)
            int sent = 0;
            for (int i = 0; i < sub_count; i++) {
                if (strcmp(subscribers[i].topic, topic) == 0) {
                    sendto(sockfd, message, strlen(message), 0,
                           (struct sockaddr *)&subscribers[i].addr,
                           sizeof(subscribers[i].addr));
                    sent++;
                }
            }
            printf("Publicado '%s' a %d suscriptores de %s\n", message, sent, topic);
            fflush(stdout); // Forzar escritura para que el log aparezca de inmediato
        }
    }

    close(sockfd);
    return 0;
}
