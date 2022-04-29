#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>

void execute();

struct config{
    char *idTransf[7];
    int limit[7];
    int current[7];
};

char *directory = NULL;

struct fila *queue = NULL;

pid_t pids[1024];
int last_process = 0;
struct config *pedidos[1024];

struct config configuracao;

int find_pid(pid_t pid){
    for(int i = 0; i < last_process; ++i){
        if(pid == pids[i]) return i;
    }
    return -1;
}

void sigchld_handler(){
    pid_t pid = -1;
    while((pid = waitpid(-1, NULL, WNOHANG)) != -1){
        int i = find_pid(pid);
        if(i != -1){
            for(int j = 0; j < 7; ++j){
                configuracao.current[j] -= pedidos[i]->current[j];
            }
            free(pedidos[i]);
            pedidos[i] = NULL;
        }
    }
}

struct fila{
    char **pedidos;
    int n_pedidos;
    struct fila *next;
};

void push(char **pedido, int num_transf){
    struct fila *new = (struct fila *)malloc(sizeof(struct fila)); 
    new->next = queue;
    new->pedidos = pedido;
    new->n_pedidos = num_transf;
    queue = new;
}

char **process_request(char *request, int *size){
    char **pedidos = (char**)malloc(sizeof(char *) * 40);     
    int max = 20;

    int i = 0;
    for(char *token = strtok(request, " "); token != NULL; token = strtok(NULL, " ")){
        pedidos[i++] = strdup(token);
    }

    *size = i;

    return pedidos;
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


void execute(){
    struct config *pedido = (struct config *)malloc(sizeof(struct config));
    memcpy(pedido, &configuracao, sizeof(struct config));
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

    puts("teste");
    if(pode){
        puts("pode executar");

        for(int i = 0; i < 7; ++i){
            configuracao.current[i] += pedido->current[i];
        }

        pid_t pid = -1;
        if(!(pid = fork())){
            pid_t ultimo = -1;
            int pd[queue->n_pedidos][2];
            int current_pipe = 0;

            int client_pipe = open(queue->pedidos[queue->n_pedidos - 1], O_WRONLY);
            if(client_pipe < 0){
                perror("Error opening client_pipe");
                _exit(1);
            }
            write(client_pipe, "executing\n", sizeof("executing\n"));
            close(client_pipe);

            //pipeline de execucao
            for(int i = 2; i < queue->n_pedidos - 1; ++i){
                if(i == 2){
                    if(pipe(pd[current_pipe]) < 0){
                        perror("Error creating pipe an");
                        _exit(1);
                    }
                    if(!fork()){
                        close(pd[current_pipe][0]);
                        int file = open(queue->pedidos[0], O_RDONLY);
                        if(file < 0){
                            perror("Error opening input file");
                            _exit(1);
                        }
                        dup2(file, 0);
                        close(file);
                        dup2(pd[current_pipe][1], 1);
                        close(pd[current_pipe][1]);
                        char transform[100];
                        snprintf(transform, sizeof(transform), "%s/%s", directory, queue->pedidos[i]);
                        execlp(transform, transform, NULL);
                        perror("Error executing request");
                        _exit(1);
                    }
                    else{
                        close(pd[current_pipe][1]);
                        ++current_pipe;
                    }
                }
                else if(i != queue->n_pedidos - 2){
                    if(pipe(pd[current_pipe]) < 0){
                        perror("Error creating pipe an");
                        _exit(1);
                    }
                    if(!fork()){
                        close(pd[current_pipe][0]);
                        dup2(pd[current_pipe - 1][0], 0);
                        close(pd[current_pipe - 1][0]);
                        dup2(pd[current_pipe][1], 1);
                        close(pd[current_pipe][1]);
                        char transform[100];
                        snprintf(transform, sizeof(transform), "%s/%s", directory, queue->pedidos[i]);
                        execlp(transform, transform, NULL);
                        perror("Error executing request");
                        _exit(1);
                    }
                    else{
                        close(pd[current_pipe][1]);
                        close(pd[current_pipe - 1][0]);
                    }
                }
                if(i == queue->n_pedidos - 2){
                    if(!(ultimo = fork())){
                        int file = open(queue->pedidos[1], O_WRONLY | O_CREAT | O_TRUNC, 0664);
                        if(file < 0){
                            perror("Error opening input file");
                            _exit(1);
                        }
                        dup2(file, 1);
                        close(file);
                        dup2(pd[current_pipe - 1][0], 0);
                        close(pd[current_pipe - 1][0]);
                        char transform[100];
                        snprintf(transform, sizeof(transform), "%s/%s", directory, queue->pedidos[i]);
                        execlp(transform, transform, NULL);
                        perror("Error executing request");
                        _exit(1);
                    }
                    else{
                        close(pd[current_pipe - 1][0]);
                    }
                }
            }

            waitpid(ultimo, NULL, 0);
            client_pipe = open(queue->pedidos[queue->n_pedidos - 1], O_WRONLY);
            if(client_pipe < 0){
                perror("Error opening client_pipe");
                _exit(1);
            }
            write(client_pipe, "concluded\n", sizeof("concluded\n"));
            close(client_pipe);

            _exit(0);
        }

        pids[last_process] = pid;
        pedidos[last_process] = pedido;
        ++last_process;

        struct fila *aux = queue;
        queue = queue->next;
        free(aux);
    }
}

int main(int argc, char *argv[]){
    if(argc < 3){
        perror("Wrong arguments");
        exit(1);
    }
    
    directory = strdup(argv[2]);
    
    signal(SIGCHLD, sigchld_handler);

    int config = open(argv[1], O_RDONLY);
    if(config < 0){
        perror("File doesn't exist");
        return EXIT_FAILURE;
    }

    /* struct config configuracao; */

    int i = 0;
    int n_bytes = 0;
    char line[1024];
    while((n_bytes = readln(config, line, sizeof(line))) > 0){
        line[n_bytes - 1] = 0;
        char *transform = strtok(line, " ");
        configuracao.idTransf[i] = strdup(transform);
        int limit = atoi(strtok(NULL, " "));
        configuracao.limit[i] = limit;
        configuracao.current[i] = 0;
        i++;
    }
    close(config);

    int server_monitor = open("server_monitor", O_RDONLY);
    while((n_bytes = read(server_monitor, line, sizeof(line))) > 0){
        line[n_bytes] = 0;
        int n_transf = 0;
        char **request = process_request(line, &n_transf);
        if(strcmp(request[0], "status") == 0){
            if(!fork()){
                int fd = open(request[1], O_WRONLY);
                if(fd < 0){
                    perror("Error opening pipe");
                    exit(1);
                }
                char answer[1024];
                int n_bytes_answer = 0;
                for(int i = 0; i < 7; ++i){
                    n_bytes_answer += snprintf(answer + n_bytes_answer, sizeof(answer) - n_bytes_answer, "%s : %d/%d\n", configuracao.idTransf[i], configuracao.current[i], configuracao.limit[i]); 
                }
                write(fd, answer, n_bytes_answer);
                close(fd);
                _exit(0);
            }
        }
        else{
            push(request, n_transf); 
            int fd = open(request[n_transf - 1], O_WRONLY);
            if(fd < 0){
                perror("Error opening pipe"); 
                exit(1);
            }
            write(fd, "pending\n", sizeof("pending\n"));
            close(fd);
            execute();

        }
    }
    close(server_monitor);

}
