#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>


int main(int argc, char *argv[]){

    pid_t pid = getpid();
    char line[1024];
    int n_bytes = 0;
    for(int i = 1; i < argc; ++i){
        n_bytes += snprintf(line + n_bytes, 1024, "%s ", argv[i]);
    }
    n_bytes += snprintf(line + n_bytes, 1024, "%d ", pid);

    char pipe_name[100];
    snprintf(pipe_name, sizeof(pipe_name), "./tmp/serverClient%d", pid);

    mkfifo(pipe_name, 0664);
    

    int write_pipe = open("./tmp/clientServer", O_WRONLY);
    write(write_pipe, line, strlen(line));    
    close(write_pipe);

    int read_pipe = open(pipe_name, O_RDONLY);
    n_bytes = 0;
    while((n_bytes = read(read_pipe, line, sizeof(line))) > 0){
        write(1, line, n_bytes);    
    }
    close(read_pipe);

    unlink(pipe_name);

    return 0;
}
