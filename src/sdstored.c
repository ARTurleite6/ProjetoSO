#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>

int id_monitor;

void sigterm_handler(int sig){

    kill(id_monitor, SIGUSR1); 
    
    waitpid(id_monitor, NULL, 0);

    kill(getpid(), SIGKILL);
}

/* struct config{ */
/*     char *idTransf[7]; */
/*     int limit[7]; */
/*     int current[7]; */
/* }; */
/*  */
/* struct fila{ */
/*     char **pedidos; */
/*     int n_pedidos; */
/*     int priority; */
/*     struct fila *next; */
/* }; */
/*  */
char buffer[1024];
int inicio = 0;
int fim = 0;

int readc(int fd, char *line){
    if(fim == 0 || inicio == fim){
        fim = read(fd, buffer, sizeof(buffer));
        if(fim <= 0) return 0;
        inicio = 0;
    }
    *line = buffer[inicio++];
    return 1;
}

int readln(int fd, char *line, int size){
    int i = 0;
    while(i < size && readc(fd, &line[i]) == 1){
        if(line[i++] == '\n') break;
    }
    return i;
}

char **process_request(char *request, int *size){
    char *pedidos[100];

    int i = 0;
    for(char *token = strtok(request, " "); token != NULL; token = strtok(NULL, " ")){
        pedidos[i++] = strdup(token);
    }

    *size = i;

    char **requests = (char **)malloc(sizeof(char *) * i);
    for(int j = 0; j < i; j++){
        requests[j] = pedidos[j];
    }
    return requests;
}

int main(int argc, char *argv[]){

    if(argc < 3){
        perror("Wrong number of arguments");
        exit(1);
    }

    int fd = open(argv[1], O_RDONLY);
    if(fd < 0){
        perror("File doesn't exist");
        return EXIT_FAILURE;
    }

    signal(SIGTERM, sigterm_handler);

    mkfifo("./tmp/server_monitor", 0664);
    if(!(id_monitor = fork())){
        execl("./bin/monitor", "./bin/monitor", argv[1], argv[2], NULL);
        perror("Error executing");
        _exit(1);
    }
    mkfifo("./tmp/client_server", 0664);     

    int read_pipe = open("./tmp/client_server", O_RDONLY);
    int close_pipe = open("./tmp/client_server", O_WRONLY);

    char line[1024];
    int n_bytes = 0;
    int monitor_pipe;
    while((n_bytes = read(read_pipe, line, sizeof(line) - 1)) > 0){
      line[n_bytes] = 0;
      char *linecpy = strdup(line);
      char *to_free = linecpy;
      int n_transf = 0;
      char **pedido = process_request(linecpy, &n_transf);

      if(strcmp(pedido[0], "exit") == 0){
        close(close_pipe);
      }
      else if(!strcmp(pedido[0], "proc-file")){
          monitor_pipe = open("./tmp/server_monitor", O_WRONLY);
          write(monitor_pipe, line + 10, n_bytes - 10);
          close(monitor_pipe);
      }

      else if(!strcmp(pedido[0], "status")){
          monitor_pipe = open("./tmp/server_monitor", O_WRONLY);
          write(monitor_pipe, line, n_bytes);
          close(monitor_pipe);
      }

      for (int i = 0; i < n_transf; ++i) {
          free(pedido[i]);
        }
        free(pedido);
        free(to_free);
    }

    close(read_pipe);

    unlink("./tmp/client_server");
    unlink("./tmp/server_monitor");

}
