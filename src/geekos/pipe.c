/*
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
#include <geekos/pipe.h>
#include <geekos/malloc.h>
#include <geekos/string.h>
#include <geekos/errno.h>
#include <geekos/projects.h>
#include <geekos/int.h>


const struct File_Ops Pipe_Read_Ops =
    { NULL, Pipe_Read, NULL, NULL, Pipe_Close, NULL };
const struct File_Ops Pipe_Write_Ops =
    { NULL, NULL, Pipe_Write, NULL, Pipe_Close, NULL };
extern struct Mutex s_vfsLock;


int Pipe_Create(struct File **read_file, struct File **write_file) {
    struct Pipe *newPipe = Malloc(sizeof(struct Pipe));
    if (newPipe == NULL)
        return ENOMEM;

    *read_file = Allocate_File(&Pipe_Read_Ops, 0, 0, newPipe, 0, 0);
    newPipe->readers = 1;

    *write_file = Allocate_File(&Pipe_Write_Ops, 0, 0, newPipe, 0, 0);
    newPipe->writers = 1;

    if (read_file == NULL || write_file == NULL){
        return ENOMEM;
    }

    Mutex_Lock(&s_vfsLock);
    (*read_file)->refCount = 1;
    (*write_file)->refCount = 1;
    Mutex_Unlock(&s_vfsLock);

    // TA (Leo) says 32KB fixed length buffer is OK for this project.
    // Would be better to implement circular buffer.
    newPipe->buffer = Malloc(32768);
    if (newPipe->buffer == NULL)
        return ENOMEM;

    newPipe->readpos = 0;
    newPipe->writepos = 0;

    return 0; 
}

int Pipe_Read(struct File *f, void *buf, ulong_t numBytes) {
    if(((struct Pipe*)(f->fsData))->writepos == 0 || ((struct Pipe*)(f->fsData))->writepos <= ((struct Pipe*)(f->fsData))->readpos){
        if(((struct Pipe*)(f->fsData))->writers){
            return EWOULDBLOCK;
        }
        return 0;
    }

    ulong_t unread = (ulong_t)(((struct Pipe*)(f->fsData))->writepos - ((struct Pipe*)(f->fsData))->readpos);
    if (unread<numBytes)
        numBytes = unread;

    memcpy(buf, (((struct Pipe*)(f->fsData))->buffer) + ((struct Pipe*)(f->fsData))->readpos, numBytes);
    ((struct Pipe*)(f->fsData))->readpos += numBytes;
    return (int)numBytes;

}

int Pipe_Write(struct File *f, void *buf, ulong_t numBytes) {
    if (!((struct Pipe*)(f->fsData))->readers){
        return EPIPE;
    }
    if (((struct Pipe*)(f->fsData))->writepos + numBytes >= 32768){
        return 0;
    }

    memcpy((((struct Pipe*)(f->fsData))->buffer) + ((struct Pipe*)(f->fsData))->writepos, buf, numBytes);
    ((struct Pipe*)(f->fsData))->writepos += numBytes;

    return numBytes;
}

int Pipe_Close(struct File *f) {
    if(f->ops == &Pipe_Read_Ops){                    // Check if called on read end
        ((struct Pipe*)(f->fsData))->readers = 0;
        ((struct Pipe*)(f->fsData))->writepos = 0;
        Free(((struct Pipe*)(f->fsData))->buffer);   // Destroy data if there is no reader but still data
    }

    else if(f->ops == &Pipe_Write_Ops){              // Check if called on write end
        ((struct Pipe*)(f->fsData))->writers = 0;
        if(((struct Pipe*)(f->fsData))->readers == 0)
            ((struct Pipe*)(f->fsData))->writepos = 0;   
    }

    // Destroy pipe if no reader or writer
    if(((struct Pipe*)(f->fsData))->readers == 0 && ((struct Pipe*)(f->fsData))->writers == 0){
        //Free(((struct Pipe*)(f->fsData))->buffer);
        Free(f->fsData);
    }
    return 0; 
}
