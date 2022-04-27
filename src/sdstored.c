#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define PENDING 0
#define EXECUTING 1
#define TERMINATED 2


struct transf{
    char *idTransf;    
    int limit;
    int current;
};

char buffer[1024];
int inicio = 0;
int fim = 0;
struct transf config[7];

pid_t pids[1024];
struct transf pid_transformation[1024][7];
int actual_pid = 0;

int find_transformation(char *idTransf);

int find_pid(pid_t pid){
    for(int i = 0; i < actual_pid; ++i){
        if(pids[i] == pid) return i;
    }
    return -1;
}

void sigchld_handler(int sig){
    int status = 0;
    pid_t pid = 0;
    while((pid = waitpid(-1, &status, 0)) > 0){
        int index = find_pid(pid);
        if(index != -1){
            pids[index] = -1;
            for(int i = 0; i < 7; ++i){
                config[i].current -= pid_transformation[index][i].current;
                pid_transformation[index][i].current = 0;
            }
        }
    }
}

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

int find_transformation(char *idTransf){
    for(int i = 0; i < 7; i++){
        if(strcmp(config[i].idTransf, idTransf) == 0) return i;
    }
    return -1;
}

int main(int argc, char *argv[]){

    int read_pipe = 0;
    char *directory = strdup(argv[2]);
    signal(SIGCHLD, sigchld_handler);

    if(argc < 3){
        perror("Missing arguments");
        return EXIT_FAILURE;
    }

    int fd = open(argv[1], O_RDONLY);
    if(fd < 0){
        perror("File doesn't exist");
        return EXIT_FAILURE;
    }
    int n_bytes = 0;
    char line[1024];

    int i = 0;
    while((n_bytes = readln(fd, line, sizeof(line))) > 0){
        line[n_bytes - 1] = 0;
        char *temp = strdup(line);
        char *trans = strdup(strtok(temp, " "));
        int limit = atoi(strtok(NULL, " "));
        
        config[i].idTransf = trans;
        config[i].current = 0;
        config[i++].limit = limit;
    }
    for(int i = 0; i < 7; ++i){
    }
    close(fd);

    mkfifo("./tmp/clientServer", 0664);

    while(1){
        char message[1024];
        int n = 0;
        read_pipe = open("./tmp/clientServer", O_RDONLY);
        n = read(read_pipe, message, sizeof(message));
        close(read_pipe);
        if(n > 0){
            message[n] = '\0';
            char *args[20];
            int n_transf = 0;
            struct transf pedido[7];
            for(int i = 0; i < 7; ++i){
                pedido[i].idTransf = strdup(config[i].idTransf); 
                pedido[i].current = 0;
            }
            for(char *token = strtok(message, " "); token != NULL; token = strtok(NULL, " ")){
                args[n_transf] = strdup(token);
                int i = find_transformation(token);
                if(i != -1){
                    ++config[i].current;
                    ++pedido[i].current;
                }
                ++n_transf;
            }


            int pid = fork();
            if(pid == 0){
                char pipe_write[100];
                snprintf(pipe_write, 100, "./tmp/serverClient%s", args[n_transf - 1]);

                int write_pipe = open(pipe_write, O_WRONLY);
                pid_t ultimo = -1;
                //cliente passa status
                if(n_transf == 2){
                    if(!strcmp(args[0], "status")){
                        for(int i = 0; i < 7; ++i){
                            char status[100];
                            int status_size = snprintf(status, sizeof(status), "transf %s: %d/%d (running/max)\n", config[i].idTransf, config[i].current, config->limit);
                            write(write_pipe, status, status_size);
                        }
                    }
                }
                // uma transformacao
                else if(n_transf == 4){ // so tem uma transformacao
                    write(write_pipe, "pending\n", sizeof("pending\n"));
                    write(write_pipe, "executing\n", sizeof("executing\n"));
                    if(!(ultimo = fork())){
                        int fd_in = open(args[0], O_RDONLY);
                        int fd_out = open(args[1], O_WRONLY | O_CREAT | O_TRUNC, 0664);
                        dup2(fd_in, 0);
                        close(fd_in);
                        dup2(fd_out, 1);
                        close(fd_out);
                        char transformacao[200];
                        strcpy(transformacao, directory);
                        strcat(transformacao, "/");
                        strcat(transformacao, args[2]);
                        execlp(transformacao, transformacao, NULL);
                        perror("Error executing");
                        _exit(1);
                    }
                }
                else{
                    write(write_pipe, "pending\n", sizeof("pending\n"));
                    write(write_pipe, "executing\n", sizeof("executing\n"));
                    int pipes[n_transf - 2][2];

                    int current_pipe = 0;
                    for(int i = 2; i < n_transf - 1; ++i){
                        char transformacao[200];
                        strcpy(transformacao, directory);
                        strcat(transformacao, "/");
                        strcat(transformacao, args[i]);
                        if(i == 2){
                            pipe(pipes[current_pipe]); 
                            if(!fork()){
                                int fd = open(args[0], O_RDONLY);
                                dup2(fd, 0);
                                close(fd);
                                dup2(pipes[current_pipe][1], 1);
                                close(pipes[current_pipe][1]);
                                execlp(transformacao, transformacao, NULL);
                                perror("Error executing");
                                _exit(1);
                            }
                            else{
                                close(pipes[current_pipe][1]);
                                ++current_pipe;
                            }
                        }
                        else if(i == n_transf - 2){
                            if(!(ultimo = fork())){
                                int fd = open(args[1], O_WRONLY | O_TRUNC | O_CREAT, 0664);
                                dup2(fd, 1);
                                close(fd);
                                dup2(pipes[current_pipe - 1][0], 0);
                                close(pipes[current_pipe - 1][0]);
                                execlp(transformacao, transformacao, NULL);
                                perror("Error executing");
                                _exit(1);
                            }
                            else{
                                close(pipes[current_pipe - 1][0]);
                            }
                        }
                        else{
                            pipe(pipes[current_pipe]);
                            if(!fork()){
                                dup2(pipes[current_pipe - 1][0], 0);
                                close(pipes[current_pipe - 1][0]);
                                dup2(pipes[current_pipe][1], 1);
                                close(pipes[current_pipe][1]);
                                execlp(transformacao, transformacao, NULL);
                                perror("Error executing");
                                _exit(1);
                            }
                            else{
                                close(pipes[current_pipe - 1][0]);
                                close(pipes[current_pipe][1]);
                                ++current_pipe;
                            }
                        }
                    }
                }
                if(ultimo != -1){
                    waitpid(ultimo, NULL, 0);
                    write(write_pipe, "concluded\n", sizeof("concluded\n"));
                }
                close(write_pipe);
                _exit(0);
            }
            else{
                pids[actual_pid] = pid;
                memcpy(pid_transformation[actual_pid++], pedido, sizeof(struct transf) * 7);
                actual_pid %= 1024; 
            }
        }
        close(read_pipe);
    }

    unlink("clientServer");

    return 0;
}
