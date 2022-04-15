#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char *argv[]){
    if(argc < 2){
        perror("Wrong number arguments");
        exit(1);
    }
    char *exec_folder = argv[1];
    puts(exec_folder);
    int pd[2];
    if(pipe(pd) < 0){
        perror("Pipes"); 
        exit(1);
    }

    char buffer[1024];
    int n_bytes = 0;
    while((n_bytes = read(0, buffer, sizeof(buffer) - 1)) > 0){
        char arg[1024];
        buffer[n_bytes + 1] = 0;

    }

}
