#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_LINE 1024 // Tamaño máximo de la línea de entrada
#define MAX_CMDS 10   // Número máximo de comandos en una línea
#define MAX_ARGS 20  // Número máximo de argumentos por comando

// Maneja las señales SIGINT (Ctrl+C) y SIGTSTP (Ctrl+Z)
void handle_signal(int sig) {
    printf("\nCtrl+D para salir.\n");
}

// Muestra el prompt y lee la línea de entrada del usuario
int get_input_line(char* buffer, size_t size, const char* last_command) {
    char* user = getenv("USER"); // Obtiene el nombre del usuario
    char cwd[256]; // Buffer para el directorio actual

    if (getcwd(cwd, sizeof(cwd))) {
        printf("%s@shell:%s$ ", user ? user : "user", cwd); // Prompt personalizado
    } else {
        printf("$ ");
    }

    fflush(stdout); // Muestra el prompt de inmediato

    if (fgets(buffer, size, stdin) == NULL) return 0; // Si se presiona Ctrl+D, termina

    buffer[strcspn(buffer, "\n")] = '\0'; // Elimina el salto de línea

    // Si se escribe "!!", intenta recuperar el último comando
    if (strcmp(buffer, "!!") == 0) {
        if (last_command[0] == '\0') {
            printf("No hay comando anterior.\n");
            return -1; // No hay comando anterior aún
        } else {
            strcpy(buffer, last_command); // Sustituye "!!" por el comando anterior
            printf("%s\n", buffer); // Imprime el comando recuperado
        }
    }

    return 1; // Entrada válida
}

// Parsea la línea de entrada en comandos y sus argumentos
int parse_line(char* line, char* args[MAX_CMDS][MAX_ARGS]) {
    int num_cmds = 0; // Contador de comandos
    char* cmd = strtok(line, "|"); // Separa los comandos por '|'

    while (cmd && num_cmds < MAX_CMDS) {
        while (*cmd == ' ') cmd++; // Elimina espacios iniciales

        int arg_idx = 0;
        char* token = strtok(cmd, " \t"); // Separa cada argumento
        while (token && arg_idx < MAX_ARGS - 1) {
            args[num_cmds][arg_idx++] = token;
            token = strtok(NULL, " \t");
        }
        args[num_cmds][arg_idx] = NULL; // Termina el arreglo de argumentos
        num_cmds++;

        cmd = strtok(NULL, "|"); // Va al siguiente comando
    }

    return num_cmds; // Devuelve el número de comandos
}

// Ejecuta los comandos con pipes
void execute_commands(char* args[MAX_CMDS][MAX_ARGS], int num_cmds) {
    int pipe_fd[2]; // Para la comunicación entre procesos
    int prev_fd = -1; // Archivo del pipe anterior

    for (int i = 0; i < num_cmds; i++) {
        if (i < num_cmds - 1 && pipe(pipe_fd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork(); // Crea proceso hijo
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) { // Proceso hijo
            if (i > 0) {
                dup2(prev_fd, STDIN_FILENO); // Entrada desde el pipe anterior
                close(prev_fd);
            }
            if (i < num_cmds - 1) {
                close(pipe_fd[0]); // Cierra lectura
                dup2(pipe_fd[1], STDOUT_FILENO); // Salida al pipe actual
                close(pipe_fd[1]);
            }
            execvp(args[i][0], args[i]); // Ejecuta el comando
            perror("execvp");
            exit(EXIT_FAILURE);
        } else { // Proceso padre
            if (i > 0) close(prev_fd); // Cierra el anterior
            if (i < num_cmds - 1) {
                close(pipe_fd[1]); // Cierra escritura
                prev_fd = pipe_fd[0]; // Guarda lectura para siguiente
            }
        }
    }

    for (int i = 0; i < num_cmds; i++) wait(NULL); // Espera a todos los hijos
}

int main() {
    char line[MAX_LINE]; // Línea de entrada
    char* args[MAX_CMDS][MAX_ARGS]; // Arreglo para los comandos y argumentos
    char last_command[MAX_LINE] = ""; // Último comando ejecutado (para !!)

    signal(SIGINT, handle_signal);  // Ctrl+C
    signal(SIGTSTP, handle_signal); // Ctrl+Z

    while (1) {
        int read_status = get_input_line(line, sizeof(line), last_command);

        if (read_status == 0) break;         // Ctrl+D
        if (read_status == -1) continue;     // !! inválido

        strcpy(last_command, line);          // Guarda el último comando válido

        int num_cmds = parse_line(line, args); // Divide en comandos
        if (num_cmds > 0) {
            execute_commands(args, num_cmds); // Ejecuta los comandos
        }
    }

    printf("\nSaliendo...\n");
    return 0;
}
