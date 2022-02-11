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

#include "sentinel.h"

#include <X11/Shell.h>
#include <Xm/MainW.h>
#include <Xm/ScrolledW.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/PushBG.h>
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/ScrollBar.h>


#define SUPER_HEIGHT	110
#define SUPER_WIDTH	180
#define SUPER_X_OFFSET	5
#define SUPER_Y_OFFSET	20
#define SUPER_ALL_CLEAR	"All clear"
#define SUPER_WARNING	"Something's wrong"

#define TROUBLE_HEIGHT	(28 * num_servers)
#define TROUBLE_WIDTH	856
#define TROUBLE_X_OFFSET	5
#define TROUBLE_Y_OFFSET	5

#define NITPICK_HEIGHT	(28 * num_servers)
#define NITPICK_WIDTH	856
#define NITPICK_X_OFFSET	5
#define NITPICK_Y_OFFSET	5

#define LOG_HEIGHT	400
#define LOG_WIDTH	400
#define LOG_X_OFFSET	500
#define LOG_Y_OFFSET	10

#define PROBE_LABEL	"Force Probe"
#define QUIT_LABEL	"Quit"
#define FORMAT_LABEL	"Format"
#define LABEL_WIDTH	40

struct entry {
    Widget n_widget;
    Widget t_widget;
    int mode;
    char msg[2][BUFLEN];
};

static void make_button();
static Widget make_supervisor();
static Widget make_trouble();
static Widget make_nitpicker();
static Widget make_log();
static Widget make_label();
static void initialize_table();
static void swap_colors();
static void initialize_font();

static int trouble_format = FORMAT_PERCENT;
static int nitpick_format = FORMAT_PERCENT;
static XmStringCharSet charset = (XmStringCharSet) XmSTRING_DEFAULT_CHARSET;
static struct entry **widget_table;
static Widget super_shell;
static Widget trouble_shell;
static Widget nitpick_shell;
static Widget log_shell;
static Widget log_text;
static Widget super_label;
static int log_visible = 0;
static int log_pos = 0;
static Display *dpy;
static char log_geometry[BUFLEN];
static char super_geometry[BUFLEN];
static char nitpick_geometry[BUFLEN];
static char trouble_geometry[BUFLEN];

static Widget current_shell;

static void initialize_font()
{
    app_data.font_list = XmFontListCreate(app_data.font_struct, charset);
}

void create_windows(display)
Display *display;
{
    dpy = display;

    initialize_table();
    initialize_font();
    super_shell = make_supervisor();
    trouble_shell = make_trouble();
    log_shell = make_log();
    nitpick_shell = make_nitpicker();

    XtSetMappedWhenManaged(super_shell, False);
    XtSetMappedWhenManaged(trouble_shell, False);
    XtSetMappedWhenManaged(nitpick_shell, False);
    XtSetMappedWhenManaged(log_shell, False);
    
    XtRealizeWidget(super_shell);
    XtRealizeWidget(trouble_shell);
    XtRealizeWidget(nitpick_shell);
    XtRealizeWidget(log_shell);

    if (!strcmp(app_data.initial_window, SUPER_LABEL))
      current_shell = super_shell;
    else if (!strcmp(app_data.initial_window, TROUBLE_LABEL))
      current_shell = trouble_shell;
    else if (!strcmp(app_data.initial_window, NITPICK_LABEL))
      current_shell = nitpick_shell;
    else
      {
	  fprintf(stderr, "Bad initial window: %s\n", app_data.initial_window);
	  exit(-1);
      }

    XtMapWidget(current_shell);

    if (app_data.log_visible)
      {
	  log_visible = 1;
	  XtMapWidget(log_shell);
      }
}

static void make_button(parent, name, cb)
Widget parent;
char *name;
XtCallbackProc cb;
{
    int ac;
    Arg al[10];
    Widget button;
    XmString label_string;

    label_string = XmStringCreateLtoR(name, charset);

    ac = 0;
    XtSetArg(al[ac], XmNlabelString, label_string); ac++;
    XtSetArg(al[ac], XmNfontList, app_data.font_list); ac++;
    button = XmCreatePushButtonGadget(parent, name, al, ac);

    XtAddCallback(button, XmNactivateCallback, cb, NULL);
    XtManageChild(button);

    XmStringFree(label_string);
}

static Widget make_supervisor()
{
    int ac;
    Arg al[10];
    Widget button_list;
    Widget window;
    Widget form;
    Widget shell;
    XmString label_string;
  
    /* Create supervisor shell */
    if (!app_data.supervisor_geometry)
      {
	  sprintf(super_geometry, "%dx%d+%d+%d",
		  SUPER_WIDTH, SUPER_HEIGHT, SUPER_X_OFFSET, SUPER_Y_OFFSET);
	  app_data.supervisor_geometry = super_geometry;
      }

    ac = 0;
    XtSetArg(al[ac], XmNgeometry, app_data.supervisor_geometry); ac++;
    XtSetArg(al[ac], XmNforeground, app_data.fg); ac++;
    XtSetArg(al[ac], XmNbackground, app_data.bg); ac++;
    XtSetArg(al[ac], XmNborderWidth, app_data.border_width); ac++;
    XtSetArg(al[ac], XmNborderColor, app_data.border_color); ac++;
#if 0
    XtSetArg(al[ac], XmNminHeight, SUPER_HEIGHT); ac++;
    XtSetArg(al[ac], XmNminWidth, SUPER_WIDTH); ac++;
#endif
    shell = XtAppCreateShell(NULL, SUPER_LABEL, applicationShellWidgetClass, dpy, al, ac);

    /* Create supervisor window */
    ac = 0;
    window = XmCreateMainWindow(shell, "window", al, ac);
    XtManageChild(window);

    /* Create a Form for the window */
    ac = 0;
    form = XmCreateForm(window, "form", al, ac);
    XtManageChild(form);

    /* Create buttons for supervisor window */
    ac = 0;
    XtSetArg(al[ac], XmNisHomogeneous, True); ac++;
    XtSetArg(al[ac], XmNentryClass, xmPushButtonGadgetClass); ac++;
    XtSetArg(al[ac], XmNnumColumns, 1); ac++;
    XtSetArg(al[ac], XmNpacking, XmPACK_COLUMN); ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
    button_list = XmCreateRowColumn(form, "row_column", al, ac);
    XtManageChild(button_list);

    make_button(button_list, LOG_LABEL, log_button_cb);
    make_button(button_list, TROUBLE_LABEL, switch_window_cb);
    make_button(button_list, NITPICK_LABEL, switch_window_cb);
#if not_implented
    make_button(button_list, PROBE_LABEL, probe_cb); */
#endif
    make_button(button_list, QUIT_LABEL, quit_cb);

    label_string = XmStringCreateLtoR(SUPER_ALL_CLEAR, charset);

    ac = 0;
    XtSetArg(al[ac], XmNfontList, app_data.font_list); ac++;
    XtSetArg(al[ac], XmNlabelString, label_string); ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(al[ac], XmNleftWidget, button_list); ac++;
    XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
    super_label = XmCreateLabel(form, "label", al, ac);
    XtManageChild(super_label);

    return shell;
}

static Widget make_trouble()
{
    int ac, ii, jj;
    Arg al[10];
    Widget form;
    Widget table;
    Widget button_list;
    Widget window;
    Widget row;
    Widget shell;
  
    /* Create supervisor shell */
    if (!app_data.trouble_geometry)
      {
	  sprintf(trouble_geometry, "%dx%d+%d+%d",
		  TROUBLE_WIDTH, TROUBLE_HEIGHT, TROUBLE_X_OFFSET, TROUBLE_Y_OFFSET);
	  app_data.trouble_geometry = trouble_geometry;
      }

    ac = 0;
    XtSetArg(al[ac], XmNgeometry, app_data.trouble_geometry); ac++;
    XtSetArg(al[ac], XmNforeground, app_data.fg); ac++;
    XtSetArg(al[ac], XmNbackground, app_data.bg); ac++;
    XtSetArg(al[ac], XmNborderWidth, app_data.border_width); ac++;
    XtSetArg(al[ac], XmNborderColor, app_data.border_color); ac++;
#if 0
    XtSetArg(al[ac], XmNminHeight, TROUBLE_HEIGHT); ac++;
    XtSetArg(al[ac], XmNminWidth, TROUBLE_WIDTH); ac++;
#endif
    shell = XtAppCreateShell(NULL, TROUBLE_LABEL, applicationShellWidgetClass, dpy, al, ac);

    ac = 0;
    window = XmCreateMainWindow(shell, "window", al, ac);
    XtManageChild(window);

    /* Create a Form for the window */
    ac = 0;
    form = XmCreateForm(window, "form", al, ac);
    XtManageChild(form);

    /* Create buttons for trouble window */
    ac = 0;
    XtSetArg(al[ac], XmNisHomogeneous, True); ac++;
    XtSetArg(al[ac], XmNentryClass, xmPushButtonGadgetClass); ac++;
    XtSetArg(al[ac], XmNnumColumns, 1); ac++;
    XtSetArg(al[ac], XmNpacking, XmPACK_COLUMN); ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    button_list = XmCreateRowColumn(form, "row_column", al, ac);
    XtManageChild(button_list);

    make_button(button_list, LOG_LABEL, log_button_cb);
    make_button(button_list, SUPER_LABEL, switch_window_cb);
    make_button(button_list, NITPICK_LABEL, switch_window_cb);
#ifdef not_implented
    make_button(button_list, PROBE_LABEL, probe_cb);
#endif
    make_button(button_list, FORMAT_LABEL, format_cb);
    make_button(button_list, QUIT_LABEL, quit_cb);

    /* Create table for displaying problems */
    ac = 0;
    XtSetArg(al[ac], XmNnumColumns, 1); ac++;
    XtSetArg(al[ac], XmNpacking, XmPACK_COLUMN); ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(al[ac], XmNleftWidget, button_list); ac++;
    XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
    table = XmCreateRowColumn(form, "row_column", al, ac);
    XtManageChild(table);

    /* Create table entries */
    for (ii = 0; ii < num_servers; ii++)
    {
	/* Create a Form, and then create labels */
	ac = 0;
	XtSetArg(al[ac], XmNborderWidth, app_data.border_width); ac++;
	XtSetArg(al[ac], XmNnumColumns, 1); ac++;
	XtSetArg(al[ac], XmNpacking, XmPACK_COLUMN); ac++;
	XtSetArg(al[ac], XmNorientation, XmHORIZONTAL); ac++;
	row = XmCreateRowColumn(table, "form", al, ac);
	XtManageChild(row);

	/* Make a label for the name */
	widget_table[ii][0].t_widget = make_label(row, servers[ii]->name);
	strcpy(widget_table[ii][0].msg[FORMAT_PERCENT], servers[ii]->name);
	strcpy(widget_table[ii][0].msg[FORMAT_BLOCKS], servers[ii]->name);

	for (jj = 1; jj < NUM_LABELS; jj++)
	{
	    widget_table[ii][jj].t_widget = make_label(row, NULL_LABEL);
	}
    }

    return shell;
}

static Widget make_nitpicker()
{
    int ac, ii, jj;
    Arg al[10];
    Widget form;
    Widget table;
    Widget button_list;
    Widget window;
    Widget row;
    Widget shell;
  
    /* Create supervisor shell */
    if (!app_data.nitpicker_geometry)
      {
	  sprintf(nitpick_geometry, "%dx%d+%d+%d",
		  NITPICK_WIDTH, NITPICK_HEIGHT, NITPICK_X_OFFSET, NITPICK_Y_OFFSET);
	  app_data.nitpicker_geometry = nitpick_geometry;
      }

    ac = 0;
    XtSetArg(al[ac], XmNgeometry, app_data.nitpicker_geometry); ac++;
    XtSetArg(al[ac], XmNforeground, app_data.fg); ac++;
    XtSetArg(al[ac], XmNbackground, app_data.bg); ac++;
    XtSetArg(al[ac], XmNborderWidth, app_data.border_width); ac++;
    XtSetArg(al[ac], XmNborderColor, app_data.border_color); ac++;
#if 0
    XtSetArg(al[ac], XmNminHeight, NITPICK_HEIGHT); ac++;
    XtSetArg(al[ac], XmNminWidth, NITPICK_WIDTH); ac++;
#endif
    shell = XtAppCreateShell(NULL, NITPICK_LABEL, applicationShellWidgetClass, dpy, al, ac);

    /* Create nitpicker window */
    ac = 0;
    window = XmCreateMainWindow(shell, "window", al, ac);
    XtManageChild(window);

    /* Create a Form for the window */
    ac = 0;
    form = XmCreateForm(window, "form", al, ac);
    XtManageChild(form);

    /* Create buttons for nitpicker window */
    ac = 0;
    XtSetArg(al[ac], XmNisHomogeneous, True); ac++;
    XtSetArg(al[ac], XmNentryClass, xmPushButtonGadgetClass); ac++;
    XtSetArg(al[ac], XmNnumColumns, 1); ac++;
    XtSetArg(al[ac], XmNpacking, XmPACK_COLUMN); ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    button_list = XmCreateRowColumn(form, "row_column", al, ac);
    XtManageChild(button_list);

    make_button(button_list, LOG_LABEL, log_button_cb);
    make_button(button_list, SUPER_LABEL, switch_window_cb);
    make_button(button_list, TROUBLE_LABEL, switch_window_cb);
#ifdef not_implented
    make_button(button_list, PROBE_LABEL, probe_cb);
#endif
    make_button(button_list, FORMAT_LABEL, format_cb);
    make_button(button_list, QUIT_LABEL, quit_cb);

    /* Create table for displaying problems */
    ac = 0;
    XtSetArg(al[ac], XmNnumColumns, 1); ac++;
    XtSetArg(al[ac], XmNpacking, XmPACK_COLUMN); ac++;
    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(al[ac], XmNleftWidget, button_list); ac++;
    XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
    XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
    table = XmCreateRowColumn(form, "row_column", al, ac);
    XtManageChild(table);

    /* Create table entries */
    for (ii = 0; ii < num_servers; ii++)
    {
	/* Create a Form, and then create labels */
	ac = 0;
	XtSetArg(al[ac], XmNborderWidth, app_data.border_width); ac++;
	XtSetArg(al[ac], XmNnumColumns, 1); ac++;
	XtSetArg(al[ac], XmNpacking, XmPACK_COLUMN); ac++;
	XtSetArg(al[ac], XmNorientation, XmHORIZONTAL); ac++;
	row = XmCreateRowColumn(table, "form", al, ac);
	XtManageChild(row);

	/* Make a label for the name*/
	widget_table[ii][0].n_widget = make_label(row, servers[ii]->name);

	for (jj = 1; jj < NUM_LABELS; jj++)
	{
	    widget_table[ii][jj].n_widget = make_label(row, NULL_LABEL);
	}
    }

    return shell;
}

static Widget make_label(parent, string, min_width, max_width)
Widget parent;
char *string;
int min_width;
int max_width;
{
    int ac;
    Arg al[10];
    Widget label;
    XmString label_string;

    label_string = XmStringCreateLtoR(string, charset);

    ac = 0;
    XtSetArg(al[ac], XmNborderWidth, 0); ac++;
    XtSetArg(al[ac], XmNfontList, app_data.font_list); ac++;
    XtSetArg(al[ac], XmNlabelString, label_string); ac++;
    label = XmCreateLabel(parent, string, al, ac);
    XtManageChild(label);

    XmStringFree(label_string);

    return label;
}

static Widget make_log()
{
    Arg al[10];
    int ac;
    Widget window;
    Widget shell;
  
    /* Create supervisor shell */
    if (!app_data.log_geometry)
      {
	  sprintf(log_geometry, "%dx%d+%d+%d",
		  LOG_WIDTH, LOG_HEIGHT, LOG_X_OFFSET, LOG_Y_OFFSET);
	  app_data.log_geometry = log_geometry;
      }

    ac = 0;
    XtSetArg(al[ac], XmNgeometry, app_data.log_geometry); ac++;
    XtSetArg(al[ac], XmNforeground, app_data.fg); ac++;
    XtSetArg(al[ac], XmNbackground, app_data.bg); ac++;
    XtSetArg(al[ac], XmNborderWidth, app_data.border_width); ac++;
    shell = XtAppCreateShell(NULL, LOG_LABEL, applicationShellWidgetClass, dpy, al, ac);

    ac = 0;
    XtSetArg(al[ac], XmNscrollBarPlacement, XmBOTTOM_LEFT); ac++;
    XtSetArg(al[ac], XmNscrollBarDisplayPolicy, XmAS_NEEDED); ac++;
    XtSetArg(al[ac], XmNscrollingPolicy, XmAUTOMATIC); ac++;
    window = XmCreateScrolledWindow(shell, "window", al, ac);
    XtManageChild(window);

    /* the text area */
    ac = 0;
    XtSetArg(al[ac], XmNautoShowCursorPosition, False); ac++;
    XtSetArg(al[ac], XmNeditable, False); ac++;
    XtSetArg(al[ac], XmNeditMode, XmMULTI_LINE_EDIT); ac++;
    XtSetArg(al[ac], XmNcursorPositionVisible, False); ac++;
    XtSetArg(al[ac], XmNheight, LOG_HEIGHT); ac++;
    XtSetArg(al[ac], XmNwidth, LOG_WIDTH); ac++;
    XtSetArg(al[ac], XmNfontList, app_data.font_list); ac++;
    log_text = XmCreateText(window, "text", al, ac);
    XtManageChild(log_text);

    /* notify window about text widget */
    ac = 0;
    XtSetArg(al[ac], XmNworkWindow, log_text); ac++;
    XtSetValues(window, al, ac);

    return shell;
}

void switch_window(w)
Widget w;
{
    Arg al[10];
    int ac;
    XmString new_string;
    char *name;
    Widget new_shell;

    ac = 0;
    XtSetArg(al[ac], XmNlabelString, &new_string); ac++;
    XtGetValues(w, al, ac);
    XmStringGetLtoR(new_string, charset, &name);

    if (!strcmp(name, SUPER_LABEL)) new_shell = super_shell;
    else if (!strcmp(name, TROUBLE_LABEL)) new_shell = trouble_shell;
    else if (!strcmp(name, NITPICK_LABEL)) new_shell = nitpick_shell;
    else
    {
	fprintf(stderr, "Bad name in switch_window.\n");
	exit(-1);
    }
    XtUnmapWidget(current_shell);
    current_shell = new_shell;
    XtMapWidget(new_shell);
}

void change_label(server, field, format, string)
int server, field, format;
char *string;
{
    int ac;
    Arg al[10];
    XmString label_string;
    struct entry *entry;

    entry = widget_table[server] + field;
    strcpy(entry->msg[format], string);

    if (format == nitpick_format)
    {
	label_string = XmStringCreateLtoR(string, charset);

	ac = 0;
	XtSetArg(al[ac], XmNfontList, app_data.font_list); ac++;
	XtSetArg(al[ac], XmNlabelString, label_string); ac++;
	XtSetValues(entry->n_widget, al, ac);
	XmStringFree(label_string);
    }

    if (format == trouble_format)
    {
	switch (entry->mode)
	{
	case MODE_CLEAR:
	    if (field != SERVER_NAME) string = NULL_LABEL;
	    break;

	case MODE_WARN:
	    break;

	default:
	    fprintf(stderr, "Impossible mode in change_label: %d\n", entry->mode);
	    exit(-1);
	}
	label_string = XmStringCreateLtoR(string, charset);

	ac = 0;
	XtSetArg(al[ac], XmNfontList, app_data.font_list); ac++;
	XtSetArg(al[ac], XmNlabelString, label_string); ac++;
	XtSetValues(entry->t_widget, al, ac);

	XmStringFree(label_string);
    }
}

void toggle_log()
{
    if (log_visible)
    {
	XtUnmapWidget(log_shell);
	log_visible = 0;
    }
    else
    {
	XtMapWidget(log_shell);
	log_visible = 1;
    }
}

void append_log(string)
char *string;
{
    XmTextReplace(log_text, log_pos, log_pos, string);
    log_pos += strlen(string);
}

void flip_label(server, field, mode)
int server, field, mode;
{
    Pixel fg, bg;
    Arg al[10];
    int ac;
    struct entry *entry;
    XmString label_string;
    char *string;

    entry = widget_table[server] + field;

    /* Already looks this way */
    if (mode == entry->mode) return;

    entry->mode = mode;

    /* Re-display the existing string in the t_widget */
    switch (mode)
    {
    case MODE_CLEAR:
	if (field != SERVER_NAME)
	{
	    string = NULL_LABEL;
	    break;
	}
    case MODE_WARN:
	string = entry->msg[trouble_format];
	break;
    default:
	fprintf(stderr, "Impossible mode in flip_label: %d\n", mode);
	exit(-1);
    }
    
    label_string = XmStringCreateLtoR(string, charset);

    ac = 0;
    XtSetArg(al[ac], XmNfontList, app_data.font_list); ac++;
    XtSetArg(al[ac], XmNlabelString, label_string); ac++;
    XtSetValues(entry->t_widget, al, ac);

    XmStringFree(label_string);

    swap_colors(entry->n_widget, entry->mode);
    swap_colors(entry->t_widget, entry->mode);
}

static void initialize_table()
{
    int ii, jj;

    widget_table = (struct entry **)malloc(num_servers * sizeof(struct entry *));
    if (!widget_table) MALLOC_ERROR();

    for (ii = 0; ii < num_servers; ii++)
    {
	widget_table[ii] = (struct entry *)malloc(NUM_LABELS * sizeof(struct entry));
	if (!widget_table[ii]) MALLOC_ERROR();
	for (jj = 0; jj < NUM_LABELS; jj++)
	{
	    widget_table[ii][jj].mode = MODE_CLEAR;
	    strcpy(widget_table[ii][jj].msg[FORMAT_PERCENT], NULL_LABEL);
	    strcpy(widget_table[ii][jj].msg[FORMAT_BLOCKS], NULL_LABEL);
	}
    }
}

void switch_format()
{
    int ii, jj;
    int ac;
    Arg al[10];
    XmString label_string;

    if (current_shell == nitpick_shell) {
	nitpick_format = !nitpick_format;
	for (ii = 0; ii < num_servers; ii++) {
	    for (jj = DISK_1; jj <= DISK_10; jj++) {
		label_string = XmStringCreateLtoR(widget_table[ii][jj].msg[nitpick_format], charset);
		ac = 0;
		XtSetArg(al[ac], XmNfontList, app_data.font_list); ac++;
		XtSetArg(al[ac], XmNlabelString, label_string); ac++;
		XtSetValues(widget_table[ii][jj].n_widget, al, ac);
		XmStringFree(label_string);
	    }
	}
    }
    else {
	trouble_format = !trouble_format;
	for (ii = 0; ii < num_servers; ii++) {
	    for (jj = DISK_1; jj <= DISK_10; jj++) {
		if (widget_table[ii][jj].mode == MODE_WARN) {
		    label_string = XmStringCreateLtoR(widget_table[ii][jj].msg[trouble_format], charset);
		    ac = 0;
		    XtSetArg(al[ac], XmNfontList, app_data.font_list); ac++;
		    XtSetArg(al[ac], XmNlabelString, label_string); ac++;
		    XtSetValues(widget_table[ii][jj].t_widget, al, ac);
		    XmStringFree(label_string);
		}
	    }
	}
    }
}

static void swap_colors(widget, mode)
Widget widget;
int mode;
{
    int ac;
    Arg al[10];
    Pixel fg, bg;

    switch (mode)
      {
      case MODE_CLEAR:
	  fg = app_data.fg;
	  bg = app_data.bg;
	  break;

      case MODE_WARN:
	  fg = app_data.t_fg;
	  bg = app_data.t_bg;
	  break;

      default:
	  fprintf(stderr, "Bogus mode in swap_colors: %d\n", mode);
	  exit(-1);
      }

    ac = 0;
    XtSetArg(al[ac], XmNforeground, fg); ac++;
    XtSetArg(al[ac], XmNbackground, bg); ac++;
    XtSetValues(widget, al, ac);
}

void update_supervisor(warn_count, new_trouble)
int warn_count, new_trouble;
{
    char *string;
    int ac;
    Arg al[10];
    XmString label_string;
    static int old_count = 0;

    /* change the label if necessary */
    if ((warn_count && !old_count) || (!warn_count && old_count))
    {
	if (warn_count) string = SUPER_WARNING;
	else string = SUPER_ALL_CLEAR;

	label_string = XmStringCreateLtoR(string, charset);

	ac = 0;
	XtSetArg(al[ac], XmNfontList, app_data.font_list); ac++;
	XtSetArg(al[ac], XmNlabelString, label_string); ac++;
	XtSetValues(super_label, al, ac);
	swap_colors(super_label, warn_count ? MODE_WARN : MODE_CLEAR);
    }

    /* If the current mode is the supervisor, and there's new
       trouble, switch to the trouble mode */
    if (new_trouble && current_shell == super_shell)
    {
	XtUnmapWidget(current_shell);
	current_shell = trouble_shell;
	XtMapWidget(current_shell);
    }

    old_count = warn_count;
}
