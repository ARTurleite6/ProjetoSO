#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

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

struct fila *push(struct fila *q, char **pedido, int num_transf){
    struct fila *new = (struct fila *)malloc(sizeof(struct fila)); 
    new->next = q;
    new->pedidos = pedido;
    new->n_pedidos = num_transf;
    return new;
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

struct fila *execute(struct fila *queue, struct config *config){
    struct config pedido;
    memcpy(&pedido, config, sizeof(struct config));
    int pode = 1;
    for(int i = 0; i < 7 && pode; ++i){
        pedido.current[i] = 0;
        for(int j = 2; j < queue->n_pedidos && pode; ++j){
            if(strcmp(queue->pedidos[j], pedido.idTransf[i]) == 0){
                ++pedido.current[i];
                if(pedido.current[i] > config->limit[i]) pode = 0;
            }
        }
    }

    if(!pode) return queue;
    
    for(int i = 0; i < 7; ++i){
        printf("%s -> %d\n", pedido.idTransf[i], pedido.current[i]);
    }
    return queue;
}

int main(int argc, char *argv[]){

    if(argc < 3){
        perror("Wrong number of arguments");
        exit(1);
    }

    int fd = open(argv[1], O_RDONLY);
    if(fd < 0){
        perror("File doesn't exist");
        return EXIT_FAILURE;
    }

    /* struct config configuracao; */
    /*  */
    /* struct fila *queue = NULL; */
    /*  */
    /* int i = 0; */
    /* int n_bytes = 0; */
    /* char line[1024]; */
    /* while((n_bytes = readln(fd, line, sizeof(line))) > 0){ */
    /*     line[n_bytes - 1] = 0; */
    /*     char *transform = strtok(line, " "); */
    /*     configuracao.idTransf[i] = strdup(transform); */
    /*     int limit = atoi(strtok(NULL, " ")); */
    /*     configuracao.limit[i] = limit; */
    /*     configuracao.current[i] = 0; */
    /*     i++; */
    /* } */
    /* close(fd); */
    /*  */
    if(!fork()){
        execl("./monitor", "./monitor", argv[1], argv[2], NULL);
        perror("Error executing");
        _exit(1);
    }
    if(!fork()){
        _exit(1);
    }
    mkfifo("client_server", 0664);     

    int read_pipe = open("client_server", O_RDONLY);
    int close_pipe = open("client_server", O_WRONLY);
    int monitor_pipe = open("server_monitor", O_WRONLY);

    char line[1024];
    int n_bytes = 0;
    while((n_bytes = read(read_pipe, line, sizeof(line))) > 0){

        write(monitor_pipe, line, n_bytes);
        line[n_bytes] = 0;
        int n_transf = 0;
        char **pedido = process_request(line, &n_transf);
        /* if(strcmp(pedido[0], "status") == 0){ */
        /*     if(!fork()){ */
        /*         int fd = open(pedido[1], O_WRONLY); */
        /*         if(fd < 0){ */
        /*             perror("Error opening pipe"); */
        /*             exit(1); */
        /*         } */
        /*         close(fd); */
        /*         _exit(0); */
        /*     } */
        /* } */
        if(strcmp(pedido[0], "exit") == 0){
            close(close_pipe);
        }
        else{
            /* write(monitor_pipe, line, n_bytes); */
            /* queue = push(queue, pedido, n_transf);  */
            /* int fd = open(pedido[n_transf - 1], O_WRONLY); */
            /* if(fd < 0){ */
            /*     perror("Error opening pipe");  */
            /*     exit(1); */
            /* } */
            /* write(fd, "pending\n", sizeof("pending\n")); */
            /* close(fd); */
            /* queue = execute(queue, &configuracao); */
        }
    }


    close(read_pipe);
    close(monitor_pipe);

    unlink("client_server");

}
