#include <stdio.h>

#include "sentinel.h"

AppData app_data;

static XtResource resources[] = {
    {
	XmNfont, XmCFont,
	XmRFontStruct, sizeof(XFontStruct),
	XtOffsetOf(AppData, font_struct),
	XmRString, DEF_FONT,
    },
    {
	XmNforeground, XmCForeground,
	XmRPixel, sizeof(Pixel),
	XtOffsetOf(AppData, fg),
	XmRString, XtDefaultForeground,
    },
    {
	XmNbackground, XmCBackground,
	XmRPixel, sizeof(Pixel),
	XtOffsetOf(AppData, bg),
	XmRString, XtDefaultBackground,
    },
    {
	XmNborderWidth, XmCBorderWidth,
	XmRDimension, sizeof(Dimension),
	XtOffsetOf(AppData, border_width),
	XmRImmediate, (XtPointer) BORDER_WIDTH,
    },
    {
	XmNborderColor, XmCBorderColor,
	XmRPixel, sizeof(Pixel),
	XtOffsetOf(AppData, border_color),
	XmRString, XtDefaultForeground,
    },
    {
	XmNconfigFile, XmCConfigFile,
	XmRString, sizeof(String),
	XtOffsetOf(AppData, config_file),
	XmRString, DEF_CONFIG,
    },
    {
	XmNlogFile, XmCLogFile,
	XmRString, sizeof(String),
	XtOffsetOf(AppData, log_file),
	XmRString, DEF_LOGFILE,
    },
    {
	XmNtroubleForeground, XmCTroubleForeground,
	XmRPixel, sizeof(Pixel),
	XtOffsetOf(AppData, t_fg),
	XmRString, XtDefaultBackground,	/* the default is to just reverse fg and bg */
    },
    {
	XmNtroubleBackground, XmCTroubleBackground,
	XmRPixel, sizeof(Pixel),
	XtOffsetOf(AppData, t_bg),
	XmRString, XtDefaultForeground,	/* the default is to just reverse fg and bg */
    },
    {
	XmNinitialWindow, XmCInitialWindow,
	XmRString, sizeof(String),
	XtOffsetOf(AppData, initial_window),
	XmRString, SUPER_LABEL,
    },
    {
	XmNlogVisible, XmCLogVisible,
	XmRBoolean, sizeof(Boolean),
	XtOffsetOf(AppData, log_visible),
	XmRImmediate, (XtPointer) False,
    },
    {
	XmNtroubleGeometry, XmCTroubleGeometry,
	XmRString, sizeof(String),
	XtOffsetOf(AppData, trouble_geometry),
	XmRString, NULL,
    },
    {
	XmNnitpickerGeometry, XmCNitpickerGeometry,
	XmRString, sizeof(String),
	XtOffsetOf(AppData, nitpicker_geometry),
	XmRString, NULL,
    },
    {
	XmNsupervisorGeometry, XmCSupervisorGeometry,
	XmRString, sizeof(String),
	XtOffsetOf(AppData, supervisor_geometry),
	XmRString, NULL,
    },
    {
	XmNlogGeometry, XmCLogGeometry,
	XmRString, sizeof(String),
	XtOffsetOf(AppData, log_geometry),
	XmRString, NULL,
    },
    {
	XmNprobeDelay, XmCProbeDelay,
	XmRString, sizeof(String),
	XtOffsetOf(AppData, probe_delay),
	XmRString, PROBE_DELAY,
    },
};


static XrmOptionDescRec options[] = {
  {"-config",		"*configFile",		XrmoptionSepArg,	NULL},
  {"-logfile",		"*logFile",		XrmoptionSepArg,	NULL},
  {"-tfg",		"*troubleForeground",	XrmoptionSepArg,	NULL},
  {"-tbg",		"*troubleBackground",	XrmoptionSepArg,	NULL},
  {"-trouble",		"*initialWindow",	XrmoptionNoArg,		TROUBLE_LABEL},
  {"-nitpicker",	"*initialWindow",	XrmoptionNoArg,		NITPICK_LABEL},
  {"-supervisor",	"*initialWindow",	XrmoptionNoArg,		SUPER_LABEL},
  {"-log",		"*logVisible",		XrmoptionNoArg,		"True"},
  {"-tgeometry",	"*troubleGeometry",	XrmoptionSepArg,	NULL},
  {"-sgeometry",	"*supervisorGeometry",	XrmoptionSepArg,	NULL},
  {"-ngeometry",	"*nitpickerGeometry",	XrmoptionSepArg,	NULL},
  {"-lgeometry",	"*logGeometry",		XrmoptionSepArg,	NULL},
  {"-delay",		"*probeDelay",		XrmoptionSepArg,	NULL},
  /* something for specifying blocks free instead of percent full */
};


main(argc, argv)
int argc;
char *argv[];
{
    Widget top_level;
    Display *display;
    XtAppContext app_context;
    int to_probe[2], to_x[2];
    int pid;

    XtToolkitInitialize();
    app_context = XtCreateApplicationContext();

    /* Parse the arguments */
    display = XtOpenDisplay(app_context, NULL, NULL, "Sentinel",
			    options, XtNumber(options), &argc, argv);

    /* top_levl never gets used for anything but determining resources */
    top_level = XtAppCreateShell(NULL, "bogus_shell", applicationShellWidgetClass, display, NULL, 0);
    XtGetApplicationResources(top_level, &app_data, resources, XtNumber(resources), NULL, 0);
    
    /* Get starting data */
    parse_config(app_data.config_file);

    /* Create pipes */
    if (pipe(to_probe) == -1 || pipe(to_x) == -1)
    {
	fputs("Couldn't create pipe.\n", stderr);
	exit(-1);
    }

    /* Fork here */
    pid = fork();
    if (pid < 0)
    {
	fputs("Couldn't fork.\n", stderr);
	exit(-1);
    }
    else if (pid == 0)
    {
	char **av;
	int ii;

	dup2(to_probe[0], 0);
	dup2(to_x[1], 1);

	close(to_probe[0]);
	close(to_probe[1]);
	close(to_x[0]);
	close(to_x[1]);
	close(ii);

	/* Set up the arguments for the prober */
	av = (char **)malloc((num_servers + 3) * sizeof(char *));
	if (!av) MALLOC_ERROR();
	av[0] = "sentinel-prober";
	av[1] = app_data.probe_delay;

	for (ii = 0; ii < num_servers; ii++)
	{
	    av[ii + 2] = servers[ii]->name;
	}

	av[ii + 2] = NULL;
	execv("/usr/local/etc/sentinel-prober", av);
    }
    else
    {
	close(to_probe[0]);
	close(to_x[1]);

	initialize_data();
	create_windows(display);
	open_log_file(app_data.log_file);

	(void)XtAppAddInput(app_context, to_x[0], XtInputReadMask | XtInputExceptMask, receive_data, to_x[0]);

	/* never returns */
	XtAppMainLoop(app_context);
    }
}
