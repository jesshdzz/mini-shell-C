/*
    * cliente.c
    * Equipo 1
    *
    * IMPORTANTE: Si el programa muestra errores con las librerias de readline,
    *             favor de instalar la libreria readline-dev: sudo apt install readline-dev
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdbool.h>

#define LECTURA 0
#define ESCRITURA 1
#define ERROR -1
#define MAX_LINEA 2048
#define MAX_ARGS 100
#define MAX_COMANDOS 10
#define MAX_RUTA 1024
#define PUERTO 1666
#define IP_SERVIDOR "172.18.0.1"
#define MAX_BUFFER_SIZE 2048

int sockfd = -1; // Variable global para el socket
bool conexion = false; // Variable global para la conexión

// Para evitar que Ctrl+C y Ctrl+Z cierren el programa
void manejar_senial(int senial) {
    printf("\n\n\033[1;33m(!) Usa 'exit' o Ctrl+D para salir.\033[0m\n\n");
    rl_on_new_line();
    rl_redisplay();
}

// Función para mostrar el prompt estilo shell
char* mostrar_prompt() {
    char* usuario = getenv("USER");
    char ruta[MAX_RUTA];
    char prompt[MAX_RUTA + 512];
    if (getcwd(ruta, sizeof(ruta)) != NULL) {
        snprintf(prompt, sizeof(prompt), "%s@mini-shell:%s$ ", usuario ? usuario : "user", ruta);
    } else {
        perror("getcwd");
        snprintf(prompt, sizeof(prompt), "$ ");
    }
    return strdup(prompt);
}

// Función para dividir una cadena en tokens por un delimitador
int dividir(char* entrada, char* delim, char** resultado) {
    int i = 0;
    resultado[i] = strtok(entrada, delim); // Primer token
    while (resultado[i] != NULL && i < MAX_ARGS - 1) {
        resultado[++i] = strtok(NULL, delim);
    }
    return i;
}

// Función para conectarse al servidor
void conectar_servidor() {
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == ERROR) {
        perror("Error creando el socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in servidor;
    memset(&servidor, 0, sizeof(servidor));
    servidor.sin_family = AF_INET;
    servidor.sin_port = htons(PUERTO);
    if (inet_pton(AF_INET, IP_SERVIDOR, &servidor.sin_addr) <= 0) {
        perror("Error en inet_pton");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    if (connect(sockfd, (struct sockaddr*)&servidor, sizeof(servidor)) == ERROR) {
        perror("Error conectando al servidor");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Conectado al servidor %s:%d\n", IP_SERVIDOR, PUERTO);
    conexion = true;
}

// Función para la interacción con el servidor
void comunicacion_servidor(const char* mensaje) {
    if (conexion) {
        // Enviar la línea al servidor
        if (send(sockfd, mensaje, strlen(mensaje), 0) == ERROR) {
            perror("Error enviando al servidor");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        char buffer[MAX_BUFFER_SIZE];
        ssize_t bytes_recibidos = recv(sockfd, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
        if (bytes_recibidos > 0) {
            buffer[bytes_recibidos] = '\0';
            printf("\n\033[1;41;37m[servidor] %s\033[0m\n", buffer);
            close(sockfd);
            exit(EXIT_SUCCESS);
        }
    }
}

int main() {
    char linea[MAX_LINEA];
    char* comandos[MAX_COMANDOS];
    char* args[MAX_COMANDOS][MAX_ARGS];

    signal(SIGINT, manejar_senial);  // Ctrl+C
    signal(SIGTSTP, manejar_senial); // Ctrl+Z

    conectar_servidor();
    int pipe_salida[2];

    printf("\033[1;35m\nBienvenido al \033[1;36mMini Shell - Equipo 1\033[0m\n");

    while (1) {
        char* entrada = readline(mostrar_prompt());
        if (!entrada) {
            printf("\nSaliendo del mini-shell...\n");
            break;
        }

        if (*entrada) add_history(entrada);
        strncpy(linea, entrada, MAX_LINEA);
        free(entrada);

        if (strcmp(linea, "exit") == 0) {
            printf("\nSaliendo del mini-shell...\n");
            break;
        }

        linea[strcspn(linea, "\n")] = '\0';  // Quitar el caracter fin de cadena

        comunicacion_servidor(linea); // Enviar al servidor

        int num_comandos = dividir(linea, "|", comandos);
        // Separar los argumentos de cada comando
        for (int i = 0; i < num_comandos; i++) {
            while (*comandos[i] == ' ') comandos[i]++; // Saltar espacios
            dividir(comandos[i], " \t", args[i]);
        }

        int fd[2]; // Pipe
        int prev_fd = -1;  // Pipe anterior
        pid_t pid;
        if (pipe(pipe_salida) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < num_comandos; i++) {
            if (i < num_comandos - 1) {
                if (pipe(fd) == ERROR) { // Crear pipe solo si hay otro comando después
                    perror("pipe");
                    close(sockfd);
                    exit(EXIT_FAILURE);
                }
            }

            pid = fork();
            if (pid == ERROR) {
                perror("fork");
                close(sockfd);
                exit(EXIT_FAILURE);
            }

            if (pid == 0) { // Proceso hijo
                if (i > 0) { // Si no es el primer comando
                    dup2(prev_fd, STDIN_FILENO); // Entrada desde el pipe anterior
                    close(prev_fd);
                }

                if (i < num_comandos - 1) { // Si no es el último comando
                    close(fd[LECTURA]);
                    dup2(fd[ESCRITURA], STDOUT_FILENO); // Salida hacia el siguiente comando
                    close(fd[ESCRITURA]);
                }

                if (i == num_comandos - 1) { // Es el último comando
                    close(pipe_salida[LECTURA]);
                    dup2(pipe_salida[ESCRITURA], STDOUT_FILENO);
                    close(pipe_salida[ESCRITURA]);
                }

                // Ejecutar el comando
                execvp(args[i][0], args[i]); // argv[i][0] es el nombre del comando, args[i] es el array de argumentos ls -l -r | grep "txt" execvp("ls", "ls", "-l", "-r", NULL);
                perror("Error al ejecutar comando"); // Codigo solo alcanzado si execvp falla
                exit(EXIT_FAILURE);
            } else { // Proceso padre
                if (i > 0) {
                    close(prev_fd); // Cerrar el fd de lectura anterior si no es el primer comando
                }

                if (i < num_comandos - 1) { // Si no es el último comando, guardar el fd de lectura
                    close(fd[ESCRITURA]);
                    prev_fd = fd[LECTURA];
                }
            }
        }

        close(pipe_salida[ESCRITURA]);
        char buffer[MAX_BUFFER_SIZE];
        ssize_t bytes_leidos;
        int status;

        // Esperar que todos los hijos terminen
        for (int i = 0; i < num_comandos; i++) {
            wait(&status);
        }

        while ((bytes_leidos = read(pipe_salida[LECTURA], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_leidos] = '\0';
            printf("\n%s\n", buffer);  // Mostrar en local
            comunicacion_servidor(buffer);  // Enviar al servidor
        }
        close(pipe_salida[LECTURA]);

        // for (int i = 0; i < num_comandos; i++) { // Esperar a que todos los hijos terminen
        //     if (wait(NULL) == ERROR) {
        //         perror("wait");
        //         close(sockfd);
        //         exit(EXIT_FAILURE);
        //     }
        // }
    }
    printf("\033[1;31m\nSesión terminada.\033[0m\n\n");
    if (conexion) {
        close(sockfd);
        conexion = false;
    }
    return EXIT_SUCCESS;
}


/*
    ls -l /usr/bin | grep python | sort -r | head -n 5                  # Ver los 5 archivos más grandes en /usr/bin que contengan "python"
    ps -ely | grep -v grep | grep -v PID | sort -n | head -n 10         // Ver los 10 procesos más grandes
    du -ah /var | sort -rh | head -n 10                                 // Ver los 10 archivos más grandes en /var
    ss -tuln | cut -d: -f2 | sort -u | head -n 4 | sort -r              // Ver puertos abiertos
*/