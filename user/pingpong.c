#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


int main(){
    int pipefd[4];
    char buffer[4] = {0};
    if(pipe(pipefd) < 0){
        fprintf(2, "pipe error\n");
        exit(-1);
    }
    if(pipe(&(pipefd[2])) < 0){
        fprintf(2, "pipe error\n");
        exit(-1);
    }
    int pid = fork();
    if(pid < 0){
        fprintf(2, "fork error\n");
        exit(-1);
    }
    if(pid == 0){ // child
        close(pipefd[1]);
        close(pipefd[2]);
        pid = getpid();
        if(read(pipefd[0], buffer, 1) > 0){
            printf("%d: received ping\n", pid);
            write(pipefd[3], "b", 1);
            close(pipefd[0]);
            close(pipefd[3]);
            exit(0);
        }
        exit(-1);
    }
    else{ // parent
        close(pipefd[0]);
        close(pipefd[3]);
        pid = getpid();
        write(pipefd[1], "a", 1);
        if(read(pipefd[2], buffer, 1) > 0){
            printf("%d: received pong\n", pid);
            close(pipefd[1]);
            close(pipefd[2]);
            exit(0);
        }
        exit(-1);
    }
}