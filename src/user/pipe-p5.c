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
    int read_fd, write_fd, 
    read_fd2, write_fd2, 
    read_fd3, write_fd3, 
    read_fd4, write_fd4, 
    read_fd5, write_fd5,
    read_fd6, write_fd6;
    int pipe_retval;

    Print("calling pipe 0");
    pipe_retval = Pipe(&read_fd, &write_fd);
    assert(pipe_retval == 0);

    Print("calling pipe 1");
    pipe_retval = Pipe(&read_fd2, &write_fd2);
    assert(pipe_retval == 0);

    Print("calling pipe 2");
    pipe_retval = Pipe(&read_fd3, &write_fd3);
    assert(pipe_retval == 0);

    Print("calling pipe 3");
    pipe_retval = Pipe(&read_fd4, &write_fd4);
    assert(pipe_retval == 0);

    Print("calling pipe 4");
    pipe_retval = Pipe(&read_fd5, &write_fd5);
    assert(pipe_retval == 0);

    Print("calling pipe 5 (should return error)");
    pipe_retval = Pipe(&read_fd6, &write_fd6);
    //Print("%d\n", pipe_retval);
    assert(pipe_retval == -13);
    
    Print("Test 5 Finished\n");
    return 0;
}
