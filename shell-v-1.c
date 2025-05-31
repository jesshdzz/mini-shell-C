#include <stdio.h>      // Para entrada/salida estándar (printf, etc.)
#include <stdlib.h>     // Para funciones estándar (malloc, exit, etc.)
#include <string.h>     // Para manejo de strings (strcpy, strtok, etc.)
#include <unistd.h>     // Para llamadas al sistema POSIX (fork, pipe, etc.)
#include <sys/wait.h>   // Para manejo de procesos hijos (waitpid)
#include <termios.h>    // Para control del terminal (modos no canónicos)
#include <ctype.h>      // Para funciones de caracteres (isprint)

// Límites definidos para el programa
#define MAX_CMDS 10     // Máximo número de comandos en un pipeline
#define MAX_ARGS 20     // Máximo número de argumentos por comando
#define MAX_HISTORY 100 // Máximo número de comandos en el historial
#define LINE_LEN 1024   // Longitud máxima de una línea de comando

// Estructura para manejar el historial de comandos
typedef struct {
    char commands[MAX_HISTORY][LINE_LEN]; // Array para almacenar comandos
    int count;    // Contador de comandos en el historial
    int current;  // Índice del comando actual al navegar el historial
} History;

// Variable global que almacena el historial, inicializada a cero
History history = { .count = 0, .current = 0 };

// Función para configurar el terminal en modo no canónico (sin buffering)
void set_terminal_mode() {
    struct termios term;                  // Estructura de configuración del terminal
    tcgetattr(STDIN_FILENO, &term);      // Obtener configuración actual
    term.c_lflag &= ~(ICANON | ECHO);    // Desactivar entrada canónica y eco
    tcsetattr(STDIN_FILENO, TCSANOW, &term); // Aplicar nueva configuración
}

// Función para restaurar el terminal a su modo normal
void reset_terminal_mode() {
    struct termios term;                 // Estructura de configuración
    tcgetattr(STDIN_FILENO, &term);     // Obtener configuración actual
    term.c_lflag |= (ICANON | ECHO);    // Reactivar entrada canónica y eco
    tcsetattr(STDIN_FILENO, TCSANOW, &term); // Aplicar configuración
}

// Función para leer una línea con soporte para historial y edición básica
// Función para leer una línea con soporte para historial y edición básica
int read_line_with_history(char *line) {
    set_terminal_mode();
    
    int pos = 0;
    int len = 0;
    char c;
    
    while (1) {
        c = getchar();
        
        if (c == '\n') {
            line[len] = '\0';
            reset_terminal_mode();
            printf("\n");
            return len;
        } else if (c == 27) { // Secuencia de escape (flechas)
            getchar(); // Leer el '['
            c = getchar();
            
            if (c == 'A' && history.current > 0) { // Flecha arriba
                // Limpiar la línea visualmente
                printf("\r"); // Retorno de carro
                printf("\033[K"); // Borrar hasta el final de la línea
                
                history.current--;
                strncpy(line, history.commands[history.current], LINE_LEN);
                len = strlen(line);
                pos = len;
                
                printf(">> %s", line);
                fflush(stdout);
            } else if (c == 'B' && history.current < history.count - 1) { // Flecha abajo
                // Limpiar la línea visualmente
                printf("\r"); // Retorno de carro
                printf("\033[K"); // Borrar hasta el final de la línea
                
                history.current++;
                strncpy(line, history.commands[history.current], LINE_LEN);
                len = strlen(line);
                pos = len;
                
                printf(">> %s", line);
                fflush(stdout);
            }
        } else if (c == 127 || c == 8) { // Backspace
            if (pos > 0) {
                memmove(&line[pos-1], &line[pos], len - pos);
                pos--;
                len--;
                line[len] = '\0';
                
                printf("\r>> %s \b", line);
                fflush(stdout);
            }
        } else if (isprint(c)) { // Caracter imprimible
            if (len < LINE_LEN - 1) {
                if (pos < len) {
                    memmove(&line[pos+1], &line[pos], len - pos);
                }
                line[pos] = c;
                pos++;
                len++;
                line[len] = '\0';
                
                printf("\r>> %s", line);
                if (pos < len) {
                    printf("\033[%dD", len - pos);
                }
                fflush(stdout);
            }
        }
    }
}

// Función para ejecutar un pipeline de comandos
void ejecutar_pipeline(char **comandos, int n_cmds) {
    int pipes[MAX_CMDS - 1][2];  // Array de pipes para conectar comandos
    pid_t pids[MAX_CMDS];        // Array de PIDs de procesos hijos

    // Crear pipes para conectar los comandos
    for (int i = 0; i < n_cmds - 1; i++) {
        if (pipe(pipes[i]) == -1) {  // Crear pipe
            perror("pipe");          // Manejar error
            exit(EXIT_FAILURE);
        }
    }

    // Crear un proceso hijo para cada comando
    for (int i = 0; i < n_cmds; i++) {
        pids[i] = fork();  // Crear proceso hijo
        if (pids[i] == -1) {
            perror("fork"); // Manejar error
            exit(EXIT_FAILURE);
        }

        if (pids[i] == 0) { // Código del proceso hijo
            // Redirigir entrada si no es el primer comando
            if (i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO); // Conectar a pipe anterior
            }

            // Redirigir salida si no es el último comando
            if (i < n_cmds - 1) {
                dup2(pipes[i][1], STDOUT_FILENO); // Conectar a pipe siguiente
            }

            // Cerrar todos los pipes en el hijo
            for (int j = 0; j < n_cmds - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Tokenizar los argumentos del comando
            char *args[MAX_ARGS];
            int arg_count = 0;
            char *token = strtok(comandos[i], " \t"); // Primer token

            while (token != NULL && arg_count < MAX_ARGS - 1) {
                args[arg_count++] = token;  // Almacenar argumento
                token = strtok(NULL, " \t"); // Siguiente token
            }
            args[arg_count] = NULL;  // Terminar lista de argumentos

            // Ejecutar el comando
            execvp(args[0], args);  // Reemplazar imagen del proceso
            perror("execvp");       // Solo se ejecuta si falla execvp
            exit(EXIT_FAILURE);
        }
    }

    // Código del proceso padre
    // Cerrar todos los pipes en el padre
    for (int i = 0; i < n_cmds - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Esperar a que todos los hijos terminen
    for (int i = 0; i < n_cmds; i++) {
        waitpid(pids[i], NULL, 0);
    }
}

// Función para procesar un comando (puede ser un pipeline)
void procesar_comando(char *linea) {
    if (!linea || strlen(linea) == 0) {  // Si línea vacía
        return;  // No hacer nada
    }

    // Agregar al historial (excepto si es igual al anterior)
    if (history.count == 0 || strcmp(history.commands[history.count-1], linea) != 0) {
        if (history.count < MAX_HISTORY) {  // Si hay espacio
            strcpy(history.commands[history.count], linea); // Copiar comando
            history.count++;  // Incrementar contador
        } else {
            // Rotar el historial si está lleno (eliminar el más viejo)
            for (int i = 0; i < MAX_HISTORY-1; i++) {
                strcpy(history.commands[i], history.commands[i+1]);
            }
            strcpy(history.commands[MAX_HISTORY-1], linea); // Agregar al final
        }
    }
    history.current = history.count;  // Resetear posición en historial

    char *comandos[MAX_CMDS];  // Array para comandos del pipeline
    int n_cmds = 0;            // Contador de comandos

    // Separar los comandos por '|'
    char *token = strtok(linea, "|");
    while (token != NULL && n_cmds < MAX_CMDS) {
        // Eliminar espacios en blanco al inicio y final
        while (*token == ' ' || *token == '\t') token++;
        char *end = token + strlen(token) - 1;
        while (end > token && (*end == ' ' || *end == '\t' || *end == '\n')) end--;
        *(end + 1) = '\0';

        comandos[n_cmds++] = token;  // Almacenar comando
        token = strtok(NULL, "|");   // Siguiente token
    }

    if (n_cmds > 0) {  // Si hay comandos válidos
        ejecutar_pipeline(comandos, n_cmds);  // Ejecutar pipeline
    }
}

// Función principal
int main() {
    printf("Mi Shell Básico (Ctrl+C para salir)\n");

    char linea[LINE_LEN];  // Buffer para la línea de comando
    
    while (1) {  // Bucle principal
        printf(">> ");     // Mostrar prompt
        fflush(stdout);    // Forzar salida
        
        int len = read_line_with_history(linea);  // Leer línea
        if (len == 0) continue;  // Si línea vacía, continuar
        
        procesar_comando(linea);  // Procesar comando
    }

    return 0;
}

/*
Compilar: gcc -Wall -o proyecto_shell proyecto_shell.c
Ejecutar: ./proyecto_shell
Ejemplo de uso:
>> ls -l
>> ps -ely | grep bash
>> ls -l /usr/bin | grep python | sort -r
> cat /var/log/syslog | grep error | cut -d' ' -f5- | sort | uniq -c
>> find /home -name "*.txt" -type f | xargs wc -l | sort -n
>> du -ah /var | sort -rh | head -n 10
*/