/* Some common definitions for ucsdb */

#ifndef UCSDB_H
#define UCSDB_H

#include "config.h"
#define TEMP_MOUNT	"/afs/." CELLNAME "/temp"
#define STRINGLEN 256

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* Database Server list entry */
typedef struct dbserv_entry {
	char	name[STRINGLEN];	/* full qualified domain name */
	char	ip[STRINGLEN];		/* ip address */
	struct dbserv_entry	*next;	/* next server entry in list */
	} *dbserv_list;

/* CellServDB entry */
typedef struct csdb_entry {
	char	cellname[STRINGLEN];	/* cell name for computers */
	char	description[STRINGLEN];	/* cell name for humans */
	dbserv_list	servers;	/* List of db servers */
	struct csdb_entry	*next;	/* next cell entry in list */
	} *csdb_list;

/* Action list entry */
typedef enum {NOTHING_TO_DO, UPDATE_SERVERS, NEW_CELL, INVALID_CELL} action_type;
typedef struct action_entry {
	action_type	action;
	csdb_list	entry;
	struct action_entry	*next;
	} *action_list;

/* some global flags */
extern unsigned char    no_action, mount, unmount, localauth;

csdb_list	read_csdb(char[]);	/* read CellServDB file into list */
int	write_csdb(csdb_list, char[]);	/* write list into CellServDB file */
csdb_list	merge_csdb(csdb_list, csdb_list);	/* merge two lists */
action_list	build_action_list(csdb_list, csdb_list);
int	same_servers(dbserv_list, dbserv_list);
void	process_actions_list(action_list);
int	is_local_cell(char *cellname);
#endif UCSDB_H
