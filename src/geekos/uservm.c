/*
 * Paging-based user mode implementation
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
 * $Revision: 1.51 $
 */

#include <geekos/int.h>
#include <geekos/mem.h>
#include <geekos/paging.h>
#include <geekos/malloc.h>
#include <geekos/string.h>
#include <geekos/argblock.h>
#include <geekos/kthread.h>
#include <geekos/range.h>
#include <geekos/vfs.h>
#include <geekos/user.h>
#include <geekos/projects.h>
#include <geekos/smp.h>
#include <geekos/synch.h>
#include <geekos/errno.h>
#include <geekos/gdt.h>
#include <geekos/segment.h>

#define CEIL_DIV(x, y) (((x) + (y) - 1) / (y))
#define DEFAULT_USER_STACK_SIZE 8192


extern Spin_Lock_t kthreadLock;

int userDebug = 0;
#define Debug(args...) if (userDebug) Print("uservm: " args)

/* ----------------------------------------------------------------------
 * Private functions
 * ---------------------------------------------------------------------- */

// TODO: Add private functions

void Map_Page_Pageable(pde_t *pageDir, ulong_t addr){
    if (addr == 0x80000000){
        return;
    }
    int pdeIndex = PAGE_DIRECTORY_INDEX(addr);
    int pteIndex = PAGE_TABLE_INDEX(addr);
    pde_t pde = pageDir[pdeIndex];
    pte_t *pte;
    if (!pde.present){
        pde.present = 1;
        pde.flags = VM_WRITE | VM_READ | VM_USER;
        pte = Alloc_Page();
        pde.pageTableBaseAddr = PAGE_ALIGNED_ADDR(pte);
        pageDir[pdeIndex] = pde;
    }
    else{
        pte = (pte_t *)(pde.pageTableBaseAddr << 12);
    }
    pte[pteIndex].present = 1;
    pte[pteIndex].flags = VM_WRITE | VM_READ | VM_USER;
    void *allocedAddr = Alloc_Pageable_Page(&pte[pteIndex], addr);
    pte[pteIndex].pageBaseAddr = PAGE_ALIGNED_ADDR(allocedAddr);
    pageDir[pdeIndex] = pde;
}

void Map_Segment(struct Exe_Segment *segment, pde_t *pageDir) {
    int i;
    int j;
    int startAddr = segment->startAddress;
    if (startAddr == 0){
        return;
    }
    //Print("startAddr: %x\n", startAddr);
    int size = segment->sizeInMemory;
    //Print("size: %x\n", size);
    //int startPage = startAddr / PAGE_SIZE;
    int numPages = CEIL_DIV(size, PAGE_SIZE);
    pte_t *pageTable;
    int pageFlags = VM_WRITE | VM_READ | VM_USER;
    int pDir = PAGE_DIRECTORY_INDEX(0x80000000 + startAddr);
    int pTable = PAGE_TABLE_INDEX(0x80000000 + startAddr);
    //Print("pDir: %d, pTable: %d\n", pDir, pTable);

    int numPageTables = CEIL_DIV(numPages,1024);
    //Print("numPageTables: %d\n", numPageTables);

    for (i = pDir; i<pDir+numPageTables; i++){
        //Print("i: %d\n", i);
        if (!pageDir[i].present){
            //Print("pageDir[i] not present\n");
            pageDir[i].present = 1;
            pageDir[i].flags = pageFlags;

            // Allocate space for the page table
            pageTable = Alloc_Page();
            pageDir[i].pageTableBaseAddr = PAGE_ALIGNED_ADDR(pageTable);
        }

        else{
            //Print("pageDir[i] present\n");
            pageTable = (pte_t *)(pageDir[i].pageTableBaseAddr << 12);
            //Print("pageTable found\n");
        }

        for(j = pTable; j<pTable+numPages; j++){
                //Print("j: %d\n", j);
                pageTable[j].present = 1;
                pageTable[j].flags = VM_WRITE | VM_READ | VM_USER;
                void *allocedAddr = Alloc_Pageable_Page(&pageTable[j], Round_Down_To_Page(((i << 10) | j)+0x80000000));
                //Print("i: %d, j: %d\n", i, j);
                pageTable[j].pageBaseAddr = PAGE_ALIGNED_ADDR(allocedAddr);
        }
    }
}

void *User_To_Kernel(struct User_Context *userContext, ulong_t userPtr) {
    uchar_t *userBase = (uchar_t *) 0x80000000;

    return (void *)(userBase + userPtr);
}

/*
 * Create a new user context of given size
 */
extern struct User_Context *Create_User_Context(ulong_t size) {
    struct User_Context *context;
    pde_t *page_dir;
    page_dir = Alloc_Page();
    memcpy(page_dir, Kernel_Page_Dir(), PAGE_SIZE);
    int index;

    /* Size must be a multiple of the page size */
    // size = Round_Up_To_Page(size); don't think we need size
    size = 0x70000000;
    if(userDebug)
        Print("Size of user memory == %lu (%lx) (%lu pages)\n", size,
              size, size / PAGE_SIZE);

    /* Allocate memory for the user context */
    context = (struct User_Context *)Malloc(sizeof(*context));
    if(context != 0) {
        memset(context, 0, sizeof(struct User_Context));
        context->memory = (char *)0x80000000;
    }

    if(context == 0 || context->memory == 0)
        goto fail;

    context->size = size;

    /* Allocate an LDT descriptor for the user context */
    context->ldtDescriptor = Allocate_Segment_Descriptor();
    if(context->ldtDescriptor == 0)
        goto fail;
    if(userDebug)
        Print("Allocated descriptor %d for LDT\n",
              Get_Descriptor_Index(context->ldtDescriptor));
    Init_LDT_Descriptor(context->ldtDescriptor, context->ldt,
                        NUM_USER_LDT_ENTRIES);
    index = Get_Descriptor_Index(context->ldtDescriptor);
    context->ldtSelector = Selector(KERNEL_PRIVILEGE, true, index);

    /* Initialize code and data segments within the LDT */
    Init_Code_Segment_Descriptor(&context->ldt[0],
                                 (ulong_t) context->memory,
                                 size / PAGE_SIZE, USER_PRIVILEGE);
    Init_Data_Segment_Descriptor(&context->ldt[1],
                                 (ulong_t) context->memory,
                                 size / PAGE_SIZE, USER_PRIVILEGE);
    context->csSelector = Selector(USER_PRIVILEGE, false, 0);
    context->dsSelector = Selector(USER_PRIVILEGE, false, 1);

    context->pageDir = page_dir;
    
    /* Nobody is using this user context yet */
    context->refCount = 0;


    /* Success! */
    return context;

  fail:
    /* We failed; release any allocated memory */
    if(context != 0) {
        Free(context);
    }

    return 0;
}



/* ----------------------------------------------------------------------
 * Public functions
 * ---------------------------------------------------------------------- */

/*
 * Destroy a User_Context object, including all memory
 * and other resources allocated within it.
 */
void Destroy_User_Context(struct User_Context *context) {
    /*
     * Hints:
     * - Free all pages, page tables, and page directory for
     *   the process (interrupts must be disabled while you do this,
     *   otherwise those pages could be stolen by other processes)
     * - Free semaphores, files, and other resources used
     *   by the process
     */
    /* 
    KASSERT(context->refCount == 0);

   
    Disable_Interrupts();
    pde_t *pageDir = context->pageDir;
    pte_t *pageTable;
    int i;
    int j;
    int startAddr = 0x80000000;
    int size = context->size;
    int numPages = size / PAGE_SIZE;
    int numPageTables = CEIL_DIV(numPages,1024);

    for (i = (int)PAGE_DIRECTORY_INDEX(0x80000000 + startAddr); i<(int)PAGE_DIRECTORY_INDEX(0x80000000 + startAddr)+numPageTables; i++){

        pageTable = (pte_t *)PAGE_ADDR(pageDir[i].pageTableBaseAddr);

        // Map the page table
        if (i<numPageTables-1){
            for(j = 0; j<1024; j++){
                if (pageTable[j].present){
                    Free_Page((void *)PAGE_ADDR(pageTable[j].pageBaseAddr));
                }
            }
        }
        Free_Page((void *)PAGE_ADDR(pageDir[i].pageTableBaseAddr));
    }
    Free_Page(pageDir);
    Enable_Interrupts();
    */

    /* Free the context's LDT descriptor
    Free_Segment_Descriptor(context->ldtDescriptor);
    */

    /* Free the context's memory 
    Free(context->memory);
    Free(context);
    */

}

/*
 * Load a user executable into memory by creating a User_Context
 * data structure.
 * Params:
 * exeFileData - a buffer containing the executable to load
 * exeFileLength - number of bytes in exeFileData
 * exeFormat - parsed ELF segment information describing how to
 *   load the executable's text and data segments, and the
 *   code entry point address
 * command - string containing the complete command to be executed:
 *   this should be used to create the argument block for the
 *   process
 * pUserContext - reference to the pointer where the User_Context
 *   should be stored
 *
 * Returns:
 *   0 if successful, or an error code (< 0) if unsuccessful
 */
int Load_User_Program(char *exeFileData, ulong_t exeFileLength,
                      struct Exe_Format *exeFormat, const char *command,
                      struct User_Context **pUserContext) {
    /*
     * Hints:
     * - This will be similar to the same function in userseg.c
     * - Determine space requirements for code, data, argument block,
     *   and stack
     * - Allocate pages for above, map them into user address
     *   space (allocating page directory and page tables as needed)
     * - Fill in initial stack pointer, argument block address,
     *   and code entry point fields in User_Context
     */
    int i;
    ulong_t maxva = 0;
    unsigned numArgs;
    ulong_t argBlockSize;
    ulong_t size, argBlockAddr;
    struct User_Context *userContext = 0;
    pde_t *holdPDBR = Get_PDBR();

    /* Find maximum virtual address */
    for(i = 0; i < exeFormat->numSegments; ++i) {
        struct Exe_Segment *segment = &exeFormat->segmentList[i];
        ulong_t topva = segment->startAddress + segment->sizeInMemory;  /* FIXME: range check */

        if(topva > maxva)
            maxva = topva;
    }

    /* Determine size required for argument block */
    Get_Argument_Block_Size(command, &numArgs, &argBlockSize);

    /*
     * Now we can determine the size of the memory block needed
     * to run the process.
     */
    size = Round_Up_To_Page(maxva) + DEFAULT_USER_STACK_SIZE;
    argBlockAddr = 0xF0000000;
    size += argBlockSize;

    /* Create User_Context */
    userContext = Create_User_Context(size);
    if(userContext == 0)
        return -1;

    Set_PDBR(userContext->pageDir);
    /* Load segment data into memory */
    for(i = 0; i < exeFormat->numSegments; ++i) {
        struct Exe_Segment *segment = &exeFormat->segmentList[i];

        Map_Segment(segment, userContext->pageDir);
        memcpy(userContext->memory + segment->startAddress,
               exeFileData + segment->offsetInFile,
               segment->lengthInFile);
        
        
    }

    // Map argument block
    int argBlockStart = Round_Down_To_Page(argBlockAddr - argBlockSize);
    Map_Page_Pageable(userContext->pageDir, argBlockStart);

    // Map stack
    userContext -> stackSize = DEFAULT_USER_STACK_SIZE;
    int j;
    for (j = 0; j <= DEFAULT_USER_STACK_SIZE/PAGE_SIZE; j++){
        Map_Page_Pageable(userContext->pageDir, Round_Down_To_Page(argBlockStart - (j*PAGE_SIZE)));
    }
   
    /* Format argument block */
    Format_Argument_Block((char *)(argBlockAddr - argBlockSize), numArgs,
                          argBlockAddr - 0x80000000 - argBlockSize, command);

    /* Fill in code entry point */
    userContext->entryAddr = exeFormat->entryAddr;

    /*
     * Fill in addresses of argument block and stack
     * (They happen to be the same)
     */
    userContext->argBlockAddr = argBlockAddr - 0x80000000 - argBlockSize;
    userContext->stackPointerAddr = argBlockAddr - 0x80000000 - argBlockSize;


    *pUserContext = userContext;
    Set_PDBR(holdPDBR);
    Print("user program loaded\n");
    //KASSERT(false);
    return 0;
}

/*
 * Copy data from user buffer into kernel buffer.
 * Returns true if successful, false otherwise.
 */
bool Copy_From_User(void *destInKernel, ulong_t srcInUser,
                    ulong_t numBytes) {
    
    pde_t *holdPDBR = Get_PDBR();
    Set_PDBR(CURRENT_THREAD->userContext->pageDir);
    struct User_Context *current = CURRENT_THREAD->userContext;

    //if(!Validate_User_Memory(current, srcInUser, numBytes, VUM_READING))
    //  return false;
    memcpy(destInKernel, User_To_Kernel(current, srcInUser), numBytes);
    Set_PDBR(holdPDBR);
    return true;
}

/*
 * Copy data from kernel buffer into user buffer.
 * Returns true if successful, false otherwise.
 */
bool Copy_To_User(ulong_t destInUser, const void *srcInKernel,
                  ulong_t numBytes) {
  
    pde_t *holdPDBR = Get_PDBR();
    Set_PDBR(CURRENT_THREAD->userContext->pageDir);
    struct User_Context *current = CURRENT_THREAD->userContext;

    memcpy(User_To_Kernel(current, destInUser), srcInKernel, numBytes);
    Set_PDBR(holdPDBR);
    return true;
}


/*
 * Switch to user address space.
 */
void Switch_To_Address_Space(struct User_Context *userContext) {
    ushort_t ldtSelector;

    /* Eager check to ensure that the new address space
       has either memory (userseg) or a page directory 
       (uservm) and is not likely to abort */
    KASSERT(userContext->memory || userContext->pageDir);

    /* Switch to the LDT of the new user context */
    ldtSelector = userContext->ldtSelector;
    __asm__ __volatile__("lldt %0"::"a"(ldtSelector)
        );

    Set_PDBR(userContext->pageDir);
}
