#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

int pathmatch(char* path, char* target){
    char* p;
    for(p=path+strlen(path); p >= path && *p != '/'; p--);
    p++;
    return !strcmp(p, target);
}

void find(char* path, char* target){
    int fd = 0;
    struct stat st;
    struct dirent de;
    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }
    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }
    if(pathmatch(path, target)){
        printf("%s\n", path);
    }
    if(st.type == T_DIR){
        while(read(fd, &de, sizeof(de)) == sizeof(de)){
            if(de.inum && strcmp(de.name, ".") && strcmp(de.name, "..")){
                char buffer[32];
                int len = strlen(path);
                strcpy(buffer, path);
                buffer[len] = '/';
                strcpy(buffer + len + 1, de.name);
                find(buffer, target);
            }
        }
    }
    close(fd);
}

int main(int argc, char* argv[]){
    if(argc < 3){
        fprintf(2, "error: missing argument\n");
        exit(-1);
    }
    char* path = argv[1];
    char* target = argv[2];
    find(path, target);
    exit(0);
}