#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>


int main(int argc, char *argv[]){
    int write_pipe = open("clientServer", O_WRONLY);
    mkfifo("serverClient", 0664);
    char line[1024];
    strcpy(line, argv[1]);
    for(int i = 2; i < argc; ++i){
        strcat(line, " ");
        strcat(line, argv[i]);
        
    }
    write(write_pipe, line, strlen(line));    
    close(write_pipe);

    int read_pipe = open("serverClient", O_RDONLY);
    int n_bytes = 0;
    while((n_bytes = read(read_pipe, line, sizeof(line))) > 0){
        write(1, line, n_bytes);    
    }

    close(read_pipe);
    unlink("serverClient");


    return 0;
}
