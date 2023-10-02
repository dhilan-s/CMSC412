/*************************************************************************/
/*
 * GeekOS master source distribution and/or project solution
 * Copyright (c) 2001,2003,2004 David H. Hovemeyer <daveho@cs.umd.edu>
 * Copyright (c) 2003,2013,2014 Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 *
 * All rights reserved.
 *
 * This code may not be resdistributed without the permission of the copyright holders.
 * Any student solutions using any of this code base constitute derviced work and may
 * not be redistributed in any form.  This includes (but is not limited to) posting on
 * public forums or web sites, providing copies to (past, present, or future) students
 * enrolled in similar operating systems courses the University of Maryland's CMSC412 course.
 */
/*************************************************************************/
/*
 * Signals
 * $Rev $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/kassert.h>
#include <geekos/defs.h>
#include <geekos/screen.h>
#include <geekos/int.h>
#include <geekos/mem.h>
#include <geekos/symbol.h>
#include <geekos/string.h>
#include <geekos/kthread.h>
#include <geekos/malloc.h>
#include <geekos/user.h>
#include <geekos/signal.h>
#include <geekos/projects.h>
#include <geekos/alarm.h>
#include <geekos/smp.h>


/* Called when signal handling is complete. */
void Complete_Handler(struct Kernel_Thread *kthread,
                      struct Interrupt_State *state) {
    KASSERT(kthread);
    KASSERT(state);
    struct User_Interrupt_State *userState = (struct User_Interrupt_State *)(state);
    userState->espUser += sizeof(int); // remove signal number
    memcpy(state, (void *)userState->espUser, sizeof(struct Interrupt_State)); //Restore interrupt_state on userstack to interrupt_state on current thread
    userState->espUser += sizeof(struct Interrupt_State);
}

int Check_Pending_Signal(struct Kernel_Thread *kthread,
                         struct Interrupt_State *state) {
    KASSERT(kthread);
    KASSERT(state);

    bool pending = 0;
    int i;
    for (i = 0; i<6; i++){
        if ((kthread->sigFlags)[i]){
            pending = 1;
            break;
        }

    }
    if (pending && state->cs != KERNEL_CS && !kthread->busy){
        kthread->busy = 1;
        return 1;
    }
    return 0;
}

#if 0
void Print_IS(struct Interrupt_State *esp) {
    void **p;
    Print("esp=%x:\n", (unsigned int)esp);
    Print("  gs=%x\n", (unsigned int)esp->gs);
    Print("  fs=%x\n", (unsigned int)esp->fs);
    Print("  es=%x\n", (unsigned int)esp->es);
    Print("  ds=%x\n", (unsigned int)esp->ds);
    Print("  ebp=%x\n", (unsigned int)esp->ebp);
    Print("  edi=%x\n", (unsigned int)esp->edi);
    Print("  esi=%x\n", (unsigned int)esp->esi);
    Print("  edx=%x\n", (unsigned int)esp->edx);
    Print("  ecx=%x\n", (unsigned int)esp->ecx);
    Print("  ebx=%x\n", (unsigned int)esp->ebx);
    Print("  eax=%x\n", (unsigned int)esp->eax);
    Print("  intNum=%x\n", (unsigned int)esp->intNum);
    Print("  errorCode=%x\n", (unsigned int)esp->errorCode);
    Print("  eip=%x\n", (unsigned int)esp->eip);
    Print("  cs=%x\n", (unsigned int)esp->cs);
    Print("  eflags=%x\n", (unsigned int)esp->eflags);
    p = (void **)(((struct Interrupt_State *)esp) + 1);
    Print("esp+n=%x\n", (unsigned int)p);
    Print("esp+n[0]=%x\n", (unsigned int)p[0]);
    Print("esp+n[1]=%x\n", (unsigned int)p[1]);
}

void dump_stack(unsigned int *esp, unsigned int ofs) {
    int i;
    Print("Setup_Frame: Stack dump\n");
    for(i = 0; i < 25; i++) {
        Print("[%x]: %x\n", (unsigned int)&esp[i] - ofs, esp[i]);
    }
}
#endif

void Setup_Frame(struct Kernel_Thread *kthread,
                 struct Interrupt_State *state) {
    KASSERT(kthread);
    KASSERT(state);
    int i;
    for (i = 0; i < 6; i++){
        if((CURRENT_THREAD->sigFlags)[i]){
            if ((CURRENT_THREAD->sigHandlers)[i] == SIG_DFL){
                switch (i+1){
                    case 1:
                    Exit(257);
                    break;

                    case 2:
                    Exit(258);
                    break;

                    case 3:
                    Exit(259);
                    break;

                    case 4:
                    // IGNORE
                    break;

                    case 5:
                    Exit(261);
                    break;

                    case 6:
                    Exit(262);
                    break;
                }
                goto done;
            }
            if ((CURRENT_THREAD->sigHandlers)[i] == SIG_IGN){
                goto done;
            }
            
            struct User_Interrupt_State *userState = (struct User_Interrupt_State *)(state);
            
            
            userState->espUser -= sizeof(struct Interrupt_State);
            memcpy((void *)(userState->espUser), state, sizeof(struct Interrupt_State));
            
            userState->espUser -= sizeof(int);
            *((ulong_t *) userState->espUser) = (i+1);

            userState->espUser -= sizeof(void *);
            memcpy((void *)userState->espUser, (CURRENT_THREAD->userContext)->trampoline, sizeof(void *));

            state->eip = (uint_t)(CURRENT_THREAD->sigHandlers)[i];

            done:
            break;
        }
    }
}
