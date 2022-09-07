#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int usr_sleep(int click){
    return sleep(click);
}

int main(int argc, char* argv[]){
    if(argc == 1){
        fprintf(2, "sleep: missing operand\n");
        exit(-1);
    }
    else{
        usr_sleep(atoi(argv[1]));
        exit(0);
    }
}