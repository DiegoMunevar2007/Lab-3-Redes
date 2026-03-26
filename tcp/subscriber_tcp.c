#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8000

int main()
{
    // Gracias https://www.geeksforgeeks.org/computer-networks/udp-client-server-using-connect-c-implementation/ por tanto
    int sockfd;
    struct sockaddr_in server_addr;

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);
    server_addr.sin_family = AF_INET;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        printf("Error creando socket\n");
        return 1;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("\n Error : Connect Failed \n");
        exit(0);
    }

    char topic[50];
    char buffer[1024];

    printf("Ingrese topic a suscribirse: ");
    scanf("%s", topic);

    // Se arma el mensaje de suscripción con formato "SUB topic"
    char sub_msg[64];
    sprintf(sub_msg, "SUB %s", topic); // Se guarda en sub_msg a partir de topic

    // Se usa send() y no sendto() porque el socket ya está conectado con connect().
    // Usar sendto() con dirección explícita en un socket conectado retorna EISCONN en Linux
    // y el mensaje nunca se envía. send() usa la dirección guardada por connect().
    send(sockfd, sub_msg, strlen(sub_msg), 0);

    printf("Suscrito a %s\n", topic);

    // Escuchar mensajes del broker indefinidamente

    while (1)
    {
        ssize_t n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0)
        {
            printf("Conexion cerrada o error\n");
            break;
        }

        buffer[n] = '\0';
        printf("Mensaje recibido: %s\n", buffer);
    }

    // while (1)
    // {
    //     struct sockaddr_in from_addr;
    //     socklen_t from_len = sizeof(from_addr);

    //     ssize_t n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
    //                          (struct sockaddr *)&from_addr,
    //                          &from_len);
    //     if (n < 0)
    //     {
    //         printf("Error recibiendo mensaje\n");
    //         break;
    //     }

    //     buffer[n] = '\0';
    //     printf("Mensaje recibido: %s\n", buffer);
    // }

    close(sockfd);
    return 0;
}
