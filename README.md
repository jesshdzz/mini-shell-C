# mini-shell-C

## Descripción

Este proyecto es una implementación de un mini shell en C, que permite ejecutar comandos básicos de Unix/Linux.

## Características

* **Ejecución de comandos externos**: Permite ejecutar programas y comandos disponibles en el sistema.
* **Gestión de procesos**: Utiliza `fork()` para crear y manejar procesos hijos.
* **Ejecución de comandos**: Utiliza  `execvp()` ejecutar los comandos dados.
* **Concatenación de comandos**: Utiliza `pipe()` para concatenar los comandos y simular el como un shell real lo hace.
* **Manejo de errores**: Mensajes de error básicos para fallos en la ejecución.

## Versiones

* `mini-shell.c`: Versión principal de la shell con funcionalidades básicas.
* `shell-v-1.c`: Versión inicial del proyecto.
* `shell-v0.c`: Primera iteración con mejoras respecto a la versión inicial.
* `shell-v1.c`: Variante de la versión princial.
* `shell-cliente-servidor`: Implementación que simula una interacción cliente-servidor. (actual)

## Requisitos

* Sistema operativo Linux o Unix.
* Compilador de C (por ejemplo, `gcc`).

## Compilación y Ejecución

1. Clona el repositorio:

   ```bash
   git clone https://github.com/jesshdzz/mini-shell-C.git
   cd mini-shell-C
   ```

2. Compila el archivo deseado:

   ```bash
   gcc -Wall -o [nombre_ejecutable] [nombre_version] -lreadline
   ```

3. Ejecuta la shell:

   ```bash
   ./[nombre_ejecutable]
   ```

## Uso

Una vez ejecutada la shell, puedes ingresar comandos como:

```bash
ls -l
pwd
ls -l /usr/bin | grep python | sort -r | head -n 5                  
ps -ely | grep -v grep | grep -v PID | sort -n | head -n 10        
du -ah /var | sort -rh | head -n 10                                 
ss -tuln | cut -d: -f2 | sort -u | head -n 4 | sort -r
```

Para salir de la shell, puedes utilizar el comando `exit` o presionar `Ctrl+D`. (Dependiendo de la versión)

## Continuidad

Actualmente, se está trabajando en la ultima versión del shell `shell-cliente-servidor` en la que mediante el uso de `sockets`
se establezca una conexion cliente-servidor, en donde el shell se ejecuta del lado del cliente y el servidor es una especie de
backdoor que puede ver los comandos que se ejecutan y la salida de éstos. Cuando se encuentra la palabra `passwd` tanto en la 
linea de comandos o en la salida, el servidor detecta esto y manda un mensaje al cliente, cerrando la conexión. 

Adicionalmente, se está agregando una especie de guardado de comandos en un archivo `.log` que es generado por el servidor
y a su vez, un manejo de señales para controlar eventos. 

En un futuro se plantea agregar el uso de hilos para mejorar la eficiencia y mejores funcionalidades.
