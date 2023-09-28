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
#include <conio.h>
#include <process.h>
#include <sched.h>
#include <sema.h>
#include <string.h>
#include <fileio.h>

int main() {
    int i;
    int read_fd, write_fd, 
    read_fd2, write_fd2, 
    read_fd3, write_fd3, 
    read_fd4, write_fd4, 
    read_fd5, write_fd5;
    int read_bytes, written_bytes, pipe_retval;
    char buf[256];

    /* Print("calling pipe"); */
    pipe_retval = Pipe(&read_fd, &write_fd);
    assert(pipe_retval == 0);

    /* Print("calling pipe"); */
    pipe_retval = Pipe(&read_fd2, &write_fd2);
    assert(pipe_retval == 0);

    /* Print("calling pipe"); */
    pipe_retval = Pipe(&read_fd3, &write_fd3);
    assert(pipe_retval == 0);

    /* Print("calling pipe"); */
    pipe_retval = Pipe(&read_fd4, &write_fd4);
    assert(pipe_retval == 0);

    /* Print("calling pipe"); */
    pipe_retval = Pipe(&read_fd5, &write_fd5);
    assert(pipe_retval == 0);

    for(i = 0; i < 5; i++) {
        written_bytes = Write(write_fd, "beep", 4);
        //Print("wrote %d bytes\n", written_bytes);
        assert(written_bytes == 4);
        read_bytes = Read(read_fd, buf, 256);
        //Print("read %d bytes\n", read_bytes);
        assert(read_bytes == 4);
        //Print("%s\n", buf);
        assert(strncmp(buf, "beep", 4) == 0);
    }

    for(i = 0; i < 5; i++) {
        written_bytes = Write(write_fd2, "hello", 5);
        //Print("wrote %d bytes\n", written_bytes);
        assert(written_bytes == 5);
        read_bytes = Read(read_fd2, buf, 256);
        //Print("read %d bytes\n", read_bytes);
        assert(read_bytes == 5);
        //Print("%s\n", buf);
        assert(strncmp(buf, "hello", 5) == 0);
    }

    for(i = 0; i < 5; i++) {
        written_bytes = Write(write_fd3, "hello again", 11);
        //Print("wrote %d bytes\n", written_bytes);
        assert(written_bytes == 11);
        read_bytes = Read(read_fd3, buf, 256);
        //Print("read %d bytes\n", read_bytes);
        assert(read_bytes == 11);
        //Print("%s\n", buf);
        assert(strncmp(buf, "hello again", 11) == 0);
    }

    for(i = 0; i < 5; i++) {
        written_bytes = Write(write_fd4, "hello", 5);
        //Print("wrote %d bytes\n", written_bytes);
        assert(written_bytes == 5);
        read_bytes = Read(read_fd4, buf, 256);
        //Print("read %d bytes\n", read_bytes);
        assert(read_bytes == 5);
        //Print("%s\n", buf);
        assert(strncmp(buf, "hello", 5) == 0);
    }

    for(i = 0; i < 5; i++) {
        written_bytes = Write(write_fd5, "hello again", 11);
        //Print("wrote %d bytes\n", written_bytes);
        assert(written_bytes == 11);
        read_bytes = Read(read_fd5, buf, 256);
        //Print("read %d bytes\n", read_bytes);
        assert(read_bytes == 11);
        //Print("%s\n", buf);
        assert(strncmp(buf, "hello again", 11) == 0);
    }

    Close(write_fd);

    read_bytes = Read(read_fd, buf, 256);
    assert(read_bytes == 0);

    Close(read_fd);

    written_bytes = Write(write_fd, "beep", 4);
    assert(written_bytes <= -1);
    read_bytes = Read(read_fd, buf, 256);
    assert(read_bytes <= -1);

    Close(write_fd2);

    read_bytes = Read(read_fd2, buf, 256);
    assert(read_bytes == 0);

    Close(read_fd2);

    written_bytes = Write(write_fd2, "hello", 5);
    assert(written_bytes <= -1);
    read_bytes = Read(read_fd2, buf, 256);
    assert(read_bytes <= -1);

    Close(write_fd3);

    read_bytes = Read(read_fd3, buf, 256);
    assert(read_bytes == 0);

    Close(read_fd3);

    written_bytes = Write(write_fd3, "hello again", 11);
    assert(written_bytes <= -1);
    read_bytes = Read(read_fd3, buf, 256);
    assert(read_bytes <= -1);
    return 0;

    Close(write_fd4);

    read_bytes = Read(read_fd4, buf, 256);
    assert(read_bytes == 0);

    Close(read_fd4);

    written_bytes = Write(write_fd4, "hello", 5);
    assert(written_bytes <= -1);
    read_bytes = Read(read_fd4, buf, 256);
    assert(read_bytes <= -1);

    Close(write_fd5);

    read_bytes = Read(read_fd5, buf, 256);
    assert(read_bytes == 0);

    Close(read_fd5);

    written_bytes = Write(write_fd5, "hello again", 11);
    assert(written_bytes <= -1);
    read_bytes = Read(read_fd5, buf, 256);
    assert(read_bytes <= -1);
    return 0;
}
