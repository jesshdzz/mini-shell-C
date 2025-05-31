#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_LINEA 1024
#define MAX_COMANDOS 10
#define MAX_ARGS 100
#define ERROR -1

void manejar_senial(int senial) { // Para evitar que Ctrl+C y Ctrl+Z cierren el programa
    printf("\nUsa Ctrl+D para salir.\n");
}

int dividir(char* entrada, char* delim, char** resultado) {  // Función para dividir una cadena en tokens por un delimitador
    int i = 0;
    resultado[i] = strtok(entrada, delim);
    while (resultado[i] != NULL) {
        resultado[++i] = strtok(NULL, delim);
    }
    return i;
}

int main() {
    char linea[MAX_LINEA];
    char* comandos[MAX_COMANDOS];
    char* args[MAX_COMANDOS][MAX_ARGS];

    signal(SIGINT, manejar_senial);  // Ctrl+C
    signal(SIGTSTP, manejar_senial); // Ctrl+Z

    while (1) {
        printf("mini-shell@equipo1:~$ ");

        if (fgets(linea, sizeof(linea), stdin) == NULL) { 
            printf("\nSaliendo del mini-shell.\n");
            break;
        }

        linea[strcspn(linea, "\n")] = '\0';  // Quitar salto de línea

        // Separar la línea por comandos
        int num_comandos = dividir(linea, "|", comandos);

        // Separar los argumentos de cada comando
        for (int i = 0; i < num_comandos; i++) {
            while (*comandos[i] == ' ') comandos[i]++; // Saltar espacios
            dividir(comandos[i], " \t", args[i]);
        }

        int fd[2], prev_fd = -1;
        pid_t pid;

        for (int i = 0; i < num_comandos; i++) {
            if (i < num_comandos - 1) {
                if (pipe(fd) == ERROR) { // Crear pipe solo si hay otro comando después
                    perror("Error creando pipe");
                    return EXIT_FAILURE;
                }
            }

            pid = fork();
            if (pid == -1) {
                perror("Error al hacer fork");
                return EXIT_FAILURE;
            }
            
            if (pid == 0) {
                // Proceso hijo
                if (i > 0) {
                    dup2(prev_fd, 0); // Entrada desde el pipe anterior
                    close(prev_fd);
                }
                if (i < num_comandos - 1) {
                    close(fd[0]);
                    dup2(fd[1], 1); // Salida hacia el siguiente comando
                    close(fd[1]);
                }
                execvp(args[i][0], args[i]);
                perror("Error al ejecutar comando");
                exit(1);
            } else if (pid < 0) {
                perror("Error al hacer fork");
                exit(1);
            } else {
                // Proceso padre
                if (i > 0) close(prev_fd);
                if (i < num_comandos - 1) {
                    close(fd[1]);
                    prev_fd = fd[0];
                }
            }
        }

        // Esperar a todos los hijos
        for (int i = 0; i < num_comandos; i++) {
            wait(NULL);
        }
    }
    return 0;
}
