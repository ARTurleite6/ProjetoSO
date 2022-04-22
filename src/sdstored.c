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

char buffer[1024];
int inicio = 0;
int fim = 0;

int read_pipe = 0;
int write_pipe = 0;

int process_status[1024];
pid_t process[1024];
int n_process = 0;

int find_pid(pid_t pid){
    int index = -1;
    for(int i = 0; i < n_process; ++i)
        if(pid == process[i]) index = i;
    return index;
}

//delete element from array
void delete_process(int index){
    for(int i = index; i < n_process - 1; ++i)
        process[i] = process[i + 1];
    n_process--;
}


void sigchld_handler(int sig){
    int status = 0;
    pid_t pid = 0;
    while((pid = waitpid(-1, &status, 0)) > 0){
        int i = find_pid(pid);
        if(i != -1){
            process_status[i] = TERMINATED;
            delete_process(i);
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

struct transf{
    char *idTransf;    
    int limit;
    int current;
};

int main(int argc, char *argv[]){

    signal(SIGCHLD, sigchld_handler);

    if(argc < 3){
        perror("Missing arguments");
        return EXIT_FAILURE;
    }

    struct transf config[7];
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
        char *trans = strdup(strtok(line, " "));
        int limit = atoi(strtok(NULL, " "));
        config[i].idTransf = trans;
        config[i].current = 0;
        config[i++].limit = limit;
    }
    close(fd);


    mkfifo("clientServer", 0664);

    read_pipe = open("clientServer", O_RDONLY);


    while(1){
        char message[1024];
        int n = 0;
        n = read(read_pipe, message, sizeof(message));
        if(n != 0){
            write_pipe = open("serverClient", O_WRONLY);
            if(write_pipe < 0){
                perror("Error opening pipe");
                return EXIT_FAILURE;
            }
            if(write(write_pipe, "pending\n", sizeof("pending\n")) <= 0){
                perror("Error writing to pipe");
                return EXIT_FAILURE;
            }
            message[n] = 0;
            char *args[20];
            int n_transf = 0;
            for(char *token = strtok(message, " "); token != NULL; token = strtok(NULL, " ")){
                args[n_transf] = strdup(token);
                ++n_transf;
            }

            int pid = fork();
            if(pid == 0){
                if(n_transf == 3){ // so tem uma transformacao
                    if(!fork()){
                        int fd_in = open(args[0], O_RDONLY);
                        int fd_out = open(args[1], O_WRONLY | O_CREAT | O_TRUNC, 0664);
                        dup2(fd_in, 0);
                        close(fd_in);
                        dup2(fd_out, 1);
                        close(fd_out);
                        execlp(args[2], args[2], NULL);
                        perror("Error executing");
                        _exit(1);
                    }
                }
                else{
                    int pipes[n_transf - 1][2];

                    int current_pipe = 0;
                    for(int i = 2; i < n_transf; ++i){
                        if(i == 2){
                            pipe(pipes[current_pipe]); 
                            if(!fork()){
                                int fd = open(args[0], O_RDONLY);
                                dup2(fd, 0);
                                close(fd);
                                dup2(pipes[current_pipe][1], 1);
                                close(pipes[current_pipe][1]);
                                execlp(args[i], args[i], NULL);
                                perror("Error executing");
                                _exit(1);
                            }
                            else{
                                close(pipes[current_pipe][1]);
                                ++current_pipe;
                            }
                        }
                        else if(i == n_transf - 1){
                            if(!fork()){
                                int fd = open(args[1], O_WRONLY | O_TRUNC | O_CREAT, 0664);
                                dup2(fd, 1);
                                close(fd);
                                dup2(pipes[current_pipe - 1][0], 0);
                                close(pipes[current_pipe - 1][0]);
                                execlp(args[i], args[i], NULL);
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
                                execlp(args[i], args[i], NULL);
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
                _exit(0);
            }
            else{
                if(n_process == 1024){
                    n_process = 0;
                }
                process[n_process] = pid;
                process_status[n_process++] = EXECUTING;
                write(write_pipe, "processing\n", sizeof("processing\n"));
                close(write_pipe);
            }

        }
    }

    close(read_pipe);
    unlink("clientServer");

    return 0;
}
