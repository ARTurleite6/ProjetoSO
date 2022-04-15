#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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


struct transf{
    char *idTransf;    
    int limit;
    int current;
};


int main(int argc, char *argv[]){
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


    int log = mkfifo("log", 0664);

    fd = open("log", O_RDONLY);

    while(1){
        char message[1024];
        int n = 0;
        n = read(fd, message, sizeof(message));
        if(n != 0){
            printf("li alguma coisa\n");
            message[n] = 0;
            char *args[20];
            char *token = NULL;
            int j = 0;
            for(char *token = strtok(message, " "); token != NULL; token = strtok(NULL, " ")){
                args[j] = strdup(token);
                ++j;
            }
            args[j] = NULL;
            if(!fork()){
                int fd1 = open(args[0], O_RDONLY);
                dup2(fd1, 0);
                close(fd1);
                int fd2 = open(args[1], O_WRONLY | O_CREAT | O_TRUNC, 0664);
                dup2(fd2, 1);
                close(fd2);

                execvp(args[2], args + 2);
            }
        }
    }

    return 0;
}
