
/***********************************************************
        Copyright 1991 by Carnegie Mellon University

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of CMU not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

CMU DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
CMU BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.
******************************************************************/



#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <afs/fsprobe.h>
#include "sentinel.h"

#define WRITE_LOG(s) { fputs(s, log_file); append_log(s); fflush(log_file);}
		       
extern void free();

static void initialize_disks();
static void check_disk();
static void shut_down();
static void read_pipe();
static void process_server();
static void get_time();

static FILE *log_file = NULL;
static char now[26];
static struct fsprobe_ProbeResults results;

static int warn_count = 0;
static int new_trouble = 0;

void open_log_file(path)
char *path;
{
    char msg[BUFLEN];
    char st[26];

    log_file = fopen(path, "a");
    if (!log_file)
    {
	fprintf(stderr, "Couldn't open log file: %s\n", path);
	exit(1);
    }
    get_time(st, (long)time((long *)NULL));
    sprintf(msg, "Sentinel started at %s.\n", st);
    WRITE_LOG(msg);
}
 
void initialize_data()
{
    int ii;
    char *cp;

    /* truncate the host names */
    for (ii = 0; ii < num_servers; ii++)
    {
	cp = index(servers[ii]->name, '.');
	if (cp) *cp = '\0';
    }

    results.stats = (struct ViceStatistics *)malloc(sizeof(struct ViceStatistics) * num_servers);
    results.probeOK = (int *)malloc(sizeof(int) * num_servers);

    if (!results.stats || !results.probeOK)
    {
	fputs("Malloc failed in initialize_data().\n", stderr);
	exit(-1);
    }
}

static void shut_down(m)
char *m;
{
    char st[26];

    if (log_file)
      {
	  get_time(st, (long)time((long *)NULL));
	  fprintf(log_file, "Sentinel shut down at %s\n.", st);
	  (void)fclose(log_file);
      }
    fprintf(stderr, m);
    exit(-1);
}

void close_log_file()
{
    char st[26];

    if (log_file)
    {
	get_time(st, (long)time((long *)NULL));
	fprintf(log_file, "Sentinel shut down at %s\n.", st);
	(void)fclose(log_file);
      }
}

static void read_pipe(p_in)
int p_in;
{
    int num_read, num_left, ii;

    num_left = sizeof(results.probeNum);
    num_read = 0;
    while (num_left)
    {
	ii = read(p_in, (char *)&results.probeNum + num_read, num_left);
	if (!ii) shut_down("Couldn't read pipe\n.");
	num_left -= ii;
	num_read += ii;
    }

    num_left = sizeof(results.probeTime);
    num_read = 0;
    while (num_left)
    {
	ii = read(p_in, (char *)&results.probeTime + num_read, num_left);
	if (!ii) shut_down("Couldn't read pipe\n.");
	num_left -= ii;
	num_read += ii;
    }

    num_left = sizeof(results.stats[0]) * num_servers;
    num_read = 0;
    while (num_left)
    {
	ii = read(p_in, (char *)results.stats + num_read, num_left);
	if (!ii) shut_down("Couldn't read pipe\n.");
	num_left -= ii;
	num_read += ii;
    }

    num_left = sizeof(results.probeOK[0]) * num_servers;
    num_read = 0;
    while (num_left)
    {
	ii = read(p_in, (char *)results.probeOK + num_read, num_left);
	if (!ii) shut_down("Couldn't read pipe\n.");
	num_left -= ii;
	num_read += ii;
    }
}
 
void process_data(p_in)
int p_in;
{
    int ii;

    read_pipe(p_in);

    get_time(now, (long)time((long *)NULL));

    for (ii = 0; ii < num_servers; ii++)
    {
	process_server(ii);
    }
    update_supervisor(warn_count, new_trouble);
    new_trouble = 0;
}

static void process_server(s_num)
int s_num;
{
    struct server *s;
    struct disk *d;
    struct ViceStatistics *stats;
    ViceDisk *d_stats;
    int ii;
    char log_string[BUFLEN];

    s = servers[s_num];
    if (results.probeOK[s_num])
    {
	/* server is down */

	/* Was it down last time? */
	if (!(s->warning & SW_DOWN))
	{
	    s->warning ^= SW_DOWN;
	    sprintf(log_string, "%s not responding. (%s)\n", s->name, now);
	    WRITE_LOG(log_string);

	    /* Clear the disk data */
	    for (ii = SERVER_NAME + 1; ii <= DISK_10; ii++) {
		change_label(s_num, ii, FORMAT_PERCENT, NULL_LABEL);
		change_label(s_num, ii, FORMAT_BLOCKS, NULL_LABEL);
	    }

	    for (ii = 0; ii < NUM_LABELS; ii++)
	      flip_label(s_num, ii, MODE_WARN);

	    warn_count++;
	    new_trouble++;

	    /* Clear the disk warnings */
	    d = &s->Disk1;
	    for (ii = 0; ii < 10; ii++)
	    {
		if (d->warning & DW_BLOCKS_AVAIL) warn_count--;
		if (d->warning & DW_PERCENT_FULL) warn_count--;
		d->warning = 0;
		d->last_blocks_avail = LONG_MAX;
		d->last_percent_full = LONG_MAX;
		d++;
	    }
	}
    }
    else
    {
	/* server is up */
	stats = &results.stats[s_num];

	/* Was it down last time? */
	if (s->warning & SW_DOWN)
	{
	    char tbuf[26];

	    s->warning ^= SW_DOWN;
	    sprintf(log_string, "%s responding. (%s)\n", s->name, now);
	    WRITE_LOG(log_string);
	    for (ii = 0; ii < NUM_LABELS; ii++)
	      flip_label(s_num, ii, MODE_CLEAR);
	    warn_count--;

	    /* Put the times back */
	    get_time(tbuf, (long) stats->BootTime);
	    change_label(s_num, BOOT_TIME, FORMAT_PERCENT, tbuf);
	    change_label(s_num, BOOT_TIME, FORMAT_BLOCKS, tbuf);

	    get_time(tbuf, (long) stats->StartTime);
	    change_label(s_num, START_TIME, FORMAT_PERCENT, tbuf);
	    change_label(s_num, START_TIME, FORMAT_BLOCKS, tbuf);
	}

	/* Have we initialized the disk names yet? */
	if (s->disks)
	  initialize_disks(stats, s);

	/* Check the boot time */
	if (stats->BootTime != s->boot_time)
	{
	    char boot[26];
	    get_time(boot, (long) stats->BootTime);

	    /* Update the display */
	    change_label(s_num, BOOT_TIME, FORMAT_PERCENT, boot);
	    change_label(s_num, BOOT_TIME, FORMAT_BLOCKS, boot);
#if 0
	    /* this isn't in use because the boot time on the servers drift <sigh> */
	    if (s->boot_time)
	    {
		fprintf(stderr, "Old boot time: %ld %ld\n", s->boot_time, stats->BootTime);
		sprintf(log_string, "%s rebooted at %s. (%s)\n", s->name, boot, now);
		WRITE_LOG(log_string);
	    }
#endif
	    s->boot_time = stats->BootTime;
	}

	/* Check the start time */
	if (stats->StartTime != s->start_time)
	{
	    char start[26];

	    get_time(start, (long)stats->StartTime);

	    /* Update the display */
	    change_label(s_num, START_TIME, FORMAT_PERCENT, start);
	    change_label(s_num, START_TIME, FORMAT_BLOCKS, start);
	    if (s->start_time)
	    {
		sprintf(log_string, "%s restarted at %s. (%s)\n", s->name, start, now);
		WRITE_LOG(log_string);
	    }
	    s->start_time = stats->StartTime;
	}

	/* Check the thresholds */
	d = &s->Disk1;
	d_stats = &stats->Disk1;
	for (ii = 0; ii < 10; ii++)
	{
	    check_disk(s_num, ii + DISK_1, s->name, d_stats, d);
	    d_stats++;
	    d++;
	}
    }
}


static void check_disk(s_num, d_num, s_name, data, disk)
int d_num;
char *s_name;
ViceDisk *data;
struct disk *disk;
{
    int percent_full;
    int blocks_avail;
    int percent_update = 0;
    int blocks_update = 0;
    int ii;
    char msg[BUFLEN];
    char log_string[BUFLEN];

    if (!data->Name[0]) return;

    /* Check percent full */
    percent_full = 100 * (data->TotalBlocks - data->BlocksAvailable) / data->TotalBlocks;

    if (percent_full != disk->last_percent_full) percent_update = 1;
    if (data->BlocksAvailable != disk->last_blocks_avail) blocks_update = 1;

    if (percent_full >= disk->thresholds.percent_full)
    {
	if (!(disk->warning & DW_PERCENT_FULL))
	{
	    /* Disk is now too full */
	    disk->warning ^= DW_PERCENT_FULL;
	    sprintf(log_string, "%s partition %s over %d%% full. (%s)\n",
		    s_name, data->Name, disk->thresholds.percent_full, now);
	    WRITE_LOG(log_string);
	    flip_label(s_num, d_num, MODE_WARN);
	    warn_count++;
	    new_trouble++;
	}
    }
    else
    {
	if (disk->warning & DW_PERCENT_FULL)
	{
	    /* Disk is no longer too full */
	    disk->warning ^= DW_PERCENT_FULL;
	    sprintf(log_string, "%s partions %s no longer over %d%% full. (%s)\n",
		    s_name, data->Name, disk->thresholds.percent_full, now);
	    WRITE_LOG(log_string);
	    if (!(disk->warning & DW)) flip_label(s_num, d_num, MODE_CLEAR);
	    warn_count--;
	}
    }

    /* Check blocks available */
    if (data->BlocksAvailable < disk->thresholds.blocks_avail)
    {
	if (!(disk->warning & DW_BLOCKS_AVAIL))
	{
	    /* Disk has too few blocks free */
	    disk->warning ^= DW_BLOCKS_AVAIL;
	    sprintf(log_string, "%s partition %s has less than %ld blocks free. (%s)\n",
		    s_name, data->Name, disk->thresholds.blocks_avail, now);
	    WRITE_LOG(log_string);
	    flip_label(s_num, d_num, MODE_WARN);
	    warn_count++;
	    new_trouble++;
	}
    }
    else
    {
	if (disk->warning & DW_BLOCKS_AVAIL)
	{
	    /* Disk has enough free blocks now */
	    disk->warning ^= DW_BLOCKS_AVAIL;
	    sprintf(log_string, "%s partition %s has at least %ld blocks free. (%s)\n",
		    s_name, data->Name, disk->thresholds.blocks_avail, now);
	    WRITE_LOG(log_string);
	    if (!(disk->warning & DW)) flip_label(s_num, d_num, MODE_CLEAR);
	    warn_count--;
	}
    }

    disk->last_percent_full = percent_full;
    disk->last_blocks_avail = data->BlocksAvailable;

    if (percent_update)
    {
	sprintf(msg, "    %s: %3d%%", &data->Name[6], disk->last_percent_full);
	change_label(s_num, d_num, FORMAT_PERCENT, msg);
    }
    if (blocks_update)
    {
	sprintf(msg, "  %s: %l6d", &data->Name[6], disk->last_blocks_avail);
	change_label(s_num, d_num, FORMAT_BLOCKS, msg);
    }
}

static void initialize_disks(stats, s)
ViceStatistics *stats;
struct server *s;
{
    int bogus = 0;
    int ii, jj;
    ViceDisk *d_stats;

    for (jj = 0; jj < s->disk_count; jj++)
    {
	d_stats = &stats->Disk1;
	for (ii = 0; ii < 10; ii++)
	{
	    if (!strcmp(d_stats->Name, s->disks[jj].name))
	    {
		bcopy(&s->disks[jj], &s->Disk1 + ii, sizeof(struct thresholds));
		s->disks[jj].name[0] = '\0';
		break;
	    }
	    d_stats++;
	}
    }

    for (jj = 0; jj < s->disk_count; jj++)
    {
	if (s->disks[jj].name[0])
	{
	    fprintf(stderr, "Bad partition name: server %s: partition %s\n", s->name, s->disks[jj].name);
	    bogus = 1;
	}
	if (bogus) exit(-1);
    }

    if (s->disks) free(s->disks);
    s->disks = NULL;
}

/* This is completely dependent on the format for ctime() */
static void get_time(s, t)
char *s;
long t;
{
    int ii, jj;

    strcpy(s, ctime(&t));

    ii = 0;
    jj = 4;
    while (jj < 19)
      s[ii++] = s[jj++];
    s[12] = '\0';
}
