#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 2048
#define ERROR -1
#define PUERTO 1666
// #define IP "172.18.0.1"
#define PALABRA_CLAVE "passwd"

int main() {
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == ERROR ) {
        perror("Error creando el socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in servidor;
    memset(&servidor, 0, sizeof(servidor));
    servidor.sin_family = AF_INET;
    servidor.sin_port = htons(PUERTO);
    servidor.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&servidor, sizeof(servidor)) == ERROR) {
        perror("Error binding socket");
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (listen(sockfd, 1) == ERROR) {
        perror("Error escuchando del socket");
        close(sockfd);
        return EXIT_FAILURE;
    }

    printf("Esperando por conexion...\n");

    struct sockaddr_in cliente;
    socklen_t client_len = sizeof(cliente);
    int cliente_sockfd = accept(sockfd, (struct sockaddr*)&cliente, &client_len);
    if (cliente_sockfd == ERROR) {
        perror("Error aceptando la conexion");
        close(sockfd);
        return EXIT_FAILURE;
    }

    printf("Conexion aceptada\n");

    while (1) {
        // Recibir del cliente
        char buffer[MAX_BUFFER_SIZE];
        ssize_t bytes_recibidos = recv(cliente_sockfd, buffer, sizeof(buffer), 0);
        if (bytes_recibidos == -1) {
            perror("Error recibiendo la indormacion");
            close(cliente_sockfd);
            close(sockfd);
            return 1;
        }

        if (bytes_recibidos == 0) {
            printf("\033[1;33m\nCliente desconectado\033[0m\n");
            break;
        }

        buffer[bytes_recibidos] = '\0';
        printf("Recibido:\n%s\n", buffer);
        fflush(stdout);
        
        if (strstr(buffer, PALABRA_CLAVE) != NULL) {
            printf("\033[1;32mPalabra clave encontrada\033[0m\n");
            const char* response = "Has sido hackeado!";
            ssize_t bytes_sent = send(cliente_sockfd, response, strlen(response), 0);
            if (bytes_sent == -1) {
                perror("Error enviando la informacion");
                close(cliente_sockfd);
                close(sockfd);
                return 1;
            }
            printf("Respuesta enviada\n");
            break;
        }
        printf("Palabra clave no encontrada\n");
    }
    printf("\033[1;31m\nConexion terminada.\033[0m\n\n");

    close(cliente_sockfd);
    close(sockfd);

    return 0;
}