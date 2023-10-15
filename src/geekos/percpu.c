#include <geekos/percpu.h>
#include <geekos/ktypes.h>
#include <geekos/errno.h>
#include <geekos/malloc.h>

void Init_PerCPU(int cpu){
    //perCPUarr[cpu] = Malloc(sizeof(struct CPU_Descriptor));
    perCPUarr[cpu].CPU_id = cpu;
    perCPUarr[cpu].current_thread = NULL;
}
int PerCPU_Get_CPU(void){
    int CPUid;
    __asm__ __volatile__(
    "movl %%gs:(0), %0" 
    : "=r" (CPUid) 
    );
    return CPUid;
}

struct Kernel_Thread *PerCPU_Get_Current(void){
    struct Kernel_Thread *current_thread;
    __asm__ __volatile__(
    "movl %%gs:(4), %0"
    : "=r" (current_thread)
    );
    return current_thread;
}