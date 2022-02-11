/* Some useful routines for ucsdb */

#include <stdio.h>
#include <netdb.h>
#include <sys/stat.h>
#include "ucsdb.h"

/* read CellServDB file into list */

csdb_list	read_csdb(filename)
char	filename[STRINGLEN];
{
	FILE	*file = NULL;
	csdb_list	list = NULL, newcell = NULL;
	dbserv_list	newserver = NULL;
	char	*lineptr = NULL, line[STRINGLEN];

	/* open file for reading */
	if ((file = fopen(filename,"r")) == NULL)
	{
		fprintf(stderr, "Can't open '%s' for reading\n", filename);
		exit(-1);
	}

	/* read records from file and put them into list */
		/* read line until EOF */
	while ((lineptr = fgets(line,STRINGLEN,file)) != NULL)
	{
		/* if line is cell entry: create new cell entry */
		if (line[0] == '>')
		{
			if (NULL == (newcell =
			 (csdb_list) malloc(sizeof(struct csdb_entry))))
			{
				fprintf(stderr, "Not enough memory\n");
				exit(-1);
			}
			lineptr = &line[1];
			sscanf(lineptr, "%s", newcell->cellname);
			while ((*lineptr != '#') && (*lineptr != '\n')
				&& (*lineptr != '\n'))
				lineptr++;
			if ((*lineptr == '\n') || (*lineptr == '\0'))
			{
				fprintf(stderr, "Error reading %s\n", filename);
				exit(-1);
			}
			lineptr++;
			strcpy(newcell->description,lineptr);
			lineptr = newcell->description;
			while ((*lineptr != '\n') && (*lineptr != '\0'))
				lineptr++;
			*lineptr = '\0';
			newcell->servers = NULL;
			newcell->next = list;
			list = newcell;
			newcell = NULL;
		}
		else if ((line[0] >= '0') && (line[0] <= '9'))
		{
		/* if line is server entry: add server to cell */
			if (NULL == (newserver =
                         (dbserv_list) malloc(sizeof(struct dbserv_entry))))
                        {
                                fprintf(stderr, "Not enough memory\n");
                                exit(-1);
                        }
			lineptr = &line[0];
			sscanf(lineptr, "%s", newserver->ip);
			while ((*lineptr != '#') && (*lineptr != '\n')
				&& (*lineptr != '\n'))
				lineptr++;
			if ((*lineptr == '\n') || (*lineptr == '\0'))
			{
				fprintf(stderr, "Error reading %s\n", filename);
				exit(-1);
			}
                        lineptr++;
                        strcpy(newserver->name,lineptr);
			lineptr = newserver->name;
			while ((*lineptr != '\n') && (*lineptr != '\0'))
				lineptr++;
			*lineptr = '\0';
			newserver->next = list->servers;
			list->servers = newserver;
			newserver = NULL;
		}
	}

	/* close file and return list */
	fclose(file);

	return(list);
}

/* write list into CellServDB file */

int	write_csdb(list, filename)
csdb_list	list;
char		filename[STRINGLEN];
{
	FILE	*file = NULL;
	csdb_list	rover = list;
	dbserv_list	server = NULL;

	/* open file for writing */
	if ((file = fopen(filename,"w")) == NULL)
	{
		fprintf(stderr, "Can't open '%s' for writing\n", filename);
		exit(-1);
	}

	/* write list to file */
	
	while (rover != NULL)
	{
		fprintf(file, ">%s\t\t#%s\n",
			rover->cellname, rover->description);
		server = rover->servers;
		while (server != NULL)
		{
			fprintf(file, "%s\t\t\t#%s\n",
				server->ip, server->name);
			server = server->next;
		}
		rover = rover->next;
	}

	/* close file and return (1 on success, 0 else) */
	if (fclose(file) == EOF)
	{
		fprintf(stderr, "Can't close '%s'\n", filename);
		exit(-1);
	}

	return(1);
}

/* 
 * merge two lists 
 * if the first list is nonempty, the second list is appended to the first
 */

csdb_list	merge_csdb(first, second)
csdb_list	first, second;
{
	csdb_list rover = first, reference, temp;

	if (rover == NULL) return(second);

	/* check rest of list */
	while (rover->next != NULL)
	{
		reference = second;
		while (reference != NULL)
		{
			if (strcmp(reference->cellname, rover->cellname) == 0)
			{
				fprintf(stderr, "Duplicate entry: Cell %s is in public CellServDB and in private extension file\n", rover->cellname);
				if (same_servers(reference->servers,
						rover->servers))
					fprintf(stderr, "Same entry in both files\n");
				else
					fprintf(stderr, "Conflicting server entries\n");
				/* get rover one entry back (if possible) */
				if (rover == first)
				{
					first = rover->next;
				}
				else
				{
					temp = first;
					while (temp->next != rover)
						temp = temp->next;
					/* skip the offending entry */
					temp->next = rover->next;
				}
				reference = NULL;
			}
			if (reference != NULL) reference = reference->next;
		}
		rover = rover->next;
	}

	/* special case: last element in list */
	reference = second;
	while (reference != NULL)
	{
		if (strcmp(reference->cellname, rover->cellname) == 0)
		{
			fprintf(stderr, "Duplicate entry: Cell %s is in public CellServDB and in private extension file\n", rover->cellname);
			if (same_servers(reference->servers,
					rover->servers))
				fprintf(stderr, "Same entry in both files\n");
			else
				fprintf(stderr, "Conflicting server entries\n");
			/* get rover one entry back (if possible) */
			if (rover == first)
				return(second);
			else
			{
				temp = first;
				while (temp->next != rover)
					temp = temp->next;
				/* skip the offending entry */
				temp->next = second;
				return(first);
			}
		}
		reference = reference->next;
	}

        rover->next = second;
	return(first);
}


/*
 * Compare new and old list and generate list of necessary actions.
 *
 * Local cell will not be removed.
 *
 * Returns list of actions to perform. (NULL if both lists are equal.)
 */
action_list	build_action_list(newlist, previouslist)
csdb_list	newlist, previouslist;
{
	csdb_list	actual_entry, rover;
	action_list	actions = NULL, newaction = NULL;
	unsigned int	found;

	/* process new list against old */
	actual_entry	= newlist;
	while (actual_entry != NULL)
	{
		found = FALSE;
		rover = previouslist;
		while ((rover != NULL) && (!found))
		{
			if (strcmp(actual_entry->cellname,rover->cellname) == 0)
			{
				found = TRUE;
				/* compare servers */
				if (!same_servers(actual_entry->servers,rover->servers))
				{
					if (NULL == (newaction =
						(action_list) malloc(sizeof(struct action_entry))))
					{
						fprintf(stderr, "Not enough memory\n");
						exit(1);
					}
					newaction->action	= UPDATE_SERVERS;
					newaction->entry	= actual_entry;
					newaction->next		= actions;
					actions			= newaction;
				}
			}
		rover = rover->next;
		}
		if (!found) 
		{
			/* new entry */
			if (NULL == (newaction =
				(action_list) malloc(sizeof(struct action_entry))))
			{
				fprintf(stderr, "Not enough memory\n");
				exit(1);
			}
			newaction->action	= NEW_CELL;
			newaction->entry	= actual_entry;
			newaction->next		= actions;
			actions			= newaction;
		}
		actual_entry = actual_entry->next;
	}

	/* process old list against new */
	actual_entry	= previouslist;
	while (actual_entry != NULL)
	{
		found = FALSE;
		rover = newlist;
		while ((rover != NULL) && (!found))
		{
			if (strcmp(actual_entry->cellname,rover->cellname) == 0)
			{
				found = TRUE;
			}
		rover = rover->next;
		}
		if (!found) 
		{
			/* stale entry, delete it? */
			if (is_local_cell((char *) &(actual_entry->cellname)))
			{
				fprintf(stderr, "Attempt to delete local cell ('%s'). Exiting.\n", actual_entry->cellname);
				exit(-1);
			}
			else
			{
				if (NULL == (newaction =
					(action_list) malloc(sizeof(struct action_entry))))
				{
					fprintf(stderr, "Not enough memory\n");
					exit(1);
				}
				newaction->action	= INVALID_CELL;
				newaction->entry	= actual_entry;
				newaction->next		= actions;
				actions			= newaction;
			}
		}
		actual_entry = actual_entry->next;
	}

	return(actions);
}

/*
 * compare two lists of server entries
 *
 * returns TRUE if both lists contain the same servers
 */
int	same_servers(first, second)
dbserv_list	first, second;
{
	dbserv_list	actual, rover;
	unsigned int	found;

	/* compare first against second */
	actual = first;
	while (actual != NULL)
	{
		found = FALSE;
		rover = second;
		while ((rover != NULL) && !found)
		{
			if ((strcmp(actual->name,rover->name) == 0) &&
				(strcmp(actual->ip,rover->ip) == 0))
				found = TRUE;
			rover = rover->next;
		}
		if (!found) return(FALSE);
		actual = actual->next;
	}
	/* compare second against first */
	actual = second;
	while (actual != NULL)
	{
		found = FALSE;
		rover = first;
		while ((rover != NULL) && !found)
		{
			if ((strcmp(actual->name,rover->name) == 0) &&
				(strcmp(actual->ip,rover->ip) == 0))
				found = TRUE;
			rover = rover->next;
		}
		if (!found) return(FALSE);
		actual = actual->next;
	}
	/* lists have the same contents */
	return(TRUE);
}

void    process_actions_list(actions)
action_list	actions;
{
	action_list	rover;
	dbserv_list	server;
	char	commandline[STRINGLEN], serverlist[STRINGLEN];

	if (mount || unmount)
	{
		/* create temporary mountpoint */
		if (mkdir(TEMP_MOUNT, S_IRWXU) != 0)
		{
			fprintf(stderr, "Cannot create temporary directory %s\n",
				TEMP_MOUNT);
			exit(-1);
		}
		/* mount writeable root.afs */
		sprintf(commandline, "%s mkmount -dir %s/root.afs -vol root.afs -rw",
			FS, TEMP_MOUNT);
		system(commandline);
	}
	rover = actions;
	while (rover != NULL)
	{
		switch(rover->action)
		{
		case NOTHING_TO_DO:
			break;
		case UPDATE_SERVERS:
			server = rover->entry->servers;
			fprintf(stderr, "New servers in cell: %s\n",
				rover->entry->cellname);
			serverlist[0] = '\0';
			while (server != NULL)
			{
				if (NULL != gethostbyname(server->name))
					sprintf(serverlist, "%s %s", serverlist,
						server->name);
				else
					sprintf(serverlist, "%s %s", serverlist,
						server->ip);
				server = server->next;
			}
			sprintf(commandline,
				"%s newcell -name %s -servers %s",
				FS, rover->entry->cellname, serverlist);
			if (no_action)
				printf("%s\n", commandline);
			else
				system(commandline);
			break;
		case NEW_CELL:
			server = rover->entry->servers;
			fprintf(stderr, "New cell: %s\n",
				rover->entry->cellname);
                        serverlist[0] = '\0';
                        while (server != NULL)
                        {
				if (NULL != gethostbyname(server->name))
                                	sprintf(serverlist, "%s %s", serverlist,
                                        	server->name);
				else
					sprintf(serverlist, "%s %s", serverlist,
						server->ip);
                                server = server->next;
                        }
                        sprintf(commandline, "%s newcell -name %s -servers %s",
                                FS, rover->entry->cellname, serverlist);
			if (no_action)
                        	printf("%s\n", commandline);
			else
				system(commandline);
			/* mount new cell */
			sprintf(commandline,
				"%s mkmount -dir %s/root.afs/%s -vol root.cell -cell %s -root",
				FS, TEMP_MOUNT, rover->entry->cellname,
				rover->entry->cellname);
			if (no_action)
				printf("%s\n", commandline);
			else if (mount)
				system(commandline);
			break;
		case INVALID_CELL:
			/* not yet implemented */
			fprintf(stderr, "Invalid cell: %s\n",
				rover->entry->cellname);
                        /* remove mont point of cell */
                        sprintf(commandline,
                                "%s rmmount -dir %s/root.afs/%s",
                                FS, TEMP_MOUNT, rover->entry->cellname);
			if (no_action)
                               	printf("%s\n", commandline);
			else if (unmount)
				system(commandline);
			break;
		default:
			fprintf(stderr, "Invalid action in list\n");
			exit(-1);
		}
		rover = rover->next;
	}
	if (mount || unmount)
	{
		/* unmount root.afs */
		sprintf(commandline, "%s rmmount %s/root.afs", FS, TEMP_MOUNT);
		system(commandline);
		/* remove temporary mountpoint */
		if (rmdir(TEMP_MOUNT) != 0)
		{
			fprintf(stderr, "Can't remove temporary directory %s\n",
				TEMP_MOUNT);
			exit(-1);
		}
		/* release root.afs */
		sprintf(commandline, "%s release root.afs", VOS);
		if (localauth)
		{
			sprintf(commandline, "%s -localauth", commandline);
		}
		system(commandline);
		/* checkvolumes */
		sprintf(commandline, "%s checkvolumes", FS);
		system(commandline);
	}
}

int is_local_cell(char *cellname)
{
	FILE	*fd;
	int	index = 0;
	char	command[STRINGLEN], localname[STRINGLEN];

	/* get name of local cell */
	sprintf(command, "%s wscell", FS);
	if ((fd = popen(command, "r")) == NULL)
	{
		fprintf(stderr, "Unable to determine local cell. Terminating.\n");
		exit(-1);
	}
	do {/* nothing */} while ((char) fgetc(fd) != '\'');
	while ((localname[index] = (char) fgetc(fd)) != '\'')
	{
		index++;
	}
	localname[index] = '\0';

	/* compare name of local cell to cellname */
	if (!strcmp(cellname, localname)) return TRUE;
	return FALSE;
}
