#ifndef lint
static char sccsid[] = "@(#)dclock.c	3.11h 96/09/30 xlockmore";

#endif

/*-
 * dclock.c - floating digital clock screen for xlock,
 *            the X Window System lockscreen.
 * Copyright (C) 1995 by Michael Stembera <mrbig@fc.net>.
 * Use freely anywhere except on any Microsoft platform.
 * See xlock.c for more copying information.
 * Revision History:
 * 29-Aug-95: Written.
 */

/* Needs to be drawn off screen, so it does not flash. Then use with
   XEraseImage from utils.c  */

#include <time.h>
#include <math.h>
#include "xlock.h"

#define font_height(f) (f->ascent + f->descent)

ModeSpecOpt dclock_opts =
{0, NULL, 0, NULL, NULL};

extern XFontStruct *getFont(Display * display);
extern void XEraseImage(Display * display, Window window, GC gc,
		   int x, int y, int xlast, int ylast, int xsize, int ysize);

typedef struct {
	int         ascent;
	int         color;
	short       height, width;
	char       *str, str1[20], str2[20], str1old[40], str2old[40];
	char       *str1pta, *str2pta, *str1ptb, *str2ptb;
	time_t      timenew, timeold;
	short       maxx, maxy, clockx, clocky;
	short       text_height, text_width, cent_offset;
	short       hour;
	int         done;
	int         pixw, pixh;
	Pixmap      pixmap;
	GC          fgGC, bgGC;
} dclockstruct;

static dclockstruct *dclocks = NULL;

static XFontStruct *mode_font = None;

static void
drawDclock(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	GC          gc = MI_GC(mi);
	dclockstruct *dp = &dclocks[MI_SCREEN(mi)];
	static short xold, yold, dx = 1, dy = 1;
	char       *tmppt;

	xold = dp->clockx;
	yold = dp->clocky;
	dp->clockx += dx;
	dp->clocky += dy;

	if (dp->maxx < dp->cent_offset) {
		if (dp->clockx < dp->maxx + dp->cent_offset ||
		    dp->clockx > dp->cent_offset) {
			dx = -dx;
			dp->clockx += dx;
		}
	} else if (dp->maxx > dp->cent_offset) {
		if (dp->clockx > dp->maxx + dp->cent_offset ||
		    dp->clockx < dp->cent_offset) {
			dx = -dx;
			dp->clockx += dx;
		}
	}
	if (dp->maxy - mode_font->ascent <= 0) {
		if (dp->clocky > mode_font->ascent ||
		    dp->clocky < -mode_font->ascent) {
			dy = -dy;
			dp->clocky += dy;
		}
	} else {
		if (dp->clocky > dp->maxy || dp->clocky < dp->ascent) {
			dy = -dy;
			dp->clocky += dy;
		}
	}
	if (dp->timeold != (dp->timenew = time((long *) NULL))) {
		/* only parse if time has changed */
		dp->timeold = dp->timenew;
		dp->str = ctime(&dp->timeold);

		/* keep last disp time so it can be cleared even if it changed */
		tmppt = dp->str1ptb;
		dp->str1ptb = dp->str1pta;
		dp->str1pta = tmppt;
		tmppt = dp->str2ptb;
		dp->str2ptb = dp->str2pta;
		dp->str2pta = tmppt;

		/* copy the hours portion for 24 to 12 hr conversion */
		(void) strncpy(dp->str1pta, (dp->str + 11), 8);
		dp->hour = (short) (dp->str1pta[0] - 48) * 10 +
			(short) (dp->str1pta[1] - 48);
		if (dp->hour > 12) {
			dp->hour -= 12;
			(void) strcpy(dp->str1pta + 8, " PM");
		} else {
			if (dp->hour == 0)
				dp->hour += 12;
			(void) strcpy(dp->str1pta + 8, " AM");
		}
		dp->str1pta[0] = (dp->hour / 10) + 48;
		dp->str1pta[1] = (dp->hour % 10) + 48;
		if (dp->str1pta[0] == '0')
			dp->str1pta[0] = ' ';

		/* copy day month */
		(void) strncpy(dp->str2pta, dp->str, 11);
		/* copy year */
		(void) strncpy(dp->str2pta + 11, (dp->str + 20), 4);
	}
	if (dp->pixw != dp->text_width || dp->pixh != 2 * dp->text_height) {
		XGCValues   gcv;

		if (dp->fgGC)
			XFreeGC(display, dp->fgGC);
		if (dp->bgGC)
			XFreeGC(display, dp->bgGC);
		if (dp->pixmap) {
			XFreePixmap(display, dp->pixmap);
			XClearWindow(display, window);
		}
		dp->pixw = dp->text_width;
		dp->pixh = 2 * dp->text_height;
		dp->pixmap = XCreatePixmap(display, window, dp->pixw, dp->pixh, 1);
		gcv.font = mode_font->fid;
		gcv.background = 0;
		gcv.foreground = 1;
		dp->fgGC = XCreateGC(display, dp->pixmap,
				 GCForeground | GCBackground | GCFont, &gcv);
		gcv.foreground = 0;
		dp->bgGC = XCreateGC(display, dp->pixmap,
				 GCForeground | GCBackground | GCFont, &gcv);
	}
	XFillRectangle(display, dp->pixmap, dp->bgGC, 0, 0, dp->pixw, dp->pixh);

	XDrawString(display, dp->pixmap, dp->fgGC,
		    dp->cent_offset, mode_font->ascent,
		    dp->str1pta, strlen(dp->str1pta));
	XDrawString(display, dp->pixmap, dp->fgGC,
		    0, mode_font->ascent + dp->text_height,
		    dp->str2pta, strlen(dp->str2pta));
	XSetForeground(display, gc, MI_WIN_BLACK_PIXEL(mi));
	/* This could leave screen dust on the screen if the width changes
	   But that only happens once a day...
	   ... this is solved by the ClearWindow above
	 */
	XEraseImage(display, window, gc,
		dp->clockx - dp->cent_offset, dp->clocky - mode_font->ascent,
		    xold - dp->cent_offset, yold - mode_font->ascent,
		    dp->pixw, dp->pixh);
	if (MI_NPIXELS(mi) > 2)
		XSetForeground(display, gc, MI_PIXEL(mi, dp->color));
	else
		XSetForeground(display, gc, MI_WIN_WHITE_PIXEL(mi));
	XCopyPlane(display, dp->pixmap, window, gc,
		   0, 0, dp->text_width, 2 * dp->text_height,
		dp->clockx - dp->cent_offset, dp->clocky - mode_font->ascent,
		   1L);
}

void
init_dclock(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	dclockstruct *dp;

	if (dclocks == NULL) {
		if ((dclocks = (dclockstruct *) calloc(MI_NUM_SCREENS(mi),
					     sizeof (dclockstruct))) == NULL)
			return;
	}
	dp = &dclocks[MI_SCREEN(mi)];

	dp->width = MI_WIN_WIDTH(mi);
	dp->height = MI_WIN_HEIGHT(mi);

	XClearWindow(display, MI_WINDOW(mi));

	if (mode_font == None)
		mode_font = getFont(display);
	if (!dp->done) {
		dp->done = 1;
		XSetFont(display, MI_GC(mi), mode_font->fid);
	}
	/* (void)time(&dp->timenew); */
	dp->timeold = dp->timenew = time((long *) NULL);
	dp->str = ctime(&dp->timeold);

	(void) strncpy(dp->str1, (dp->str + 11), 8);
	dp->hour = (short) (dp->str1[0] - 48) * 10 + (short) (dp->str1[1] - 48);
	if (dp->hour > 12) {
		dp->hour -= 12;
		(void) strcpy(dp->str1 + 8, " PM");
	} else {
		if (dp->hour == 0)
			dp->hour += 12;
		(void) strcpy(dp->str1 + 8, " AM");
	}
	dp->str1[0] = (dp->hour / 10) + 48;
	dp->str1[1] = (dp->hour % 10) + 48;
	if (dp->str1[0] == '0')
		dp->str1[0] = ' ';
	dp->str1[11] = 0;	/* terminate dp->str1 */
	dp->str1old[11] = 0;	/* terminate dp->str1old */

	(void) strncpy(dp->str2, dp->str, 11);
	(void) strncpy(dp->str2 + 11, (dp->str + 20), 4);
	dp->str2[15] = 0;	/* terminate dp->str2 */
	dp->str2old[15] = 0;	/* terminate dp->str2old */

	dp->ascent = mode_font->ascent;
	dp->text_height = font_height(mode_font);
	dp->text_width = XTextWidth(mode_font, dp->str2, strlen(dp->str2));
	dp->cent_offset = (dp->text_width -
		      XTextWidth(mode_font, dp->str1, strlen(dp->str1))) / 2;
	dp->maxx = dp->width - dp->text_width;
	dp->maxy = dp->height - dp->text_height - mode_font->descent;
	if (dp->maxx == dp->cent_offset)
		dp->clockx = 0;
	else if (dp->maxx < dp->cent_offset)
		dp->clockx = -NRAND(-dp->maxx + dp->cent_offset) + dp->cent_offset;
	else
		dp->clockx = NRAND(dp->maxx - dp->cent_offset) + dp->cent_offset;
	if (dp->maxy - mode_font->ascent <= 0)
		dp->clocky = NRAND(2 * mode_font->ascent) - mode_font->ascent;
	else
		dp->clocky = NRAND(dp->maxy - mode_font->ascent) +
			mode_font->ascent;

	dp->str1pta = dp->str1;
	dp->str2pta = dp->str2;
	dp->str1ptb = dp->str1old;
	dp->str2ptb = dp->str2old;

	dp->color = 0;
}

void
draw_dclock(ModeInfo * mi)
{
	dclockstruct *dp = &dclocks[MI_SCREEN(mi)];

	drawDclock(mi);
	if (MI_NPIXELS(mi) > 2) {
		if (++dp->color >= MI_NPIXELS(mi))
			dp->color = 0;
	}
}

void
release_dclock(ModeInfo * mi)
{
	if (dclocks != NULL) {
		int         screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
			dclockstruct *dp = &dclocks[screen];
			Display    *display = MI_DISPLAY(mi);

			if (dp->fgGC)
				XFreeGC(display, dp->fgGC);
			if (dp->bgGC)
				XFreeGC(display, dp->bgGC);
			if (dp->pixmap)
				XFreePixmap(display, dp->pixmap);

		}
		(void) free((void *) dclocks);
		dclocks = NULL;
	}
	if (mode_font != None) {
		XFreeFont(MI_DISPLAY(mi), mode_font);
		mode_font = None;
	}
}

void
refresh_dclock(ModeInfo * mi)
{
	/* Do nothing, it will refresh by itself */
}
