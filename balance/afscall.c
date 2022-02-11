/* This file is part of the balancer
 * Author: Dan Lovinger, del+@cmu.edu
 */

/*
 *        Copyright 1993 by Carnegie Mellon University
 * 
 *                    All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of CMU not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * CMU DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * CMU BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */
#include "balance.h"
#include "balutil.h"

#include <sys/param.h>
#include <afs/param.h>
#include <afs/cellconfig.h>
#include <afs/volint.h>
#include <afs/voldefs.h>
#include <afs/volser.h>
#include <afs/vlserver.h>
#include <ubik.h>
#include <rx/rx.h>
#include <stdio.h>
#include <strings.h>

extern int UV_SetSecurity();
extern struct rx_connection *UV_Bind();
extern struct ubik_client *cstruct;

extern char *voldumpdir;

extern VL_SetLock();
extern VL_ReleaseLock();

struct pIDs *get_partitions(server)
struct host *server;
{
    long code = 0;
    struct rx_connection *conn;
    static struct pIDs partAr;

    conn = UV_Bind(server->addr, AFSCONF_VOLUMEPORT);
    code = AFSVolListPartitions(conn, &partAr);
    if (conn) rx_DestroyConnection(conn);
    if (code) {
	fail(FAIL_AFS, code, "get_partitions:", "--- on host %s\n", server->name);
	return NULL;
    }

    return &partAr;
}

struct diskPartition *get_partition_info(server, part)
struct host *server;
long part;
{
    long code = 0;
    struct rx_connection *conn;
    static struct diskPartition partI;
    static char c[8] = "/vicepX";

    if (part < 0 || part > MAXPARTID) return NULL;

    c[6] = 'a' + part; /* gross ... */
    conn = UV_Bind(server->addr, AFSCONF_VOLUMEPORT);
    code = AFSVolPartitionInfo(conn, c, &partI);
    if (conn) rx_DestroyConnection(conn);
    if (code) {
	fail(FAIL_AFS, code, "get_partition_info:", "--- on host %s %s\n", server->name, c);
	return NULL;
    }

    return &partI;
}
enum dumptag { INVALID, VOLNAME, VOLID, SERV, PART, STATUS, BACKUPID, PARENTID, CLONEID,
       INUSE, NEEDSLVG, DESTROYME, VOLTYPE, CREATD, ACCD, UPDD, BKUPD, COPYD,
       FLAGS, DSKUSED, MAXQUOTA, MINQUOTA, FILECOUNT, DAYUSE, WEEKUSE, SPARE
};
static enum dumptag maptag(tag)
  char *tag;
{
  if (!strcmp(tag,"name"))
    return VOLNAME;
  if (!strcmp(tag,"id"))
    return VOLID;
  if (!strcmp(tag,"serv"))
    return SERV;
  if (!strcmp(tag,"part"))
    return PART;
  if (!strcmp(tag,"status"))
    return STATUS;
  if (!strcmp(tag,"backupID"))
    return BACKUPID;
  if (!strcmp(tag,"parentID"))
    return PARENTID;
  if (!strcmp(tag,"cloneID"))
    return CLONEID;
  if (!strcmp(tag,"inUse"))
    return INUSE;
  if (!strcmp(tag,"needsSalvaged"))
    return NEEDSLVG;
  if (!strcmp(tag,"destroyMe"))
    return DESTROYME;
  if (!strcmp(tag,"type"))
    return VOLTYPE;
  if (!strcmp(tag, "creationDate"))
    return CREATD;
  if (!strcmp(tag,"accessDate"))
    return ACCD;
  if (!strcmp(tag,"updateDate"))
    return UPDD;
  if (!strcmp(tag,"backupDate"))
    return BKUPD;
  if (!strcmp(tag,"copyDate"))
    return COPYD;
  if (!strcmp(tag,"flags"))
    return FLAGS;
  if (!strcmp(tag,"diskused"))
    return DSKUSED;
  if (!strcmp(tag,"maxquota"))
    return MAXQUOTA;
  if (!strcmp(tag,"minquota"))
    return MINQUOTA;
  if (!strcmp(tag,"filecount"))
    return FILECOUNT;
  if (!strcmp(tag,"dayUse"))
    return DAYUSE;
  if (!strcmp(tag,"weekUse"))
    return WEEKUSE;
  if (!strncmp(tag, "spare", strlen("spare")))
    return SPARE;
  return INVALID;
}

struct volEntries *get_partition_volumes(server, part)
struct host *server;
long part;
{
    long code = 0;
    struct rx_connection *conn;
    static struct volEntries vols;
    char voldump[MAXPATHLEN];
    struct volintInfo thisvol;
    char linebuf[128];
    char *x, *y,c;
    FILE *f;
    int state;
    int nvolsalloc, volno, lineno;
      
    if (part < 0 || part > MAXPARTID) return NULL;

    if (vols.volEntries_val) free(vols.volEntries_val);
    vols.volEntries_val = (volintInfo *) NULL;
    vols.volEntries_len = 0;
    
    if (voldumpdir) {
      state=0;
      nvolsalloc=0;
      volno=0;
      lineno=0;
      sprintf(voldump,"%s/%s", voldumpdir, server->name);
      f=fopen(voldump, "r");
      if (f == NULL) {
        fail(FAIL_EXIT, "Can't open voldump file for %s\n", server->name);
        return NULL;
      }
#ifdef DEBUG
      printf("Starting %s (looking for /vicep%c)\n", server->name, part+ 'a');
#endif
      while (fgets(linebuf, 127, f)) {
        if (linebuf[strlen(linebuf) - 1] == '\n')
          linebuf[strlen(linebuf) - 1] = '\0';
        else {
          c=0;
          while (c != '\n')
            fread(&c,1,1,f);
        }
        lineno++;
        if (strncmp(linebuf,"BEGIN_OF_ENTRY", strlen("BEGIN_OF_ENTRY"))) {
          if (state == 0)
            continue;
        } else {
          state=1;
            memset(&thisvol,0, sizeof(struct volintInfo));
            volno++;
            continue;
        }
        
        if (!strncmp(linebuf,"END_OF_ENTRY", strlen("END_OF_ENTRY"))) {
          if (state == 1) {
            state=0;
            if (!thisvol.name[0] || thisvol.status == VBUSY) {
#ifdef DEBUG
              fail(FAIL_NORM, "Insufficient data for volume %lu (%d)\n",
                   thisvol.volid, volno);
#endif
              continue;
            }
            if (++vols.volEntries_len > nvolsalloc)
              {
                nvolsalloc+=10;
                vols.volEntries_val=realloc(vols.volEntries_val,nvolsalloc*sizeof(struct volintInfo));
                if (vols.volEntries_val == NULL) {
                  fclose(f);
                  return NULL;
                }
              }
              memcpy(&vols.volEntries_val[vols.volEntries_len-1], &thisvol,
                     sizeof(struct volintInfo));
          }
          continue;
        } else if (state == 2)
          continue;
        x=strtok(linebuf,"\t");
        y=strtok(NULL,"\t");
        if (x == NULL) {
          fail(FAIL_NORM, "missing tag in voldump file (server %s, entry %d, line %d)\n",
               server->name, volno, lineno);
          continue;
        }
        
        if (y == NULL) {
          if (!strcmp(x, "name"))
            continue; /* Busy volume */
          if (thisvol.name[0])
            fail(FAIL_NORM, "No data for tag '%s' volume %s, server %s, entry %d, line %d\n", x,
                 thisvol.name, server->name, volno, lineno);
          else
            fail(FAIL_NORM, "No data for tag '%s' server %s entry %d line %d\n"
                 , x, server->name, volno, lineno );
          continue;
        }
        
        switch(maptag(x)) {
        case VOLNAME:
          strcpy(thisvol.name, y);
          break;
        case VOLID:
          thisvol.volid=atol(y);
          break;
        case SERV:
          break;
        case PART:
          if (y[strlen(y) - 1 ] - 'a' != part)
            state=2;
          break;
        case STATUS:
          if (!strcmp(y, "OK"))
            thisvol.status=VOK;
          else if (!strcmp(y, "BUSY"))
            thisvol.status=VBUSY;
          else if (!strcmp(y, "UNATTACHABLE"))
            thisvol.status=-1;
          else {
            fail(FAIL_EXIT,
                 "Error parsing status field '%s' for volume %s, entry %d, line %d\n",
                 y, thisvol.name, volno, lineno);
            return NULL;
          }
          break;
        case BACKUPID:
          thisvol.backupID=atol(y);
          break;
        case PARENTID:
          thisvol.parentID=atol(y);
          break;
        case CLONEID:
          thisvol.cloneID=atol(y);
          break;
        case INUSE:
          if (*y == 'Y') 
             thisvol.inUse=1;
          else 
             thisvol.inUse=0;
          break;
        case NEEDSLVG:
          if (*y == 'Y') 
             thisvol.needsSalvaged=1;
          else
             thisvol.needsSalvaged=0;
          break;
        case DESTROYME:
          if (*y == 'Y')
             thisvol.destroyMe=0xd3;
          else
             thisvol.destroyMe=0;
          break;
        case VOLTYPE:
          if (!strcmp(y, "RW"))
            thisvol.type=RWVOL;
          else if (!strcmp(y,"RO"))
            thisvol.type=ROVOL;
          else if (!strcmp(y, "BK"))
            thisvol.type=BACKVOL;
          else if (!strcmp(y, "?"))
            thisvol.type=-1;
          else {
            fail(FAIL_EXIT, "Error parsing type field '%s' for volume %s, entry %d, line %d\n",
                 y, thisvol.name, volno, lineno);
            return NULL;
          }
          break;
        case CREATD:
          thisvol.creationDate=atol(y);
          break;
        case ACCD:
          thisvol.accessDate=atol(y);
          break;
        case UPDD:
          thisvol.updateDate=atol(y);
          break;
        case BKUPD:
          thisvol.backupDate=atol(y);
          break;
        case COPYD:
          thisvol.copyDate=atol(y);
          break;
        case FLAGS:
          thisvol.flags=atol(y);
          break;
        case DSKUSED:
          thisvol.size=atoi(y);
          break;
        case MAXQUOTA:
          thisvol.maxquota=atoi(y);
          break;
        case MINQUOTA:
          thisvol.spare0=atol(y);
          break;
        case FILECOUNT:
          thisvol.filecount=atoi(y);
          break;
        case DAYUSE:
          thisvol.dayUse=atoi(y);
          break;
        case WEEKUSE:
          thisvol.spare1=atol(y);
          break;
        case SPARE:
          break;
        default:
          fail(FAIL_EXIT, "Can't parse '%s' in voldump file for host %s (looking for part /vicep%c), entry %d, line %d\n",
               x, server->name, part+'a', volno, lineno);
          return NULL;
        }
      }
      fclose(f);
    } else {
      conn = UV_Bind(server->addr, AFSCONF_VOLUMEPORT);
      code = AFSVolListVolumes(conn, part, 1, &vols); /* 1 means full list */
      if (conn) rx_DestroyConnection(conn);
      if (code) {
	fail(FAIL_AFS, code, "get_partition_volumes:", "--- on host %s /vicep%c\n", 
	     server->name, part + 'a');
	return NULL;
      }
    }
    return &vols;
}


/* initial afs set up ook */

void initialize_afs_guts(atcell)
char *atcell;
{
    long code;

    /* this sets us up with a unauthenticated security object, et. al. */
    if (code = vsu_ClientInit(0, AFSCONF_CLIENTNAME, atcell, useserverauth, &cstruct, UV_SetSecurity))
      fail(FAIL_AFS | FAIL_EXIT, code, "initialize_afs_guts:", "--- for %s\n",
	   (atcell ? atcell : "local cell"));
}


/* boolean according to whether or not the volume is curently locked in the
 * VLDB OR volume is in some odd state (VLDB I/O error, etc - see the progint
 * spec). EXCEPT if lock attempts fail for permission, in which case we consider
 * it unlocked.
 * 
 * This exposes a minor design flaw in AFS - you can't determine if a
 * volume is locked without trying to set it. Feh.
 */

int afs_volislocked(avol)
struct volume *avol;
{
    int code1, code2;
    struct rx_connection *conn;
    static int state = 0;

    /* use ubik_Call to wind across the list of database servers in cstruct
     * for us instead of calling the RPC directly and dealing with down hosts.
     */

    if (state) return 0;

    /* this assumes we only move RW vols. Not hard to change if we do
     * other types.
     */

    /* try to set a delete lock on the volume */
    code1 = ubik_Call(VL_SetLock, cstruct, 0, avol->volid, RWVOL, VLOP_DELETE);
    if (code1 == VL_PERM && state == 0) {
	fail(FAIL_AFS|FAIL_DATE, code1, "set VLDB lock:", "--- for %s\n", avol->name);
	fail(FAIL_NORM, "WARNING: this run cannot determine if volumes are locked.\n");
	fail(FAIL_NORM, "         balance should be run as a fileserver superuser.\n");
	state = 1; /* so we don't say this again */
	return 0;
    }

    /* do all of the funky checks in ReleaseLock */
    if (!code1) 
      code2 = ubik_Call(VL_ReleaseLock, cstruct, 0, avol->volid, RWVOL,
			 LOCKREL_OPCODE|LOCKREL_AFSID|LOCKREL_TIMESTAMP);

    if (code2)
      fail(FAIL_AFS|FAIL_DATE, code2, "release VLDB lock:", 
	   "FAIL: couldn't release test lock on %s, EXAMINE MANUALLY\n",
	   avol->name);

    /* some error happened, possibly including VL_ENTRYLOCKED but in any case
     * it won't be possible to perform operations on this volume - it may as
     * well be locked
     */
    if (code1 || code2)
      return 1;

    /* all cool */
    return 0;
}    
