#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define PORT 8000
#define MAX_SUBS 100
#define BUFFER_SIZE 1024

// Estructura equivalente al diccionario topics{} del broker Python:
// topics = { topic: [socket_fd, ...] }
typedef struct {
    int socket_fd;
    char topic[50];
} Subscriber;

Subscriber subscribers[MAX_SUBS]; // Lista de suscriptores
int sub_count = 0;

// Verificar si ya existe suscripción
int already_subscribed(int socket_fd, char *topic) {
    for (int i = 0; i < sub_count; i++) {
        if (subscribers[i].socket_fd == socket_fd &&
            strcmp(subscribers[i].topic, topic) == 0) {
            return 1;
        }
    }
    return 0;
}

// Registrar suscriptor
void add_subscriber(int socket_fd, char *topic) {
    if (sub_count >= MAX_SUBS) {
        printf("No se pueden registrar más suscriptores\n");
        fflush(stdout);
        return;
    }

    if (!already_subscribed(socket_fd, topic)) {
        subscribers[sub_count].socket_fd = socket_fd;
        strcpy(subscribers[sub_count].topic, topic);
        sub_count++;

        printf("Socket %d suscrito a %s\n", socket_fd, topic);
        fflush(stdout);
    }
}

// Eliminar suscripciones de un socket desconectado
void remove_subscriber(int socket_fd) {
    for (int i = 0; i < sub_count; i++) {
        if (subscribers[i].socket_fd == socket_fd) {
            printf("Eliminando suscripción de socket %d\n", socket_fd);
            fflush(stdout);

            subscribers[i] = subscribers[sub_count - 1];
            sub_count--;
            i--;
        }
    }
}

// Publicar mensaje a suscriptores
void publish(char *topic, char *message) {
    int sent = 0;

    for (int i = 0; i < sub_count; i++) {
        if (strcmp(subscribers[i].topic, topic) == 0) {
            send(subscribers[i].socket_fd, message, strlen(message), 0);
            sent++;
        }
    }

    printf("Publicado '%s' a %d suscriptores de %s\n", message, sent, topic);
    fflush(stdout);
}

int main() {
    // Gracias https://www.geeksforgeeks.org/socket-programming-cc/ por tanto
    int listener, newfd, fdmax;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    char buffer[BUFFER_SIZE];

    fd_set master_set, read_fds;

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);
    server_addr.sin_family = AF_INET;

    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        perror("Error creando socket");
        return 1;
    }

    // Evitar "Bind Failed"
    int opt = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(listener);
        return 1;
    }

    // Bind
    if (bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(listener);
        return 1;
    }

    // Listen
    if (listen(listener, 10) < 0) {
        perror("Listen failed");
        close(listener);
        return 1;
    }

    FD_ZERO(&master_set);
    FD_SET(listener, &master_set);
    fdmax = listener;

    printf("Broker TCP escuchando en el puerto %d...\n", PORT);
    fflush(stdout);

    while (1) {
        read_fds = master_set;

        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("Error en select");
            continue;
        }

        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {

                // Nueva conexión
                if (i == listener) {
                    client_len = sizeof(client_addr);
                    newfd = accept(listener, (struct sockaddr *)&client_addr, &client_len);

                    if (newfd < 0) {
                        perror("Error en accept");
                        continue;
                    }

                    FD_SET(newfd, &master_set);
                    if (newfd > fdmax) fdmax = newfd;

                    printf("Nueva conexión TCP desde %s:%d en socket %d\n",
                           inet_ntoa(client_addr.sin_addr),
                           ntohs(client_addr.sin_port),
                           newfd);
                    fflush(stdout);
                }

                // Mensaje de cliente
                else {
                    int n = recv(i, buffer, sizeof(buffer) - 1, 0);

                    if (n <= 0) {
                        if (n == 0) {
                            printf("Socket %d desconectado\n", i);
                        } else {
                            perror("Error en recv");
                        }

                        close(i);
                        FD_CLR(i, &master_set);
                        remove_subscriber(i);
                    } else {
                        buffer[n] = '\0';

                        printf("Mensaje recibido en socket %d: %s\n", i, buffer);
                        fflush(stdout);

                        if (strncmp(buffer, "SUB", 3) == 0) {
                            char topic[50];
                            int matched = sscanf(buffer, "SUB %49s", topic);

                            if (matched == 1) {
                                add_subscriber(i, topic);
                            } else {
                                printf("Formato inválido SUB\n");
                                fflush(stdout);
                            }
                        }

                        else if (strncmp(buffer, "PUB", 3) == 0) {
                            char topic[50];
                            char message[BUFFER_SIZE];

                            int matched = sscanf(buffer, "PUB %49s %[^\n]", topic, message);

                            if (matched == 2) {
                                publish(topic, message);
                            } else {
                                printf("Formato inválido PUB\n");
                                fflush(stdout);
                            }
                        }

                        else {
                            printf("Mensaje no reconocido\n");
                            fflush(stdout);
                        }
                    }
                }
            }
        }
    }

    close(listener);
    return 0;
}