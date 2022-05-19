#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>


struct config{
    char *idTransf[7];
    int limit[7];
    int current[7];
};

struct fila{
    char **pedidos;
    int n_pedidos;
    struct fila *next;
};

struct pedido{
  char *idTransf[7];
  int current[7];
  char **task;
  int size_task;
};

struct fila_execucao{
  pid_t pid;
  struct pedido *task;
  struct fila_execucao * next;
};

struct fila *execute(struct fila *queue, struct fila_execucao **ptr);
char *directory = NULL;

// struct fila *queue = NULL;
// struct fila_execucao *fila = NULL;

int exit_program = 0;

void sigusr1_handler(int signum){
    exit_program = 1;
}

struct config configuracao;

void push(struct fila **fila, char **pedido, int num_transf){
    struct fila *new = (struct fila *)malloc(sizeof(struct fila)); 
    while(*fila){
      fila = &((*fila)->next);
    }
    new->next = NULL;
    new->pedidos = pedido;
    new->n_pedidos = num_transf;
    *fila = new;
}

char **process_request(char *request, int *size){
    char *pedidos[100];

    int i = 0;
    for(char *token = strtok(request, " "); token != NULL; token = strtok(NULL, " ")){
        pedidos[i++] = strdup(token);
    }

    *size = i;

    char **requests = (char **)malloc(sizeof(char *) * i);
    for(int j = 0; j < i; ++j){
        requests[j] = pedidos[j];
    }

    return requests;
}


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


void push_execucao(struct fila_execucao **ptr, struct pedido *pedido, pid_t pid){

  while(*ptr){
    ptr = &((*ptr)->next);
  }

  *ptr = (struct fila_execucao *)malloc(sizeof(struct fila_execucao));
  (*ptr)->task = pedido;
  (*ptr)->pid = pid;
  (*ptr)->next = NULL;
}

struct fila *execute(struct fila *queue, struct fila_execucao **ptr){
    struct pedido *pedido = (struct pedido *)malloc(sizeof(struct pedido));
    for(int i = 0; i < 7; ++i){
        pedido->idTransf[i] = strdup(configuracao.idTransf[i]);
    }
    int pode = 1;
    for(int i = 0; i < 7 && pode; ++i){
        pedido->current[i] = 0;
        for(int j = 2; j < queue->n_pedidos && pode; ++j){
            if(strcmp(queue->pedidos[j], pedido->idTransf[i]) == 0){
                ++pedido->current[i];
                if(pedido->current[i] + configuracao.current[i] > configuracao.limit[i]) pode = 0;
            }
        }
    }
    if(pode){

        for(int i = 0; i < 7; ++i){
            configuracao.current[i] += pedido->current[i];
        }

        struct fila *aux = queue;
        queue = queue->next;
        pid_t pid = -1;
        if(!(pid = fork())){
            pid_t ultimo = -1;
            int pd[aux->n_pedidos][2];
            int current_pipe = 0;

            int client_pipe = open(aux->pedidos[aux->n_pedidos - 1], O_WRONLY);
            if(client_pipe < 0){
                perror("Error opening client_pipe");
                _exit(1);
            }
            write(client_pipe, "executing\n", sizeof("executing\n"));

            if(aux->n_pedidos == 4){

                if(!(ultimo = fork())){
                    int fd1 = open(aux->pedidos[0], O_RDONLY);
                    dup2(fd1, 0);
                    close(fd1);
                    int fd2 = open(aux->pedidos[1], O_WRONLY | O_CREAT | O_TRUNC, 0664);
                    dup2(fd2, 1);
                    close(fd2);
                    char transform[100];
                    snprintf(transform, sizeof(transform), "%s/%s", directory,
                             aux->pedidos[2]);
                    execlp(transform, transform, NULL);
                    perror("Error executing command");
                    exit(1);
                }
            }
            else{
              // pipeline de execucao
              for (int i = 2; i < aux->n_pedidos - 1; ++i) {
                if (i == 2) {
                  if (pipe(pd[current_pipe]) < 0) {
                    perror("Error creating pipe an");
                    _exit(1);
                  }
                  if (!fork()) {
                    close(pd[current_pipe][0]);
                    int file = open(aux->pedidos[0], O_RDONLY);
                    if (file < 0) {
                      perror("Error opening input file");
                      _exit(1);
                    }
                    dup2(file, 0);
                    close(file);
                    dup2(pd[current_pipe][1], 1);
                    close(pd[current_pipe][1]);
                    char transform[100];
                    snprintf(transform, sizeof(transform), "%s/%s", directory,
                             aux->pedidos[i]);
                    execlp(transform, transform, NULL);
                    perror("Error executing request");
                    _exit(1);
                  } else {
                    close(pd[current_pipe][1]);
                    ++current_pipe;
                  }
                } else if (i == aux->n_pedidos - 2) {
                  if (!(ultimo = fork())) {
                    int file = open(aux->pedidos[1],
                                    O_WRONLY | O_CREAT | O_TRUNC, 0664);
                    if (file < 0) {
                      perror("Error opening input file");
                      _exit(1);
                    }
                    dup2(file, 1);
                    close(file);
                    dup2(pd[current_pipe - 1][0], 0);
                    close(pd[current_pipe - 1][0]);
                    char transform[100];
                    snprintf(transform, sizeof(transform), "%s/%s", directory,
                             aux->pedidos[i]);
                    execlp(transform, transform, NULL);
                    perror("Error executing request");
                    _exit(1);
                  } else {
                    close(pd[current_pipe - 1][0]);
                  }
                } else {
                  if (pipe(pd[current_pipe]) < 0) {
                    perror("Error creating pipe an");
                    _exit(1);
                  }
                  if (!fork()) {
                    close(pd[current_pipe][0]);
                    dup2(pd[current_pipe - 1][0], 0);
                    close(pd[current_pipe - 1][0]);
                    dup2(pd[current_pipe][1], 1);
                    close(pd[current_pipe][1]);
                    char transform[100];
                    snprintf(transform, sizeof(transform), "%s/%s", directory,
                             aux->pedidos[i]);
                    execlp(transform, transform, NULL);
                    perror("Error executing request");
                    _exit(1);
                  } else {
                    close(pd[current_pipe][1]);
                    close(pd[current_pipe - 1][0]);
                    ++current_pipe;
                  }
                }
              }
            }
            waitpid(ultimo, NULL, 0);
            int n_bytes = 0;
            int fd = open("./tmp/server_monitor", O_WRONLY);
            char message[100];
            n_bytes = snprintf(message, sizeof(message), "terminou %d", getpid());
            write(fd, message, n_bytes);

            int input = open(aux->pedidos[0], O_RDONLY);
            int bytes_input = lseek(input, 0, SEEK_END);
            close(input);
            int output = open(aux->pedidos[1], O_RDONLY);
            int bytes_output = lseek(output, 0, SEEK_END);
            close(output);

            char answer[100];
            n_bytes = snprintf(answer, sizeof(answer), "concluded (bytes-input: %d, bytes-output: %d\n", bytes_input, bytes_output);

            write(client_pipe, answer, n_bytes);
            close(client_pipe);

            int id_cliente; 
            sscanf(aux->pedidos[aux->n_pedidos - 1], "./tmp/server_cliente%d", &id_cliente);
            kill(id_cliente, SIGUSR1);

            _exit(0);
        }

        pedido->task = (char **)malloc(sizeof(char *) * aux->n_pedidos);
        for(int i = 0; i < aux->n_pedidos; ++i){
          pedido->task[i] = strdup(aux->pedidos[i]);
        }
        pedido->size_task = aux->n_pedidos;

        for(int i = 0; i < aux->n_pedidos; ++i){
            free(aux->pedidos[i]);
        }
        free(aux->pedidos);
        free(aux);
        push_execucao(ptr,pedido, pid);



    }
    else{
        for(int i = 0; i < 7; ++i){
            free(pedido->idTransf[i]);
        }
        free(pedido);
    }
    return queue;
}

int main(int argc, char *argv[]){
    if(argc < 3){
        perror("Wrong arguments");
        exit(1);
    }

    signal(SIGUSR1, sigusr1_handler);
    
    exit_program = 0;
    directory = strdup(argv[2]);
    
    int config = open(argv[1], O_RDONLY);
    if (config < 0) {
        perror("File doesn't exist");
        return EXIT_FAILURE;
    }

    /* struct config configuracao; */

    int i = 0;
    int n_bytes = 0;
    char line[1024];
    while ((n_bytes = readln(config, line, sizeof(line))) > 0) {
        line[n_bytes - 1] = 0;
        char *transform = strtok(line, " ");
        configuracao.idTransf[i] = strdup(transform);
        int limit = atoi(strtok(NULL, " "));
        configuracao.limit[i] = limit;
        configuracao.current[i] = 0;
        i++;
    }
    close(config);

    struct fila *queue = NULL;
    struct fila_execucao *queue_exec = NULL;

    int server_monitor = open("./tmp/server_monitor", O_RDONLY);
    while (1) {
        n_bytes = read(server_monitor, line, sizeof(line));
        if (n_bytes > 0) {
            line[n_bytes] = 0;
            
            if(exit_program){
                if (queue == NULL) {
                  while(wait(NULL) != -1);
                  break;
                }
            }
            int n_transf = 0;
            char **request = process_request(line, &n_transf);
            if (strcmp(request[0], "status") == 0) {
                char answer[4096];
                int n_bytes_answer = 0;

                struct fila_execucao **ptr = &queue_exec;
                int j = 1;
                while(*ptr){
                    n_bytes_answer +=
                        snprintf(answer + n_bytes_answer,
                                 sizeof(answer) - n_bytes_answer, "task #%d: ", j);
                    for (int i = 0; i < (*ptr)->task->size_task - 1; ++i) {
                      if(i < (*ptr)->task->size_task - 2){
                        n_bytes_answer += snprintf(
                            answer + n_bytes_answer,
                            sizeof(answer) - n_bytes_answer, "%s ", (*ptr)->task->task[i]);
                      } else {
                          n_bytes_answer += snprintf(
                              answer + n_bytes_answer,
                              sizeof(answer) - n_bytes_answer, "%s\n", (*ptr)->task->task[i]);
                      }
                    }
                    ++j;
                    ptr = &((*ptr)->next);
                }
                for (int i = 0; i < 7; ++i) {
                  n_bytes_answer += snprintf(
                      answer + n_bytes_answer, sizeof(answer) - n_bytes_answer,
                      "%s : %d/%d (running/max)\n", configuracao.idTransf[i],
                      configuracao.current[i], configuracao.limit[i]);
                }
                int fd = open(request[1], O_WRONLY);
                if (fd < 0) {
                    perror("Error opening pipe");
                    exit(1);
                }
                write(fd, answer, n_bytes_answer);
                close(fd);
                for(int i = 0; i < n_transf; ++i){
                    free(request[i]);
                }
                free(request);
            } 
            else if(strcmp(request[0], "terminou") == 0){
              pid_t pid = atoi(request[1]);
              waitpid(pid, NULL, 0);
              struct fila_execucao **ptr = &queue_exec;
              while(*ptr && (*ptr)->pid != pid){
                ptr = &((*ptr)->next);
              }
              if(*ptr){
                for(int i = 0; i < 7; ++i){
                    configuracao.current[i] -= (*ptr)->task->current[i];
                    free((*ptr)->task->idTransf[i]);
                }
                for(int i = 0; i < (*ptr)->task->size_task; ++i){
                  free((*ptr)->task->task[i]);
                }
                free((*ptr)->task->task);
                free((*ptr)->task);
                struct fila_execucao *aux = *ptr;
                *ptr = (*ptr)->next;
                free(aux);
              }
             
            }
            else if(!exit_program){
                push(&queue, request, n_transf);
                int fd = open(request[n_transf - 1], O_WRONLY);
                if (fd < 0) {
                    perror("Error opening pipe");
                    exit(1);
                }
                write(fd, "pending\n", sizeof("pending\n"));
                close(fd);
                queue = execute(queue, &queue_exec);
            }
        } else {
            if (queue!= NULL) queue = execute(queue, &queue_exec);
        }
    }

    close(server_monitor);

  return 0;
}
