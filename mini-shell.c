/*
    * mini-shell.c
    * Equipo 1
    * Un mini shell que soporta comandos encadenados con pipes.
    * Permite ejecutar comandos de forma similar a un shell real.
    *
    * Compilación: gcc -Wall -o mini-shell mini-shell.c -lreadline
    * Ejecución: ./mini-shell
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


#define STDIN 0
#define STDOUT 1
#define ERROR -1
#define MAX_LINEA 1024
#define MAX_ARGS 100
#define MAX_COMANDOS 10
#define MAX_RUTA 256

// Para evitar que Ctrl+C y Ctrl+Z cierren el programa
void manejar_senial(int senial) {
    printf("\nUsa Ctrl+D para salir.\n");
}

// Función para mostrar el prompt estilo shell
char* mostrar_prompt() {
    char* usuario = getenv("USER");
    char ruta[MAX_RUTA];
    char prompt[MAX_RUTA + 50];
    if (getcwd(ruta, sizeof(ruta)) != NULL) {
        snprintf(prompt, sizeof(prompt), "%s@mini-shell-eq1:%s$ ", usuario ? usuario : "user", ruta);
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

int main() {
    char linea[MAX_LINEA];
    char* comandos[MAX_COMANDOS];
    char* args[MAX_COMANDOS][MAX_ARGS];

    signal(SIGINT, manejar_senial);  // Ctrl+C
    signal(SIGTSTP, manejar_senial); // Ctrl+Z

    while (1) {
        char* entrada = readline(mostrar_prompt());
        if (!entrada) {
            printf("\nSaliendo del mini-shell.\n");
            break;
        }

        if (*entrada) add_history(entrada);
        strncpy(linea, entrada, MAX_LINEA);
        free(entrada);

        linea[strcspn(linea, "\n")] = '\0';  // Quitar el caracter fin de cadena

        int num_comandos = dividir(linea, "|", comandos);

        // Separar los argumentos de cada comando
        for (int i = 0; i < num_comandos; i++) {
            while (*comandos[i] == ' ') comandos[i]++; // Saltar espacios
            dividir(comandos[i], " \t", args[i]);
        }

        int fd[2]; // Pipe
        int prev_fd = -1;  // Pipe anterior
        pid_t pid;

        for (int i = 0; i < num_comandos; i++) {
            if (i < num_comandos - 1) {
                if (pipe(fd) == ERROR) { // Crear pipe solo si hay otro comando después
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }
            }

            pid = fork();
            if (pid == ERROR) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) { // Proceso hijo
                if (i > 0) { // Si no es el primer comando
                    dup2(prev_fd, 0); // Entrada desde el pipe anterior
                    close(prev_fd);
                }

                if (i < num_comandos - 1) { // Si no es el último comando
                    close(fd[STDIN]);
                    dup2(fd[STDOUT], 1); // Salida hacia el siguiente comando
                    close(fd[STDOUT]);
                }
                // Ejecutar el comando
                execvp(args[i][0], args[i]); // argv[i][0] es el nombre del comando, args[i] es el array de argumentos ls -l -r | grep "txt" execvp("ls", "ls", "-l", "-r", NULL);
                perror("Error al ejecutar comando"); // Codigo solo alcanzado si execvp falla
                exit(EXIT_FAILURE);
            } else { // Proceso padre
                if (i > 0) close(prev_fd); // Cerrar el fd de lectura anterior si no es el primer comando
                if (i < num_comandos - 1) { // Si no es el último comando, guardar el fd de lectura
                    close(fd[STDOUT]);
                    prev_fd = fd[STDIN];
                }
            }
        }

        for (int i = 0; i < num_comandos; i++) { // Esperar a que todos los hijos terminen
            if (wait(NULL) == ERROR) {
                perror("wait");
                exit(EXIT_FAILURE);
            }
        }
    }

    return EXIT_SUCCESS;
}


/*
    ls -l /usr/bin | grep python | sort -r | head -n 5                  # Ver los 5 archivos más grandes en /usr/bin que contengan "python"
    ps -ely | grep -v grep | grep -v PID | sort -n | head -n 10         // Ver los 10 procesos más grandes
    du -ah /var | sort -rh | head -n 10                                 // Ver los 10 archivos más grandes en /var
    ss -tuln | cut -d: -f2 | sort -u | head -n 4 | sort -r              // Ver puertos abiertos
*/