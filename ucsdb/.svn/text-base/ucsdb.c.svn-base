/*
 * Program to update CellServDB on an AFS client machine
 */

#include <stdio.h>
#include "ucsdb.h"


/*
 * Program to update CellServDB on an AFS client machine
 *
 * ucsdb [-nmul] <lokal CellServDB> <Transarc's CellServDB> [<extension file>]
 * 
 * OPTIONS
 * -n	no action is performed
 * -m	missing cells are mounted into the AFS tree
 * -u   mountpoints of cells that are no longer in the file are removed
 * -l   use -localauth for volume operations
 * 
 */

main(argc, argv, envp)
int	argc;
char	*argv[], *envp[];
{
	csdb_list	masterlist = NULL, extensionlist = NULL;
	csdb_list	previouslist = NULL, newlist = NULL;
	csdb_list	mountlist = NULL, removelist = NULL, updatelist = NULL;
	action_list	actions_to_process = NULL;
	char		*masterfile, *extensionfile, *previousfile, *newfile;
	unsigned char	options = FALSE;

	/* parse argument list */

	/* verify sane number of arguments */
	if ((argc < 3) || (argc > 5))
	{
		fprintf(stderr, "Usage: %s [-nmul] <lokal CellServDB> <Transarc's CellServDB> [<extension file>]\n",
			argv[0]);
		exit(-1);
	}

	/* parse options */
	if (*argv[1] == '-')
	{
		char	*c;

		options = TRUE;
		c = argv[1];
		while (*(++c) != '\0')
		{
			switch (*c)
			{
			case 'n':
				if (mount || unmount)
                                {
                                        fprintf(stderr, "-mu and -n are mutually exclusive\n");
                                        exit(-1);
                                }
				no_action = TRUE;
				break;
			case 'm':
				if (no_action)
				{
					fprintf(stderr, "-m and -n are mutually exclusive\n");
					exit(-1);
				}
				mount = TRUE;
				break;
			case 'u':
				if (no_action)
				{
					fprintf(stderr, "-u and -n are mutually exclusive\n");
					exit(-1);
				}
				unmount = TRUE;
				break;
			case 'l':
				localauth = TRUE;
				break;
			default:
				fprintf(stderr, "Usage: %s [-nmu] <lokal CellServDB> <Transarc's CellServDB> [<extension file>]\n",
					argv[0]);
				exit(-1);
			}
		}
	}
	else
	{
	options = FALSE;
	}

	/* parse arguments */
	if (options)
	{
		previousfile   	= argv[2];
		masterfile	= argv[3];
		if (argc == 5)
			extensionfile = argv[4];
		else
			extensionfile = NULL;
	}
	else
	{
		previousfile   	= argv[1];
		masterfile	= argv[2];
		if (argc == 4)
			extensionfile = argv[3];
		else
			extensionfile = NULL;
	}
	newfile = previousfile;

	/* Read various CellServDBs */

	previouslist	= read_csdb(previousfile);
	if (extensionfile != NULL) extensionlist = read_csdb(extensionfile);
	masterlist	= read_csdb(masterfile);

	/* Build a new one */
	newlist = merge_csdb(masterlist, extensionlist);

	/* Perform necessary operations */
	if ((actions_to_process=build_action_list(newlist, previouslist)) != NULL)
	{
		/* write new CellServDB */
		if (!no_action) write_csdb(newlist, newfile);
		process_actions_list(actions_to_process);
	}
}
