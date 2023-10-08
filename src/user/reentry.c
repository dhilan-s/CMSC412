#include <conio.h>
#include <process.h>
#include <sched.h>
#include <sema.h>
#include <string.h>
#include <fileio.h>
#include <signal.h>
#include <geekos/errno.h>

int signaled = 0;

void sig_user2(int signum){
    Print("signal handler 2\n");
    Kill(Get_PID(), SIGUSR1);
    Kill(Get_PID(), SIGUSR1);
    Kill(Get_PID(), SIGUSR1);
    signaled = 1;
}

void sig_user1(int signum){
    Signal(sig_user2, SIGUSR1);
    Kill(Get_PID(), SIGUSR1);
    Print("signal handler 1\n");
}

int main(void){
    Print("main start\n");
    Signal(sig_user1, SIGUSR1);
    Kill(Get_PID(), SIGUSR1);
    while(!signaled){
        Get_PID();
    }
    Print("main end\n");
    return 1;
}