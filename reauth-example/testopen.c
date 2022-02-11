/* testopen.c
 *  This is the test program that tries to open a file in AFS
 */

#include <stdio.h>
#define TOKEN_LIFE  20

FILE *fd, *log;
int i = 0;
char *testfile = "/afs/transarc.com/usr/hamel/private/test-file";

main() {

   if ((log = fopen("/tmp/logfile", "w")) == NULL) {
      printf("Can't open log file\n");
      exit(-1);
   }
   printf("Opened log\n");

   while (i < TOKEN_LIFE) {
      i++;
      if((fd = fopen(testfile, "w")) == 0)
         fprintf(log, "%3d: couldn't open %s\n",
                 i, testfile);
      else {
         fclose(fd);
         fprintf(log, "%3d min: Open/close OK for %s\n",
                 i, testfile);
      }
   fflush(log);
   sleep(60);
   }
}
