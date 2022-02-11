/* Here's a program we use to unload tapes from the 8mm drives, which	*/	
/* can be especially useful if the drive happens to be in a jukebox/	*/	
/* robot.								*/	
 
/* ---------								*/
#include <sys/types.h>
#include <sys/scsi.h>
#include <sys/tape.h>
#include <fcntl.h>
#include <stdio.h>
 
struct stop st;                /* For sending REWIND */
struct sc_iocmd scmd;          /* For sending UNLOAD */
 
main(argc, argv)
int argc;
char **argv;
{
    int fd;
 
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <tape dev>\n", argv[1]);
        exit(1);
    }
 
    /* First rewind tape, open w/ open() */
    if ((fd = open(argv[1], O_RDONLY)) == -1) {
        perror("open");
        exit(1);
    }
 
    st.st_count = 1;
    st.st_op = STREW;
 
    if (ioctl(fd, STIOCTOP, &st) == -1) {
        perror("ioctl");
        exit(2);
    }
    close(fd);
 
    /* Now send SCSI UNLOAD, must open w/ openx(SC_DIAGNOSTIC) */
    if ((fd = openx(argv[1], O_RDONLY, 0, SC_DIAGNOSTIC)) == -1) {
        perror("openx");
        exit(3);
    }
 
    scmd.data_length = 0;
    scmd.buffer = NULL;
    scmd.timeout_value = 60;
    scmd.flags = 0;
    scmd.command_length = 6;
    scmd.scsi_cdb[0] = SCSI_UNLOAD;
    scmd.scsi_cdb[1] = scmd.scsi_cdb[2] = scmd.scsi_cdb[3] =
    scmd.scsi_cdb[4] = scmd.scsi_cdb[5] = 0;
 
    if (ioctl(fd, STIOCMD, &scmd) == -1) {
        perror("ioctl(unload)");
        exit(4);
    }
 
    exit(0);
}
