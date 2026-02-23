// Shell personalizada para xv6
// Implementa un REPL (Read-Eval-Print Loop)
// Permite ejecutar comandos del sistema y comandos personalizados
// Incluye historial, limpiar pantalla, calculadora, hora simulada y pipes

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAXLINE 100
#define MAXARGS 10
#define HISTORY_SIZE 10

#ifndef START_TIME
#define START_TIME 0
#endif

#define TIMEZONE_OFFSET (-5 * 3600)
#define TICKS_PER_SECOND 100

// Secuencias ANSI para colores
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"
#define COLOR_RESET   "\033[0m"


// Arreglo que almacena los últimos 10 comandos ingresados
// Se usa como buffer circular
char history[HISTORY_SIZE][MAXLINE];
int history_count = 0;


// Guarda un comando en el historial
// Si se supera el tamaño máximo, comienza a sobreescribir
void add_history(char *cmd) {
  if (cmd[0] == '\0')
    return;

  int index = history_count % HISTORY_SIZE;
  strcpy(history[index], cmd);
  history_count++;
}


// Muestra los últimos comandos almacenados
void show_history() {
  printf(COLOR_CYAN "\nUltimos comandos:\n" COLOR_RESET);

  int start = history_count - HISTORY_SIZE;
  if (start < 0)
    start = 0;

  for (int i = start; i < history_count; i++) {
    int index = i % HISTORY_SIZE;
    printf("%d  %s\n", i + 1, history[index]);
  }

  printf("\n");
}


// Limpia la pantalla usando códigos ANSI
void clear_screen() {
  printf("\033[2J\033[H");
}


// Lee una línea desde la entrada estándar carácter por carácter
// Termina cuando encuentra ENTER o alcanza el límite
int readline(char *buf, int max) {
  int i = 0;
  char c;

  while (i < max - 1) {
    if (read(0, &c, 1) < 1)
      break;

    if (c == '\n')
      break;

    buf[i++] = c;
  }

  buf[i] = '\0';
  return i;
}


// Divide la línea en argumentos separados por espacios
// Convierte espacios en '\0' y construye el arreglo argv
void parse(char *buf, char **argv) {
  int i = 0;

  while (*buf != '\0') {

    while (*buf == ' ')
      *buf++ = '\0';

    if (*buf == '\0')
      break;

    argv[i++] = buf;

    while (*buf != ' ' && *buf != '\0')
      buf++;
  }

  argv[i] = 0;
}


// Ejecuta un comando usando fork, exec y wait
// El hijo ejecuta el programa y el padre espera su finalización
void run_cmd(char **argv) {
  int pid = fork();

  if (pid == 0) {
    exec(argv[0], argv);
    printf(COLOR_RED "Error ejecutando comando\n" COLOR_RESET);
    exit(1);
  } else {
    wait(0);
  }
}


// Ejecuta dos comandos conectados por un pipe
// El primer hijo escribe en el pipe y el segundo lee desde él
void run_pipe(char **left, char **right) {
  int p[2];
  pipe(p);

  if (fork() == 0) {
    close(1);
    dup(p[1]);
    close(p[0]);
    close(p[1]);
    exec(left[0], left);
    exit(1);
  }

  if (fork() == 0) {
    close(0);
    dup(p[0]);
    close(p[1]);
    close(p[0]);
    exec(right[0], right);
    exit(1);
  }

  close(p[0]);
  close(p[1]);

  wait(0);
  wait(0);
}


// Verifica si un string comienza con otro
int starts_with(char *str, char *prefix) {
  while (*prefix) {
    if (*str != *prefix)
      return 0;
    str++;
    prefix++;
  }
  return 1;
}


// Imprime números en formato de dos dígitos
void print_two_digits(int n) {
  if (n < 10)
    printf("0%d", n);
  else
    printf("%d", n);
}


// Muestra el mensaje inicial de bienvenida
void print_banner() {
  printf(COLOR_CYAN "BIENVENIDO EAFITOS\n" COLOR_RESET);
  printf("Escribe 'ayuda' para ver comandos\n\n");
}


int main(void) {

  char buf[MAXLINE];
  char *argv[MAXARGS];

  print_banner();

  // Bucle infinito que implementa el REPL
  // Read → Eval → Print → Loop
  while (1) {

    printf(COLOR_GREEN "Proyecto1 ❯ " COLOR_RESET);

    if (readline(buf, MAXLINE) <= 0)
      continue;

    if (buf[0] == 0)
      continue;

    add_history(buf);

    if (strcmp(buf, "salir") == 0) {
      printf(COLOR_WHITE "\nCerrando eafitos...\n" COLOR_RESET);
      exit(0);
    }

    if (strcmp(buf, "limpiar") == 0) {
      clear_screen();
      continue;
    }

    if (strcmp(buf, "historial") == 0) {
      show_history();
      continue;
    }

    if (strcmp(buf, "ayuda") == 0) {
      printf(COLOR_CYAN "\nComandos disponibles\n" COLOR_RESET);
      printf("listar\n");
      printf("leer <archivo>\n");
      printf("calc n1 op n2\n");
      printf("tiempo\n");
      printf("historial\n");
      printf("limpiar\n");
      printf("salir\n\n");
      continue;
    }

    // Calculadora
    if (starts_with(buf, "calc ")) {
      char *args[4];
      parse(buf + 5, args);

      if (args[0] && args[1] && args[2]) {
        int a = atoi(args[0]);
        int b = atoi(args[2]);
        char op = args[1][0];

        printf(COLOR_YELLOW "Resultado: " COLOR_RESET);

        if (op == '+') printf("%d\n", a + b);
        else if (op == '-') printf("%d\n", a - b);
        else if (op == '*') printf("%d\n", a * b);
        else if (op == '/' && b != 0) printf("%d\n", a / b);
        else printf(COLOR_RED "Operacion invalida\n" COLOR_RESET);
      } else {
        printf(COLOR_RED "Uso: calc n1 op n2\n" COLOR_RESET);
      }

      continue;
    }

    // Hora simulada basada en uptime
    if (strcmp(buf, "tiempo") == 0) {
      int ticks = uptime();
      int segundos_sistema = ticks / TICKS_PER_SECOND;

      int tiempo_total = START_TIME + segundos_sistema;
      tiempo_total += TIMEZONE_OFFSET;

      int horas = (tiempo_total / 3600) % 24;
      int minutos = (tiempo_total / 60) % 60;
      int seg = tiempo_total % 60;

      if (horas < 0)
        horas += 24;

      printf(COLOR_YELLOW "Hora actual: " COLOR_RESET);

      print_two_digits(horas);
      printf(":");
      print_two_digits(minutos);
      printf(":");
      print_two_digits(seg);
      printf("\n");

      continue;
    }

    if (strcmp(buf, "listar") == 0)
      strcpy(buf, "ls");

    if (starts_with(buf, "leer ")) {
      char temp[MAXLINE];
      strcpy(temp, "cat ");
      strcpy(temp + 4, buf + 5);
      strcpy(buf, temp);
    }

    char *pipepos = 0;

    for (int i = 0; buf[i]; i++) {
      if (buf[i] == '|') {
        pipepos = &buf[i];
        break;
      }
    }

    if (pipepos) {
      *pipepos = 0;
      char *right = pipepos + 1;

      while (*right == ' ')
        right++;

      char *left_args[MAXARGS];
      char *right_args[MAXARGS];

      parse(buf, left_args);
      parse(right, right_args);

      run_pipe(left_args, right_args);
    } else {
      parse(buf, argv);
      run_cmd(argv);
    }
  }

  return 0;
}