/* This file is a placeholder for storing per-cpu variables in a segment that is not 
   saved and restored per thread, but rather left alone although different on a per-cpu
   basis. */

#include <geekos/ktypes.h>
#ifndef NULL
#define NULL ((void *)0)
#endif
#define THREAD_OFFSET 8
#define ID_OFFSET 0

void Init_PerCPU(int cpu);
int PerCPU_Get_CPU(void);
struct Kernel_Thread *PerCPU_Get_Current(void);

struct CPU_Descriptor{
   uint_t CPU_id;
   struct Kernel_Thread *current_thread;
};

struct CPU_Descriptor perCPUarr[8];
