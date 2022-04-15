#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>


int main(int argc, char *argv[]){
    int fd = open("log", O_WRONLY);
    char line[1024];
    strcpy(line, argv[1]);
    for(int i = 2; i < argc; ++i){
        strcat(line, " ");
        strcat(line, argv[i]);
        
    }
    write(fd, line, strlen(line));    
    close(fd);
    return 0;
}
