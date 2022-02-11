/* cp.c  -- file copying (main routines)
   Copyright (C) 1989, 1990, 1991 Free Software Foundation.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Written by Torbjorn Granlund, David MacKenzie, and Jim Meyering.

   Modified for use in package by John G. Myers */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include "node.h"

#ifndef L_INCR
#define L_INCR SEEK_CUR
#endif

/* Copy a regular file from FROM to TO.  Large blocks of zeroes,
   as well as holes in the source file, are made into holes in the
   target file.  (Holes are read as zeroes by the `read' system call.)
   Return 0 if successful, -1 if an error occurred. */

#ifdef NO_HOLES
#define MAKE_HOLES_DEFAULT 0
#else
#define MAKE_HOLES_DEFAULT 1
#endif

int
cp(char *from, char *to)
{
    char *buf = 0;
    int buf_size;
    int target_desc;
    int source_desc;
    int n_read;
    int n_written;
    struct stat sb;
    char *cp;
    int *ip;
    int return_val = 0;
    long n_read_total = 0;
    int last_write_made_hole = 0;
    int make_holes = MAKE_HOLES_DEFAULT;

    if (!silent) message("cp %s %s",from,to);
    if (lazy) return 0;
    
    source_desc = open (from, O_RDONLY);
    if (source_desc < 0) {
	message("open %s; %m",from);
	return -1;
    }

    /* Create the new regular file with small permissions initially,
       to not create a security hole.  */

    target_desc = open (to, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (target_desc < 0) {
	message("open %s; %m",to);
	return_val = -1;
	goto ret2;
    }

    /* Find out the optimal buffer size.  */

    if (fstat (target_desc, &sb)) {
	message("fstat %s; %m",to);
	return_val = -1;
	goto ret;
    }

    buf_size = sb.st_blksize;

    /* Make a buffer with space for a sentinel at the end.  */

    buf = (char *) malloc (buf_size + sizeof (int));
    if (!buf) {
	message("Virtual memory exhausted");
	return_val = -1;
	goto ret;
    }

    for (;;) {
	n_read = read (source_desc, buf, buf_size);
	if (n_read < 0) {
	    message("read %s; %m", from);
	    return_val = -1;
	    goto ret;
	}
	if (n_read == 0)
	  break;

	n_read_total += n_read;

	ip = 0;
	if (make_holes) {
	    buf[n_read] = 1;	/* Sentinel to stop loop.  */

	    /* Find first non-zero *word*, or the word with the sentinel.  */

	    ip = (int *) buf;
	    while (*ip++ == 0)
	      ;

	    /* Find the first non-zero *byte*, or the sentinel.  */

	    cp = (char *) (ip - 1);
	    while (*cp++ == 0)
	      ;

	    /* If we found the sentinel, the whole input block was zero,
	       and we can make a hole.  */

	    if (cp > buf + n_read) {
		/* Make a hole.  */
		if (lseek (target_desc, (off_t) n_read, L_INCR) < 0L) {
		    message ("lseek %s; %m", to);
		    return_val = -1;
		    goto ret;
		}
		last_write_made_hole = 1;
	    }
	    else
	      /* Clear to indicate that a normal write is needed. */
	      ip = 0;
	}
	if (ip == 0) {
	    n_written = write (target_desc, buf, n_read);
	    if (n_written < n_read) {
		message("write %s; %m", to);
		return_val = -1;
		goto ret;
	    }
	    last_write_made_hole = 0;
	}
    }

    /* If the file ends with a `hole', something needs to be written at
       the end.  Otherwise the kernel would truncate the file at the end
       of the last write operation.  */

    if (last_write_made_hole) {
#ifndef FTRUNCATE_MISSING
	/* Write a null character and truncate it again.  */
	if (write (target_desc, "", 1) != 1
	    || ftruncate (target_desc, n_read_total) < 0)
#else
	/* Seek backwards one character and write a null.  */
	if (lseek (target_desc, (off_t) -1, L_INCR) < 0L
	    || write (target_desc, "", 1) != 1)
#endif
	{
	    message("write %s; %m", to);
	    return_val = -1;
	}
    }

 ret:
    close (target_desc);
 ret2:
    close (source_desc);
    if (buf) free(buf);

    return return_val;
}
