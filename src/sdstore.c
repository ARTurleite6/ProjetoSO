#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>

int close_pipe;
void sigusr1_handler(int signum){
    close(close_pipe);
}

int main(int argc, char *argv[]){
    int n_bytes = 0;
    char request[200];
    signal(SIGUSR1, sigusr1_handler);

    pid_t pid = getpid();

    for(int i = 1; i < argc; ++i){
        n_bytes += snprintf(request + n_bytes, sizeof(request) - n_bytes, "%s ", argv[i]);
    }

    int server_pipe = open("./tmp/client_server", O_WRONLY);
    if(server_pipe < 0){
        perror("Error opening server pipe");
        return 1;
    }

    char client_pipe_str[200];
    snprintf(client_pipe_str, sizeof(client_pipe_str), "./tmp/server_cliente%d", pid);
    mkfifo(client_pipe_str, 0664);
    n_bytes += snprintf(request + n_bytes, sizeof(request) - n_bytes, "%s", client_pipe_str);

    write(server_pipe, request, n_bytes);
    close(server_pipe);

    if(!strcmp(argv[1], "status")){
        int client_pipe = open(client_pipe_str, O_RDONLY);
        char buffer[1024];
        int n_bytes;
        n_bytes = read(client_pipe, buffer, sizeof(buffer));
        write(1, buffer, n_bytes);
        close(client_pipe);
    }
    else if(!strcmp(argv[1], "exit")){

    }
    else{

        int client_pipe = open(client_pipe_str, O_RDONLY);
        close_pipe = open(client_pipe_str, O_WRONLY);
        int answer_bytes = 0;
        char answer[1024];
        while((answer_bytes = read(client_pipe, answer, sizeof(answer))) > 0){
            write(1, answer, answer_bytes);
            answer[answer_bytes] = 0;
        }
        close(client_pipe);
    }
    unlink(client_pipe_str);
    return 0;
}
