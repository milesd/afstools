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
#include "sentinel.h"

void log_button_cb(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    toggle_log();
}

void switch_window_cb(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    switch_window(w);
}

void quit_cb(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    close_log_file();
    exit(0);
}

void format_cb(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    switch_format();
}

#ifdef not_implemented
void probe_cb(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    return;
}
#endif

void receive_data(client_data, s, id)
XtPointer client_data;
int *s;
XtInputId *id;
{
    process_data((int)client_data);
}
