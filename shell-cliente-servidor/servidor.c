#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define NEGRO       "\x1b[1;30m"
#define ROJO        "\x1b[1;31m"
#define VERDE       "\x1b[1;32m"
#define AMARILLO    "\x1b[1;33m"
#define AZUL        "\x1b[1;34m"
#define MAGENTA     "\x1b[1;35m"
#define CIAN        "\x1b[1;36m"
#define BLANCO      "\x1b[37m"
#define RESET       "\x1b[0m"

#define MAX_BUFFER_SIZE 2048
#define ERROR -1
#define PUERTO 1666
#define LOG_FILE "shell.log"
// #define IP "172.18.0.1"
#define PALABRA_CLAVE "passwd"
#define RESPUESTA_1 "Has sido hackeado! "
#define INTERR "supercalifragilisticoespiralidoso"
#define RESPUESTA_2 "No es posible interrumpir utilizando ctrl+C"

int main() {
    int servidor_sockfd;
    if ((servidor_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == ERROR) {
        perror("Error creando el socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_in servidor;
    memset(&servidor, 0, sizeof(servidor));
    servidor.sin_family = AF_INET;
    servidor.sin_port = htons(PUERTO);
    servidor.sin_addr.s_addr = INADDR_ANY;

    if (bind(servidor_sockfd, (struct sockaddr*)&servidor, sizeof(servidor)) == ERROR) {
        perror("Error binding socket");
        close(servidor_sockfd);
        return EXIT_FAILURE;
    }

    if (listen(servidor_sockfd, 1) == ERROR) {
        perror("Error escuchando del socket");
        close(servidor_sockfd);
        return EXIT_FAILURE;
    }

    printf(BLANCO "Esperando por conexion..." RESET "\n");

    struct sockaddr_in cliente;
    socklen_t cliente_len = sizeof(cliente);
    int cliente_sockfd = accept(servidor_sockfd, (struct sockaddr*)&cliente, &cliente_len);
    if (cliente_sockfd == ERROR) {
        perror("Error aceptando la conexion");
        close(servidor_sockfd);
        return EXIT_FAILURE;
    }

    printf(VERDE "Conexion aceptada" RESET "\n");

    FILE* log;
    log = fopen(LOG_FILE, "a");
    if (!log) {
        perror("fopen");
        close(cliente_sockfd);
        close(servidor_sockfd);
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Recibir del cliente
        char buffer[MAX_BUFFER_SIZE];
        ssize_t bytes_recibidos = recv(cliente_sockfd, buffer, sizeof(buffer), 0);
        if (bytes_recibidos == ERROR) {
            perror("Error recibiendo la indormacion");
            fclose(log);
            close(cliente_sockfd);
            close(servidor_sockfd);
            return EXIT_FAILURE;
        }

        if (bytes_recibidos == 0) {
            printf(AMARILLO "\nCliente desconectado" RESET "\n");
            break;
        }

        buffer[bytes_recibidos] = '\0';
        printf(CIAN "\nRecibido:" RESET "\n%s\n", buffer);
        fflush(stdout);

        time_t ahora = time(NULL);
        char* tiempo = ctime(&ahora);
        tiempo[strlen(tiempo) - 1] = '\0'; // quitar \n
        fprintf(log, "[%s]: \n%s\n", tiempo, buffer);
        fflush(log);

        if (strstr(buffer, PALABRA_CLAVE) != NULL) {
            printf(VERDE "Palabra clave encontrada" RESET "\n");
            ssize_t bytes_sent = send(cliente_sockfd, RESPUESTA_1, strlen(RESPUESTA_1), 0);
            if (bytes_sent == ERROR) {
                perror("Error enviando la informacion");
                fclose(log);
                close(cliente_sockfd);
                close(servidor_sockfd);
                return EXIT_FAILURE;
            }
            printf(AZUL "Respuesta enviada" RESET "\n");
            break;
        } else if (strstr(buffer, INTERR) != NULL) {
            printf(AMARILLO "Se intentó cerrar el shell" RESET "\n");
            ssize_t bytes_sent = send(cliente_sockfd, RESPUESTA_2, strlen(RESPUESTA_2), 0);
            if (bytes_sent == ERROR) {
                perror("Error evitando la interrupcion");
                fclose(log);
                close(cliente_sockfd);
                close(servidor_sockfd);
                return EXIT_FAILURE;
            }
            printf(AZUL "Interrupcion evitada" RESET "\n");
            continue;
        } else {
            ssize_t bytes_sent = send(cliente_sockfd, " ", 1, 0);
            if (bytes_sent == ERROR) {
                perror("Error enviando la informacion");
                fclose(log);
                close(cliente_sockfd);
                close(servidor_sockfd);
                return EXIT_FAILURE;
            }
            printf(NEGRO "Palabra clave no encontrada" RESET "\n");
        }
    }

    printf(ROJO "\nConexion terminada." RESET "\n\n");

    time_t ahora = time(NULL);
    char* tiempo = ctime(&ahora);
    tiempo[strlen(tiempo) - 1] = '\0'; // quitar \n
    fprintf(log, "[%s]\nCliente desconectado.\n\n", tiempo);
    fflush(log);

    fclose(log);
    close(cliente_sockfd);
    close(servidor_sockfd);

    return 0;
}