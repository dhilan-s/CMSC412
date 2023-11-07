/*
 * Paging (virtual memory) support
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
 *
 * $Revision: 1.56 $
 * 
 */

#include <geekos/string.h>
#include <geekos/int.h>
#include <geekos/idt.h>
#include <geekos/kthread.h>
#include <geekos/kassert.h>
#include <geekos/screen.h>
#include <geekos/mem.h>
#include <geekos/malloc.h>
#include <geekos/gdt.h>
#include <geekos/segment.h>
#include <geekos/user.h>
#include <geekos/vfs.h>
#include <geekos/crc32.h>
#include <geekos/paging.h>
#include <geekos/errno.h>
#include <geekos/projects.h>
#include <geekos/smp.h>
#include <libc/mmap.h>

/* ----------------------------------------------------------------------
 * Public data
 * ---------------------------------------------------------------------- */
pde_t *g_kernel_page_dir;       /* kernel page directory */

/* ----------------------------------------------------------------------
 * Private functions/data
 * ---------------------------------------------------------------------- */

#define SECTORS_PER_PAGE (PAGE_SIZE / SECTOR_SIZE)

#define CEIL_DIV(x, y) (((x) + (y) - 1) / (y))

/*
 * flag to indicate if debugging paging code
 */
int debugFaults = 0;
#define Debug(args...) if (debugFaults) Print(args)


/* const because we do not expect any caller to need to
   modify the kernel page directory */
const pde_t *Kernel_Page_Dir(void) {
    return g_kernel_page_dir;
}



/*
 * Print diagnostic information for a page fault.
 */
static void Print_Fault_Info(uint_t address, faultcode_t faultCode) {
    extern uint_t g_freePageCount;
    struct Kernel_Thread *current = get_current_thread(0);      /* informational, could be incorrect with low probability */

    if(current) {
        Print("Pid %d: (%p/%s)", current->pid, current,
              current->threadName);
    }
    Print("\n Page Fault received, at address %p (%d pages free)\n",
          (void *)address, g_freePageCount);
    if(faultCode.protectionViolation)
        Print("   Protection Violation, ");
    else
        Print("   Non-present page, ");
    if(faultCode.writeFault)
        Print("Write Fault, ");
    else
        Print("Read Fault, ");
    if(faultCode.userModeFault)
        Print("in User Mode\n");
    else
        Print("in Supervisor Mode\n");
}

union type_pun_workaround {
    faultcode_t faultCode;
    ulong_t errorCode;
};

/*
 * Handler for page faults.
 * You should call the Install_Interrupt_Handler() function to
 * register this function as the handler for interrupt 14.
 */
/*static*/ void Page_Fault_Handler(struct Interrupt_State *state) {
    ulong_t address;
    union type_pun_workaround tpw;
    faultcode_t faultCode;

    KASSERT(!Interrupts_Enabled());

    /* Get the address that caused the page fault */
    address = Get_Page_Fault_Address();
    Debug("Page fault @%lx\n", address);


    if(address < 0xfec01000 && address > 0xf0000000) {
        Print("page fault address in APIC/IOAPIC range\n");
        goto error;
    }

    /* Get the fault code */
    tpw.errorCode = state->errorCode;
    faultCode = tpw.faultCode;
    // faultCode = *((faultcode_t *) &(state->errorCode));

    /* rest of your handling code here */
    if (state->errorCode == 6){
        if (address - CURRENT_THREAD->userContext->stackSize <= PAGE_SIZE+CURRENT_THREAD->userContext->stackPointerAddr){
            // Allocate a new page
            pte_t *newPage = Alloc_Page();
            memset(newPage, 0, PAGE_SIZE);
            // Map the page
            int pageDirIndex = PAGE_DIRECTORY_INDEX(address);
            int pageTableIndex = PAGE_TABLE_INDEX(address);
            pte_t *pageTable = (pte_t *) (CURRENT_THREAD->userContext->pageDir[pageDirIndex].pageTableBaseAddr << 12);
            pageTable[pageTableIndex].present = 1;
            pageTable[pageTableIndex].flags = VM_WRITE | VM_READ | VM_USER;
            pageTable[pageTableIndex].pageBaseAddr = (address >> 12);
            CURRENT_THREAD->userContext->stackSize += PAGE_SIZE;
        }
        else{
            Print("Stack overflow\n");
            Dump_Interrupt_State(state);
            Exit(-1);
        }
    }
    TODO_P(PROJECT_VIRTUAL_MEMORY_B, "handle page faults");

    TODO_P(PROJECT_MMAP, "handle mmap'd page faults");


  error:
    Print("Unexpected Page Fault received\n");
    Print_Fault_Info(address, faultCode);
    Dump_Interrupt_State(state);
    /* user faults just kill the process; not user mode faults should halt the kernel. */
    KASSERT0(faultCode.userModeFault,
             "unhandled kernel-mode page fault.");

    /* For now, just kill the thread/process. */
    Enable_Interrupts();
    Exit(-1);
}

void Identity_Map_Page(pde_t * currentPageDir, unsigned int address,
                       int flags) {
}

/* ----------------------------------------------------------------------
 * Public functions
 * ---------------------------------------------------------------------- */


/*
 * Initialize virtual memory by building page tables
 * for the kernel and physical memory.
 */
void Init_VM(struct Boot_Info *bootInfo) {
    /*
     * Hints:
     * - Build kernel page directory and page tables
     * - Call Enable_Paging() with the kernel page directory
     * - Install an interrupt handler for interrupt 14,
     *   page fault
     * - Do not map a page at address 0; this will help trap
     *   null pointer references
     */
    // Declare loop variables
    int i;
    int j;

    // Find the number of pages needed for the kernel
    int numPages = CEIL_DIV(bootInfo->memSizeKB,4);
    int numPageTables = CEIL_DIV(numPages,1024);

    // Allocate space for the kernel page directory
    g_kernel_page_dir = Alloc_Page();
    memset(g_kernel_page_dir, 0, PAGE_SIZE);

    // Map page directories and page tables (but not PDE_T 0)
    //g_kernel_page_dir[0].present = 0;
    //g_kernel_page_dir[0].flags = VM_WRITE | VM_READ | VM_USER;
    for (i = 0; i<numPageTables; i++){
        g_kernel_page_dir[i].present = 1;
        g_kernel_page_dir[i].flags = VM_WRITE | VM_READ | VM_USER;

        // Allocate space for the page table
        pte_t *pageTable = Alloc_Page();
        g_kernel_page_dir[i].pageTableBaseAddr = PAGE_ALIGNED_ADDR(pageTable);

        // Map the page table
        if (i<numPageTables-1){
            for(j = 0; j<1024; j++){
                if (i == 0 && j == 0){
                    pageTable[j].present = 0;
                }
                else{
                    pageTable[j].present = 1;
                    pageTable[j].flags = VM_WRITE | VM_READ | VM_USER;
                    pageTable[j].pageBaseAddr = ((i << 10) | j);
                }
                }
        }
        // Last entry only maps remainder of pages
        else if (i == numPageTables-1){
            for(j = 0; j<numPages%1024; j++){
                if (i == 0 && j == 0){
                    pageTable[j].present = 0;
                }
                else{
                    pageTable[j].present = 1;
                    pageTable[j].flags = VM_WRITE | VM_READ | VM_USER;
                    pageTable[j].pageBaseAddr = ((i << 10) | j);
                }
                }
        }
    }

    // Find APIC directory and page table
    int APICdir = PAGE_DIRECTORY_INDEX(0xFEE00000);
    int APICpage = PAGE_TABLE_INDEX(0xFEE00000);
    int APICIOpage = PAGE_TABLE_INDEX(0xFEC00000);

    // Set up APIC directory entry and page table
    if (!g_kernel_page_dir[APICdir].present){
        g_kernel_page_dir[APICdir].present = 1;
        g_kernel_page_dir[APICdir].flags = VM_WRITE | VM_READ;
        pte_t *APICpageTable = Alloc_Page();
        memset(APICpageTable, 0, PAGE_SIZE);
        g_kernel_page_dir[APICdir].pageTableBaseAddr = PAGE_ALIGNED_ADDR(APICpageTable);
        
    }
    pte_t *APICpageTable = (pte_t *) (g_kernel_page_dir[APICdir].pageTableBaseAddr << 12);
    // Set up APIC and APIC IO page table entries
    APICpageTable[APICpage].present = 1;
    APICpageTable[APICpage].flags = VM_WRITE | VM_READ;
    APICpageTable[APICpage].pageBaseAddr = 0xFEE00;

    APICpageTable[APICIOpage].present = 1;
    APICpageTable[APICIOpage].flags = VM_WRITE | VM_READ;
    APICpageTable[APICIOpage].pageBaseAddr = 0xFEC00;
    
    // Enable paging and install interrupt handler
    Enable_Paging(g_kernel_page_dir);
    Install_Interrupt_Handler(14, Page_Fault_Handler);
    Install_Interrupt_Handler(46, Page_Fault_Handler);

}

void Init_Secondary_VM() {
    Enable_Paging(g_kernel_page_dir);
}

/**
 * Initialize paging file data structures.
 * All filesystems should be mounted before this function
 * is called, to ensure that the paging file is available.
 */
void Init_Paging(void) {
    TODO_P(PROJECT_VIRTUAL_MEMORY_B,
           "Initialize paging file data structures");
}

/* guards your structure for tracking free space on the paging file. */
static Spin_Lock_t s_free_space_spin_lock;

/**
 * Find a free bit of disk on the paging file for this page.
 * Interrupts must be disabled.
 * @return index of free page sized chunk of disk space in
 *   the paging file, or -1 if the paging file is full
 */
int Find_Space_On_Paging_File(void) {
    unsigned int retval;
    int iflag = Begin_Int_Atomic();
    Spin_Lock(&s_free_space_spin_lock);
    TODO_P(PROJECT_VIRTUAL_MEMORY_B, "Find free page in paging file");
    retval = EUNSUPPORTED;
    Spin_Unlock(&s_free_space_spin_lock);
    End_Int_Atomic(iflag);
    return retval;
}

/**
 * Free a page-sized chunk of disk space in the paging file.
 * Interrupts must be disabled.
 * @param pagefileIndex index of the chunk of disk space
 */
void Free_Space_On_Paging_File(int pagefileIndex) {
    int iflag = Begin_Int_Atomic();
    Spin_Lock(&s_free_space_spin_lock);
    TODO_P(PROJECT_VIRTUAL_MEMORY_B, "Free page in paging file");
    Spin_Unlock(&s_free_space_spin_lock);
    End_Int_Atomic(iflag);
}

/**
 * Write the contents of given page to the indicated block
 * of space in the paging file.
 * @param paddr a pointer to the physical memory of the page
 * @param vaddr virtual address where page is mapped in user memory
 * @param pagefileIndex the index of the page sized chunk of space
 *   in the paging file
 */
void Write_To_Paging_File(void *paddr, ulong_t vaddr, int pagefileIndex) {
    struct Page *page = Get_Page((ulong_t) paddr);
    KASSERT(!(page->flags & PAGE_PAGEABLE));    /* Page must be pageable! */
    KASSERT(page->flags & PAGE_LOCKED); /* Page must be locked! */
    TODO_P(PROJECT_VIRTUAL_MEMORY_B, "Write page data to paging file");
}

/**
 * Read the contents of the indicated block
 * of space in the paging file into the given page.
 * @param paddr a pointer to the physical memory of the page
 * @param vaddr virtual address where page will be re-mapped in
 *   user memory
 * @param pagefileIndex the index of the page sized chunk of space
 *   in the paging file
 */
void Read_From_Paging_File(void *paddr, ulong_t vaddr, int pagefileIndex) {
    struct Page *page = Get_Page((ulong_t) paddr);
    KASSERT(!(page->flags & PAGE_PAGEABLE));    /* Page must be locked! */
    TODO_P(PROJECT_VIRTUAL_MEMORY_B, "Read page data from paging file");
}


void *Mmap_Impl(void *ptr, unsigned int length, int prot, int flags,
                int fd) {
    TODO_P(PROJECT_MMAP, "Mmap setup mapping");
    return NULL;
}

bool Is_Mmaped_Page(struct User_Context * context, ulong_t vaddr) {
    TODO_P(PROJECT_MMAP,
           "is this passed vaddr an mmap'd page in the passed user context");
    return false;
}

void Write_Out_Mmaped_Page(struct User_Context *context, ulong_t vaddr) {
    TODO_P(PROJECT_MMAP, "Mmap write back dirty mmap'd page");
}

int Munmap_Impl(ulong_t ptr) {
    TODO_P(PROJECT_MMAP, "unmapp the pages");
    return 0;
}
