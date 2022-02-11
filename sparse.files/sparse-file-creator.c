/* sparse <filename> <offset>
 * Joe Jackson, 1 Feb 1993
 *
 * Test program to write a sparse file.

 * A file named by the first argument is opened for writing.
 * The file pointer is advanced to the position given by the second argument.
 * A few bytes are written at that position.  The file is closed.

 * If a large enough offset is used, the resulting file will include a
 * "hole".  That is, the initial blank space in the file will not
 * occupy actual disk blocks, but will read as a series of zeroes.

 * Use "ls -l" to see the supposed size of the file.  This should be
 * <offset> plus the size of DATA (16).  Use "du -a <filename>" to see
 * the actual amount of disk space used on a local filesystem.  Use
 * "df /vicepx" to see how much space is used in an AFS partition.

 * What happens if the file is in AFS?  On the first write, the RPC
 * passes the offset and data buffer, so limited space is actually
 * used.  On CM fetch, the entire file is obtained because the CM has
 * no understanding of sparseness.  Subsequent updates by the CM will
 * cause substantial portions of the file to be written back to the
 * fileserver. 

 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#define DATA "Joseph Jackson\n"

extern int errno;

main(argc, argv)
int argc;
char *argv[];
{
    int fd;
    int pos, off;
    char *fn;

    if (argc != 3) {
      fprintf(stderr, "Usage: %s <filename> <offset>\n", argv[0]);
      exit(1);
    } 

    fn = argv[1];
    off = atoi(argv[2]);

    if (unlink(fn) == -1) 
      perror ("Error unlinking");

    fd = open(fn, O_CREAT | O_WRONLY, 0666);

    pos = lseek(fd, off, SEEK_SET);
    if (pos == -1) {
      perror("Error seeking");
      exit(2);
    }
    fprintf(stderr, "Moved file pointer to position %d (0x%x)\n", pos, pos);

    pos = write(fd, DATA, sizeof(DATA));
    if (pos == -1) {
      perror("Error writing");
      exit(3);
    }
    fprintf(stderr, "Successfully wrote %d bytes\n", pos);

    if (close(fd) == -1) {
      perror("Error closing");
      exit(4);
    }
    fprintf(stderr, "Successfully closed the file\n", pos);

    exit(0);
}
