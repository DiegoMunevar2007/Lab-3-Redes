#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 5000

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
    char message[1024];

    printf("Ingrese topic: ");
    scanf("%s", topic);

    // limpiar buffer
    getchar();

    printf("Ingrese mensaje: ");
    scanf("%[^\n]", message); // Leer hasta el salto de línea para permitir espacios en el mensaje

    // Se arma el mensaje con formato "PUB topic mensaje"
    char final_msg[1200];
    snprintf(final_msg, sizeof(final_msg), "PUB %s %s\n", topic, message); // Se guarda en final_msg a partir de topic y message

    // Se usa send() y no sendto() porque el socket ya está conectado con connect().
    // Usar sendto() con dirección explícita en un socket conectado retorna EISCONN en Linux
    // y el mensaje nunca se envía. send() usa la dirección guardada por connect().
    send(sockfd, final_msg, strlen(final_msg), 0);

    printf("Mensaje enviado\n");

    close(sockfd);
    return 0;
}
