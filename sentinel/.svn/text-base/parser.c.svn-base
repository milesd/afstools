#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include "sentinel.h"

extern char *malloc();
extern char *realloc();

#define PARSE_ERROR(l) \
{ \
    fprintf(stderr, "Syntax error: line %d\n", l); \
    exit(1); \
}


struct server **servers;
int num_servers = 0;

static void parse_server();
static void parse_disk();
static struct server *merge_sort();
static struct server *merge_lists();
static int hostcmp();

static int current_line = 0;
static FILE *current_file = NULL;

void parse_config(path)
char *path;
{
    int ii;
    int percent_full = -1;
    int blocks_avail = -1;
    int server_percent_full = -1;
    int server_blocks_avail = -1;
    char buf[BUFLEN], keyword[BUFLEN], modifier[BUFLEN], delimiter[BUFLEN];
    int count;
    struct server *new_server;
    struct server *server_list = NULL;
    struct disk *d;

    current_file = fopen(path, "r");
    if (!current_file)
    {
	fprintf(stderr, "Couldn't open config file: %s\n", path);
	exit(1);
    }

    while (fgets(buf, BUFLEN, current_file))
    {
	current_line++;

	count = sscanf(buf, "%s %s %s", keyword, modifier, delimiter);

	/* Skip comments and blank lines */
	if (count <= 0 || keyword[0] == '#') continue;

	if (!strcmp(keyword, "percent-full"))
	{
	    if (count != 2) PARSE_ERROR(current_line);
	    percent_full = atoi(modifier);
	}
	else if (!strcmp(keyword, "blocks-avail"))
	{
	    if (count != 2) PARSE_ERROR(current_line);
	    blocks_avail = atoi(modifier);
	}
	else if (!strcmp(keyword, "server"))
	{
	    if (count < 2 || count > 3) PARSE_ERROR(current_line);

	    server_percent_full = percent_full;
	    server_blocks_avail = blocks_avail;

	    new_server = (struct server *)malloc(sizeof(struct server));
	    if (!new_server) MALLOC_ERROR();

	    strncpy(new_server->name, modifier, MAX_NAME_LEN);
	    new_server->name[MAX_NAME_LEN - 1] = '\0';
	    new_server->warning = 0;
	    new_server->boot_time = 0;
	    new_server->start_time = 0;
	    new_server->disk_count = 0;
	    new_server->disks = NULL;

	    if (count == 3)
	    {
		if (strcmp(delimiter, "{")) PARSE_ERROR(current_line);
		parse_server(new_server, &server_percent_full, &server_blocks_avail);
	    }

	    if (server_percent_full < 0)
	    {
		fprintf(stderr, "Percent-full not set: line %d\n", current_line);
		exit(1);
	    }
	    if (server_blocks_avail < 0)
	    {
		fprintf(stderr, "Blocks-avail not set: line %d\n", current_line);
		exit(1);
	    }

	    d = &new_server->Disk1;
	    for (ii = 0; ii < 10; ii++)
	    {
		d->thresholds.percent_full = server_percent_full;
		d->thresholds.blocks_avail = server_blocks_avail;
		d->warning = 0;
		d->last_blocks_avail = LONG_MAX;
		d->last_percent_full = LONG_MAX;
		d++;
	    }

	    num_servers++;
	    new_server->next = server_list;
	    server_list = new_server;
	}
	else PARSE_ERROR(current_line);
    }

    if (num_servers == 0)
    {
	fprintf(stderr, "Config file contains no servers.\n");
	exit(1);
    }

    server_list = merge_sort(server_list);
    servers = (struct server **)malloc(num_servers * sizeof(struct server *));
    if (!servers) MALLOC_ERROR();

    ii = 0;
    while (server_list)
    {
	servers[ii++] = server_list;
	server_list = server_list->next;
    }
}

static void parse_server(s, percent_full, blocks_avail)
struct server *s;
int *percent_full, *blocks_avail;
{
    char buf[BUFLEN], keyword[BUFLEN], modifier[BUFLEN], delimiter[BUFLEN];
    int count;

    while (fgets(buf, BUFLEN, current_file))
    {
	current_line++;

	count = sscanf(buf, "%s %s %s", keyword, modifier, delimiter);

	/* Skip comments and blank lines */
	if (count == 0 || keyword[0] == '#') continue;

	if (!strcmp(keyword, "}"))
	{
	    if (count != 1) PARSE_ERROR(current_line);
	    return;
	}
	if (!strcmp(keyword, "percent-full"))
	{
	    if (count != 2) PARSE_ERROR(current_line);
	    *percent_full = atoi(modifier);
	}
	else if (!strcmp(keyword, "blocks-avail"))
	{
	    if (count != 2) PARSE_ERROR(current_line);
	    *blocks_avail = atoi(modifier);
	}
	else if (!strcmp(keyword, "disk"))
	{
	    if (count != 3 || strcmp(delimiter, "{")) PARSE_ERROR(current_line);

	    if (!s->disks)
	      s->disks = (struct disk *)malloc(sizeof (struct disk));
	    else
	      s->disks = (struct disk *)realloc((char *)s->disks, (s->disk_count + 1) * sizeof (struct disk));
	    if (!s->disks) MALLOC_ERROR();

	    strncpy(s->disks[s->disk_count].name, modifier, MAX_NAME_LEN);
	    s->disks[s->disk_count].name[MAX_NAME_LEN - 1] = '\0';

	    s->disks[s->disk_count].thresholds.percent_full = *percent_full;
	    s->disks[s->disk_count].thresholds.blocks_avail = *blocks_avail;
	    s->disks[s->disk_count].warning = 0;

	    parse_disk(&s->disks[s->disk_count]);

	    if (s->disks[s->disk_count].thresholds.percent_full < 0)
	    {
		fprintf(stderr, "Percent-full not set: line %d\n", current_line);
		exit(1);
	    }
	    if (s->disks[s->disk_count].thresholds.blocks_avail < 0)
	    {
		fprintf(stderr, "Blocks-avail not set: line %d\n", current_line);
		exit(1);
	    }

	    s->disk_count++;
	}
	else PARSE_ERROR(current_line);
    }

    fprintf(stderr, "Unexpected end of file.\n");
    exit(1);
}

static void parse_disk(d)
struct disk *d;
{
    char buf[BUFLEN], keyword[BUFLEN];
    unsigned int value;
    int count;

    while (fgets(buf, BUFLEN, current_file))
    {
	current_line++;

	count = sscanf(buf, "%s %ud", keyword, &value);

	/* Skip comments and blank lines */
	if (count == 0 || keyword[0] == '#') continue;

	if (!strcmp(keyword, "}"))
	{
	    if (count != 1) PARSE_ERROR(current_line);
	    return;
	}
	if (!strcmp(keyword, "percent-full"))
	{
	    if (count != 2) PARSE_ERROR(current_line);
	    d->thresholds.percent_full = value;
	}
	else if (!strcmp(keyword, "blocks-avail"))
	{
	    if (count != 2) PARSE_ERROR(current_line);
	    d->thresholds.blocks_avail = value;
	}
	else PARSE_ERROR(current_line);
    }

    fprintf(stderr, "Unexpected end of file.\n");
    exit(1);
}

static hostcmp(a, b)
char *a, *b;
{
    int va, vb;

    for (;;)
    {
	while (*a && !isdigit(*a) && (*a == *b))
	{
	    a++;
	    b++;
	}

	if (!isdigit(*a)) return *a - *b;

	va = vb = 0;
	while (isdigit(*a))
	{
	    va = 10 * va + (*a - '0');
	    a++;
	}
	while (isdigit(*b))
	{
	    vb = 10 * vb + (*b - '0');
	    b++;
	}
	if (va != vb) return va - vb;
    }
}

static struct server *merge_sort(s)
struct server *s;
{
    struct server *p, *n;

    if (!s->next) return s;
    n = s->next;
    p = s;

    while (n)
    {
	if (n->next)
	{
	    n = n->next->next;
	    p = p->next;
	}
	else n = NULL;
    }
    n = p->next;
    p->next = NULL;
    return merge_lists(merge_sort(s), merge_sort(n));
}

static struct server *merge_lists(a, b)
struct server *a, *b;
{
    struct server *new_head, *p;

    if (hostcmp(a->name, b->name) < 0)
    {
	new_head = a;
	a = a->next;
    }
    else
    {
	new_head = b;
	b = b->next;
    }

    p = new_head;

    while (a && b)
    {
	if (hostcmp(a->name, b->name) < 0)
	{
	    p->next = a;
	    a = a->next;
	}
	else
	{
	    p->next = b;
	    b = b->next;
	}
	p = p->next;
    }
    if (a) p->next = a;
    else p->next = b;

    return new_head;
}
