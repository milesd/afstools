/**************************************************************************
* To: Mike Gahan <ccaamrg@ucl.ac.uk>
* Cc: Info-AFS@transarc.com
* Subject: examining the AFS cache
* Date: Fri, 21 Aug 1992 17:54:48 EDT
* From: John Carr <jfc@Athena.MIT.EDU>
* 
* 
* Here is a program that will list the contents of a workstation's AFS cache.
* This does not work for a memory cache, only a disk cache.  You will need the
* AFS libraries (libubik.a, etc...).
*
* The output looks like this:
*
* ----------------------------------------------------------------
* 3600 cache entries
* Local cell is athena.mit.edu
* index [    cell    :   volume :   vnode]
*    0: [          user.jacutro :      1a2] ino 37124 chunk   0 len    13a
*    1: [          user.carlala :        c] ino 37125 chunk   0 len     10
*    2: [        n.contrib.xpix :        1] ino 37126 chunk   0 len    800
*   3: [            user.blade :       40] ino 37127 chunk   0 len     15
*   4: [          user.lesliet :       30] ino 37128 chunk   0 len    c65
*   5: [         user.macfreak :      692] ino 37129 chunk   0 len    426
*   6: [              user.oye :      344] ino 37130 chunk   0 len    395
*   7: [          user.mallory :       2e] ino 37131 chunk   0 len    d55
*   8: [         user.briarose :      3d6] ino 37132 chunk   0 len   17eb
*   9: [             user.john :        1] ino 37133 chunk   0 len   1000
*  10: [sipb.mit.edu: 2000003b :        1] ino 37134 chunk   0 len   1000
*  11: [         user.rmmargol :        a] ino 37135 chunk   0 len    7f7
*  12: [              user.mjl :       62] ino 37136 chunk   0 len     7b
*  13: [            user.consi :        4] ino 37137 chunk   0 len    71c
*  14: [          user.mgeller :      26a] ino 37138 chunk   0 len    41e
* ----------------------------------------------------------------
* 
* Volumes in the local cell are printed as names, remote volumes are printed
* as cell-name:hex-number.
*
* 
* The "index" field is the cache file name (index 0 = "V0", index 1 = "V1",
* etc.).
*
****************************************************************************
*
* Revs:
*   *   Dan Hamel  changed so that only non-0 vol IDs are reported
****************************************************************************
*/

/* dump_afscache.c */

/*
 * Dump out the contents of the AFS cache.
 * Written by John Carr.
 * Based on a program by Bill Sommerfeld.
 *
 */

#define BSD_COMP

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <afs/stds.h>
#include <fcntl.h>
#include <arpa/inet.h>

#ifdef SHOW_NAMES
#include <ubik.h>
#include <afs/vlserver.h>
#include <afs/vldbint.h>
#include <afs/cellconfig.h>
extern int UV_SetSecurity();
extern int VL_GetEntryByID();
extern struct ubik_client *cstruct;
#endif

/*  Take this out for testing
struct afs_fheader {
    int32 magic;
    int32 firstCSize;
    int32 otherCSize;
    int32 spare;
};
*/
struct afs_fheader {
#define AFS_FHMAGIC         0x7635abaf /* uses version number */
    int32 magic;
#define AFS_CI_VERSION 1
    int32 version;
    int32 firstCSize;
    int32 otherCSize;
};


struct ViceFid {
        u_int32 Volume;
        u_int32 Vnode;
        u_int32 Unique;
};

struct VenusFid {
    int32 Cell;
    struct ViceFid Fid;
};

struct fcache {
    int32 hvNextp;             /* Next in vnode hash table, or freeDCList */
    int32 hcNextp;             /* Next index in [fid, chunk] hash table */
    struct VenusFid fid;        /* Fid for this file */
    int32 modTime;              /* last time this entry was modified */
    hyper versionNo;            /* Associated data version number */
    int32 chunk;               /* Relative chunk number */
    ino64_t inode;               /* Unix inode for this chunk */
    int32 chunkBytes;           /* Num bytes in this chunk */
    char states;               /* Has this chunk been modified? */
};

#define DWriting       8

/* atd -- declare search variables */
u_int32 search_volume = 0;
u_int32 search_vnode = 0;

void get_local_cell(cell, len
#ifdef SHOW_NAMES
                   , adir
#endif
                   )
char *cell;
int len;
#ifdef SHOW_NAMES
      struct afsconf_dir *adir;
#endif
{
  int fd;
#ifdef SHOW_NAMES
  if (adir && afsconf_GetLocalCell(adir, cell, len) == 0)
    return;
#endif
/*  fd = open(AFSCONF_CLIENTNAME "/ThisCell", O_RDONLY); */
  fd = open("/usr/vice/etc/ThisCell", O_RDONLY);
  if (fd == -1)
    {
      strcpy(cell, "<unknown>");
      return;
    }
  if (read(fd, cell, len) <= 0)
    strcpy(cell, "unknown");
  close(fd);
  return;
}

#ifdef SHOW_NAMES

static char remotecellname[MAXCELLCHARS];
static int32 remotecelladdr;

int find_cell(cellinfo, info,
              adir)
struct afsconf_cell *cellinfo;
char *info;
struct afsconf_dir *adir;
{
  int i;

  for (i = 0; i < cellinfo->numServers; i++)
    if (*(int32 *)info == cellinfo->hostAddr[i].sin_addr.s_addr)
      {
        remotecelladdr = *(int32 *)info;
        strcpy(remotecellname, cellinfo->name);
        return 1;
      }
  return 0;
}
#endif

main(argc, argv)
int argc;
char *argv[];
{
#ifdef SHOW_NAMES
  struct afsconf_dir *adir;
#endif
  char localcell[64];
  char cache[128];
  int fd;
  struct fcache *fcp;
  struct stat st;
  int nfcp;
  register int i;
  char *cp;
  int32 code;

  if (argc > 3)
    sprintf(cache, "%s/CacheItems", argv[3]);
  else
    strcpy (cache, "/usr/vice/cache/CacheItems");

  if (argc < 3 ) {
	printf("Incorrect number of parameters \n");
	printf("Usage: FindVFiles volume vnode <optional path to CacheItems>\n");
	exit(1);
  }

  search_volume = atoi(argv[1]);
  search_vnode = atoi(argv[2]);

  printf("Searching For Volume=%d -- Vnode=%d.\n", search_volume, search_vnode);

  fd = open (cache, O_RDONLY, 0);
  if (fd < 0)
    {
      perror(cache);
      return 2;
    }
  fstat (fd, &st);

  cp = malloc (st.st_size);
  if (cp == 0)
    {
      fprintf (stderr, "Can't allocate space to read cache index.\n");
      return 3;
    }
  fcp = (struct fcache *)(cp + sizeof(struct afs_fheader));

  nfcp = (st.st_size-sizeof(struct afs_fheader)) / sizeof (struct fcache);

  printf("%d cache entries\n", nfcp);
  if (read (fd, cp, st.st_size) != st.st_size)
    {
      fprintf (stderr, "Unable to read cache index file.\n");
      return 4;
    }
  close(fd);

#ifdef SHOW_NAMES
  adir = afsconf_Open(AFSCONF_CLIENTNAME);
  get_local_cell(localcell, sizeof(localcell), adir);
#else
  get_local_cell(localcell, sizeof(localcell));
#endif

  printf("Local cell is %s\n", localcell);
#ifdef SHOW_NAMES

  /* args:
       int             unauth
       char *          config dir
       char *          cell name
       int32            server auth
       struct ubik_client **
       int (*)()        security proc
   */
  code = vsu_ClientInit(1, AFSCONF_CLIENTNAME, localcell, 0, &cstruct,
                      UV_SetSecurity);
  if (code != 0)
    {
      fprintf(stderr, "VLDB init failed code %lu\n", code);
      return 1;
    }
#endif

  puts("index [    cell    :   volume :   vnode]");

  for (i = 0; i < nfcp; i++)
    {
      struct in_addr celladdr;
#ifdef SHOW_NAMES
      struct vldbentry atd_vldbentry;
#endif
    /* atd - Do search */
    if ((search_volume == fcp[i].fid.Fid.Volume) &&
        (search_vnode == fcp[i].fid.Fid.Vnode)) {
      if (fcp[i].fid.Cell == 0 || fcp[i].fid.Fid.Volume == 0)
        continue;
#ifdef SHOW_NAMES
      if (fcp[i].fid.Cell == 1
          && ubik_Call(VL_GetEntryByID, cstruct, 0,
                      fcp[i].fid.Fid.Volume, -1, &atd_vldbentry) == 0)
        {
          printf("%4d: [%22s : %8d] ino %lld chunk %3d len %6d%s Time %d States %x DataVersion %d.%d\n",
                i, atd_vldbentry.name, fcp[i].fid.Fid.Vnode, fcp[i].inode,
                fcp[i].chunk, fcp[i].chunkBytes,
                (fcp[i].states) & DWriting ? ", writing" : "", fcp[i].modTime, fcp[i].states, 
		 fcp[i].versionNo.high, fcp[i].versionNo.low);
          continue;
        }
#endif
      if (fcp[i].fid.Cell == 1)
        {
          printf ("%4d: [              %8d : %8d] ino %lld chunk %3d len %6d%s DataVersion %d.%d\n",
                 i, fcp[i].fid.Fid.Volume,
                 fcp[i].fid.Fid.Vnode, fcp[i].inode, fcp[i].chunk,
                 fcp[i].chunkBytes,
                 fcp[i].states & DWriting ? ", writing" : "",
		 fcp[i].versionNo.high, fcp[i].versionNo.low);
          continue;
        }
      /* Look for the cell name */
      if (remotecelladdr != fcp[i].fid.Cell &&
          afsconf_CellApply(adir, find_cell, (char *)&fcp[i].fid.Cell) != 1)
        {
          remotecelladdr = celladdr.s_addr = fcp[i].fid.Cell;
/*          strcpy(remotecellname, inet_ntoa(celladdr));  */
        }
      printf ("%4d: [%12s: %8x : %8x] ino %lld chunk %3d len %6x%s\n",
              i, remotecellname, fcp[i].fid.Fid.Volume,
              fcp[i].fid.Fid.Vnode, fcp[i].inode, fcp[i].chunk,
              fcp[i].chunkBytes,
              fcp[i].states & DWriting ? ", writing" : "");
      } /* end search test */
    }
  return !ferror (stdout);
}


