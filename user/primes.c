#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define RFD(idx) (((idx)*2)-2)
#define WFD(idx) (((idx)*2)+1)

struct process
{
    int base;
    int idx;
    int rfd;
    int wfd;
};


int fds[24] = {0};

int primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31};
int len = 11;


void work(struct process* p){
    struct process next;
    next.idx = p->idx + 1;
    if(next.idx > 11){
        return;
    }
    next.base = primes[next.idx];
    pipe(&fds[(p->idx)*2]);
    p->wfd = fds[WFD(p->idx)];
    next.rfd = fds[RFD(next.idx)];
    int pid = fork();
    if(pid == 0){ // child
        close(p->wfd);
        work(&next);
        close(next.rfd);
        return;
    }
    else{ // parent
        close(next.rfd);
        char num;
        if(p->idx){
            while(read(p->rfd, &num, 1) > 0){
                if(num == p->base){
                    printf("prime %d\n", num);
                }
                else if(num % p->base){
                    write(p->wfd, &num, 1);
                }
                else{
                    // drop: do nothing
                }
            }
        }
        else{
            for(num = 2; num < 35; num++){
                if(num == p->base){
                    printf("prime %d\n", num);
                }
                else if(num % p->base){
                    write(p->wfd, &num, 1);
                }
                else{
                    // drop: do nothing
                }
            }
        }
        close(p->wfd);
        wait(0);
        return;
    }
}

int main(){
    struct process p;
    p.base = 2;
    p.idx = 0;
    p.rfd = 0;
    p.wfd = 0;
    work(&p);
    exit(0);
}