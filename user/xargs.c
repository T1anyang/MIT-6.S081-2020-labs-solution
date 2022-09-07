#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int argc, char* argv[]){
    char* args[MAXARG] = {0};
    int i;
    for(i = 1; i < argc; i++){
        args[i-1] = argv[i];
    }
    for(i = i - 1; i < MAXARG; i++){
        args[i] = (char*) malloc(32);
    }
    i = argc - 1;
    int idx = 0;
    char c = 0;
    while (read(0, &c, 1) == 1){
        args[i][idx] = c;
        if(c == ' '){
            args[i][idx] = 0;
            i++;
            idx = 0;
            continue;
        }
        if(c == '\n'){
            args[i][idx] = 0;
            idx = 0;
            i++;
            char* ptr = args[i];
            args[i] = 0;
            int pid = fork();
            if(pid < 0){
                exit(-1);
            }
            if(pid == 0){
                exec(args[0], args);
                exit(-1);
            }
            else{
                wait(0);
            }
            args[i] = ptr;
            i = argc - 1;
            continue;
        }
        idx++;
    }
    exit(0);
}