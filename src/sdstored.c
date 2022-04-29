#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define WAITING 0
#define EXECUTING 1
#define TERMINATED 2

typedef struct queue{
    char *request[20];
    int n_transforms;
    struct queue *next;
} *Queue;


//creates a new queue
Queue createQueue(){
    Queue q = (Queue)malloc(sizeof(struct queue));
    q->next = NULL;
    return q;
}

Queue push(Queue q, char *request[], int n_requests){
    Queue temp = createQueue();
    for(int i = 0; i < n_requests; ++i){
        q->request[q->n_transforms++] = strdup(request[i]);
    }
    temp->next = q;
    return temp;
}

void dequeue(Queue q){
    for(int i = 0; i < q->n_transforms; ++i){
        free(q->request[i]);
    }
    Queue aux = q;
    q = q->next;
    free(aux);
}

struct transf{
    char *idTransf;    
    int limit;
    int current;
};
struct transf config[7];

char buffer[1024];
int inicio = 0;
int fim = 0;

pid_t pids[1024];
char *pid_tasks[1024];
int pid_status[1024];
int pid_transformation[1024][7];
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
                config[i].current -= pid_transformation[index][i];
                pid_transformation[index][i] = 0;
            }
            pid_status[index] = TERMINATED;
            free(pid_tasks[index]);
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

int can_execute(Queue q){
     
}

int main(int argc, char *argv[]){

    char *directory = strdup(argv[2]);
    signal(SIGCHLD, sigchld_handler);
    mkfifo("server_monitor", 0666);

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
    int read_pipe = open("./tmp/clientServer", O_RDONLY);
    int close_controller = open("./tmp/clientServer", O_WRONLY);

    int n_bytes_read = 0;
    char message[1024];
    Queue q = createQueue();

    while((n_bytes_read = read(read_pipe, message, sizeof(message) - 1)) > 0){
        message[n_bytes_read - 1] = '\0';
        pid_tasks[actual_pid] = strdup(message);

        char *args[20];
        int n_transf = 0;
        int pedido[7];
        for(int i = 0; i < 7; ++i){
            pedido[i] = 0;
        }


        // atualiza contadores
        char *token = NULL;
        for(token = strtok(message, " "); token != NULL; token = strtok(NULL, " ")){
            args[n_transf] = token;
            int i = find_transformation(token);
            if(i != -1){
                ++config[i].current;
                ++pedido[i];
            }
            ++n_transf;
        }

        if(strcmp(args[0], "exit") == 0) {
            printf("Server closed\n");
            close(close_controller);
        }
        else{
            push(q, args, n_transf);

            /* if(can_execute(q)){ */
            /*  */
            /*     int pid = fork(); */
            /*     if(pid == 0){ */
            /*         char pipe_write[100]; */
            /*         snprintf(pipe_write, 100, "./tmp/serverClient%s", q->request[q->n_transforms - 1]); */
            /*  */
            /*         int write_pipe = open(pipe_write, O_WRONLY); */
            /*         pid_t ultimo = -1; */
            /*         //cliente passa status */
            /*         if(q->n_transforms == 2){ */
            /*             if(!strcmp(q->request[0], "status")){ */
            /*                 char status[100]; */
            /*                 for(int i = 0; i < 1024; ++i){ */
            /*                     if(pid_status[i] == EXECUTING){ */
            /*                         int status_size = snprintf(status, 100, "task %d : %s\n", i, pid_tasks[i]); */
            /*                         write(write_pipe, status, status_size); */
            /*                     } */
            /*                 } */
            /*                 for(int i = 0; i < 7; ++i){ */
            /*                     int status_size = snprintf(status, sizeof(status), "transf %s: %d/%d (running/max)\n", config[i].idTransf, config[i].current, config->limit); */
            /*                     write(write_pipe, status, status_size); */
            /*                 } */
            /*             } */
            /*         } */
            /*         // uma transformacao */
            /*         else if(q->n_transforms == 4){ // so tem uma transformacao */
            /*             write(write_pipe, "pending\n", sizeof("pending\n")); */
            /*             write(write_pipe, "executing\n", sizeof("executing\n")); */
            /*             if(!(ultimo = fork())){ */
            /*                 int fd_in = open(args[0], O_RDONLY); */
            /*                 int fd_out = open(args[1], O_WRONLY | O_CREAT | O_TRUNC, 0664); */
            /*                 dup2(fd_in, 0); */
            /*                 close(fd_in); */
            /*                 dup2(fd_out, 1); */
            /*                 close(fd_out); */
            /*                 char transformacao[200]; */
            /*                 snprintf(transformacao, sizeof(transformacao), "%s/%s", directory, args[2]); */
            /*                 execlp(transformacao, transformacao, NULL); */
            /*                 perror("Error executing"); */
            /*                 _exit(1); */
            /*             } */
            /*         } */
            /*         else{ */
            /*             write(write_pipe, "pending\n", sizeof("pending\n")); */
            /*             write(write_pipe, "executing\n", sizeof("executing\n")); */
            /*             int pipes[n_transf - 2][2]; */
            /*  */
            /*             int current_pipe = 0; */
            /*             for(int i = 2; i < n_transf - 1; ++i){ */
            /*                 char transformacao[200]; */
            /*                 snprintf(transformacao, sizeof(transformacao), "%s/%s", directory, args[i]); */
            /*                 if(i == 2){ */
            /*                     pipe(pipes[current_pipe]);  */
            /*                     if(!fork()){ */
            /*                         int fd = open(args[0], O_RDONLY); */
            /*                         dup2(fd, 0); */
            /*                         close(fd); */
            /*                         dup2(pipes[current_pipe][1], 1); */
            /*                         close(pipes[current_pipe][1]); */
            /*                         execlp(transformacao, transformacao, NULL); */
            /*                         perror("Error executing"); */
            /*                         _exit(1); */
            /*                     } */
            /*                     else{ */
            /*                         close(pipes[current_pipe][1]); */
            /*                         ++current_pipe; */
            /*                     } */
            /*                 } */
            /*                 else if(i == n_transf - 2){ */
            /*                     if(!(ultimo = fork())){ */
            /*                         int fd = open(args[1], O_WRONLY | O_TRUNC | O_CREAT, 0664); */
            /*                         dup2(fd, 1); */
            /*                         close(fd); */
            /*                         dup2(pipes[current_pipe - 1][0], 0); */
            /*                         close(pipes[current_pipe - 1][0]); */
            /*                         execlp(transformacao, transformacao, NULL); */
            /*                         perror("Error executing"); */
            /*                         _exit(1); */
            /*                     } */
            /*                     else{ */
            /*                         close(pipes[current_pipe - 1][0]); */
            /*                     } */
            /*                 } */
            /*                 else{ */
            /*                     pipe(pipes[current_pipe]); */
            /*                     if(!fork()){ */
            /*                         dup2(pipes[current_pipe - 1][0], 0); */
            /*                         close(pipes[current_pipe - 1][0]); */
            /*                         dup2(pipes[current_pipe][1], 1); */
            /*                         close(pipes[current_pipe][1]); */
            /*                         execlp(transformacao, transformacao, NULL); */
            /*                         perror("Error executing"); */
            /*                         _exit(1); */
            /*                     } */
            /*                     else{ */
            /*                         close(pipes[current_pipe - 1][0]); */
            /*                         close(pipes[current_pipe][1]); */
            /*                         ++current_pipe; */
            /*                     } */
            /*                 } */
            /*             } */
            /*         } */
            /*         if(ultimo != -1){ */
            /*             waitpid(ultimo, NULL, 0); */
            /*             write(write_pipe, "concluded\n", sizeof("concluded\n")); */
            /*         } */
            /*         close(write_pipe); */
            /*         _exit(0); */
            /*     } */
            /*     else{ */
            /*         pids[actual_pid] = pid; */
            /*         pid_status[actual_pid] = EXECUTING; */
            /*         memcpy(pid_transformation[actual_pid++], pedido, sizeof(int) * 7); */
            /*         actual_pid %= 1024;  */
            /*     } */
            /* } */
        }
    }
    close(read_pipe);

    unlink("client_server");
    unlink("server_monitor");

    return 0;
}
