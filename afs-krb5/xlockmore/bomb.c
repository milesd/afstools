
#ifndef lint
static char sccsid[] = "@(#)bomb.c	3.11h 96/09/30 xlockmore";

#endif
/* 
 * bomb.c - temporary screen lock
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 09-Jan-95: Assorted defines to control various aspects of bomb mode.
 *            Uncomment, or otherwise define the appropriate line
 *            to obtain the relevant behaviour, thanks to Dave Shield
 *            <D.T.Shield@csc.liv.ac.uk>.
 * 20-Dec-94: Time patch for multiprocessor machines (ie. Sun10) thanks to
 *            Nicolas Pioch <pioch@Email.ENST.Fr>.
 * 1994:      Written.  Copyright (c) 1994 Dave Shield
 *            Liverpool Computer Science
 */

/*-
 * This mode may have limited appeal.  Its good for logging yourself out
 * if you do not know if your going to be back.  It is no longer to be used
 * as a way of forcing users in a lab to logout after locking the screen.
 */

#ifdef USE_BOMB

#include "xlock.h"

#include <sys/signal.h>
#ifdef SYSLOG
#include <syslog.h>
#endif
#include <stdio.h>

#if 0
#define SIMPLE_COUNTDOWN	/* Display a simple integer countdown,     */
#endif
			      /*  rather than a "MIN:SEC" format.        */
#define COLOUR_CHANGE		/* Display cycles through the colour wheel */
			      /*  rather than staying red throughout.    */

#define FULL_COUNT_FONT         "-*-*-*-*-*-*-34-*-*-*-*-*-*-*"
#define ICON_COUNT_FONT         "-*-*-*-*-*-*-8-*-*-*-*-*-*-*"
#define COUNTDOWN       600	/* No. seconds to lock for */
#define NDIGITS         4	/* Number of digits in count */

#define MAX_DELAY       1000000	/* Max delay between updates */
#define NAP_TIME        5	/* Sleep between shutdown attempts */
#define DELTA           10	/* Border around the digits */
#define RIVET_RADIUS    6	/* Size of detonator 'rivets' */

ModeSpecOpt bomb_opts =
{0, NULL, 0, NULL, NULL};

typedef struct {
	int         width, height;
	int         x, y;
	XPoint      loc;
	int         delta;
	int         color;
	int         countdown;
	int         startcountdown;
	int         text_width;
	int         text_ascent;
	int         text_descent;
	int         moveok;
} bombstruct;

static bombstruct *bombs = NULL;

	/*
	 *  Game Over
	 */
static void
explode(ModeInfo * mi)
{
	bombstruct *bp = &bombs[MI_SCREEN(mi)];
	char        buff[NDIGITS + 2];
	extern void logoutUser(void);

	/*
	 *  ToDo:
	 *      Improve the graphics - some sort of explosion?
	 *      (Will need to involve the main X event loop)
	 */
#ifdef SIMPLE_COUNTDOWN
	(void) sprintf(buff, "%.*s", NDIGITS, "*********");
#else
	(void) sprintf(buff, "%.*s:**", NDIGITS - 2, "*******");
#endif
	XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_PIXEL(mi, 1));
	XDrawString(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
		    bp->x, bp->y, buff, NDIGITS);

	(void) fprintf(stderr, "BOOM!!!!\n");
#ifndef DEBUG
	if (MI_WIN_IS_INWINDOW(mi) || MI_WIN_IS_INROOT(mi) ||
	    MI_WIN_IS_NOLOCK(mi) || MI_WIN_IS_DEBUG(mi))
		(void) kill(getpid(), SIGTERM);
	else if (getuid() == 0) {	/* Do not try to logout root! */
		bp->countdown = 0;
		init_bomb(mi);
	} else
		logoutUser();
#else
	(void) kill(getpid(), SIGTERM);
#endif
}

static void
rivet(ModeInfo * mi, int x, int y)
{
	Display    *display = MI_DISPLAY(mi);
	GC          gc = MI_GC(mi);

	XSetForeground(display, gc, MI_WIN_BLACK_PIXEL(mi));
	if (MI_NPIXELS(mi) > 2) {
		XDrawArc(display, MI_WINDOW(mi), gc, x, y,
		      2 * RIVET_RADIUS, 2 * RIVET_RADIUS, 270 * 64, 90 * 64);
		XSetForeground(display, gc, MI_WIN_WHITE_PIXEL(mi));
		XDrawArc(display, MI_WINDOW(mi), gc, x, y,
		      2 * RIVET_RADIUS, 2 * RIVET_RADIUS, 70 * 64, 130 * 64);
	} else
		XDrawArc(display, MI_WINDOW(mi), gc, x, y,
			 2 * RIVET_RADIUS, 2 * RIVET_RADIUS, 0, 360 * 64);
}

static void
detonator(ModeInfo * mi, int draw)
{
	Display    *display = MI_DISPLAY(mi);
	GC          gc = MI_GC(mi);
	bombstruct *bp = &bombs[MI_SCREEN(mi)];
	int         b_width, b_height;

	b_width = bp->width / 2;
	b_height = bp->height / 3;
	if (draw) {
		if (MI_NPIXELS(mi) > 2)
			XSetForeground(MI_DISPLAY(mi), MI_GC(mi),
				       MI_PIXEL(mi, bp->color));
		else
			XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_WIN_WHITE_PIXEL(mi));
		/*XSetForeground(display, gc, allocPixel(display, XDefaultColormap(display,
		   MI_SCREEN(mi)), "grey", "white")); */
		XFillRectangle(display, MI_WINDOW(mi), gc,
			       bp->loc.x, bp->loc.y, b_width, b_height);

		/*
		 *  If a full size screen (and colour),
		 *      'rivet' the box to it
		 */
		if (bp->width > 100 && bp->height > 100) {
			rivet(mi, bp->loc.x + RIVET_RADIUS, bp->loc.y + RIVET_RADIUS);
			rivet(mi, bp->loc.x + RIVET_RADIUS,
			      bp->loc.y + b_height - 3 * RIVET_RADIUS);
			rivet(mi, bp->loc.x + b_width - 3 * RIVET_RADIUS,
			      bp->loc.y + RIVET_RADIUS);
			rivet(mi, bp->loc.x + b_width - 3 * RIVET_RADIUS,
			      bp->loc.y + b_height - 3 * RIVET_RADIUS);
		}
	} else {
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_WIN_BLACK_PIXEL(mi));
		XFillRectangle(display, MI_WINDOW(mi), gc,
			       bp->loc.x, bp->loc.y, b_width, b_height);
	}
}

void
init_bomb(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	GC          gc = MI_GC(mi);
	bombstruct *bp;
	XFontStruct *c_font;
	char        number[NDIGITS + 2];

	if (bombs == NULL) {
		if ((bombs = (bombstruct *) calloc(MI_NUM_SCREENS(mi),
					       sizeof (bombstruct))) == NULL)
			return;
	}
	bp = &bombs[MI_SCREEN(mi)];

	bp->width = MI_WIN_WIDTH(mi);
	bp->height = MI_WIN_HEIGHT(mi);

	/* Set up text font */
	if (bp->width > 100) {	/* Full screen */
		c_font = XLoadQueryFont(display, FULL_COUNT_FONT);
		bp->delta = DELTA;
	} else {		/* icon window */
		c_font = XLoadQueryFont(display, ICON_COUNT_FONT);
		bp->delta = 2;
	}
	XSetFont(display, gc, c_font->fid);

#ifdef SIMPLE_COUNTDOWN
	(void) sprintf(number, "%0*d", NDIGITS, 0);
#else
	(void) sprintf(number, "%.*s:**", NDIGITS - 2, "*******");
#endif
	bp->text_width = XTextWidth(c_font, number, NDIGITS + 1);
	bp->text_ascent = c_font->max_bounds.ascent;
	bp->text_descent = c_font->max_bounds.descent;

	if (MI_DELAY(mi) > MAX_DELAY)
		MI_DELAY(mi) = MAX_DELAY;	/* Time cannot move slowly */
	if (MI_BATCHCOUNT(mi) < 1)
		bp->startcountdown = 1;
	bp->startcountdown = MI_BATCHCOUNT(mi);
	bp->startcountdown *= 60;
#if 0				/* Stricter if commented but they do not have to run bomb */
	if (bp->startcountdown > COUNTDOWN)
		bp->startcountdown = COUNTDOWN;
#endif
	if (bp->countdown == 0)	/* <--Stricter if uncommented */
		bp->countdown = (time((long *) NULL) + bp->startcountdown);
	/* Detonator Primed */
	XClearWindow(display, MI_WINDOW(mi));
	/*
	 *  Draw the graphics
	 *      Very simple - detonator box with countdown
	 *
	 *  ToDo:  Improve the graphics
	 *      (e.g. stick of dynamite, with burning fuse?)
	 */
	bp->loc.x = NRAND(bp->width / 2);
	bp->loc.y = NRAND(bp->height * 3 / 5);
	bp->x = bp->loc.x + bp->width / 4 - (bp->text_width / 2);
	bp->y = bp->loc.y + bp->height / 5;	/* Central-ish */
	if (MI_NPIXELS(mi) > 2)
		bp->color = NRAND(MI_NPIXELS(mi));
	detonator(mi, 1);
	bp->moveok = 0;
}

void
draw_bomb(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	GC          gc = MI_GC(mi);
	bombstruct *bp = &bombs[MI_SCREEN(mi)];
	char        number[NDIGITS + 2];
	unsigned long crayon;
	time_t      countleft;

	countleft = (bp->countdown - time((long *) NULL));
	if (countleft <= 0)
		explode(mi);	/* Bye, bye.... */
	else {
#ifdef SIMPLE_COUNTDOWN
		(void) sprintf(number, "%0*d", NDIGITS, (int) countleft);
#else
		(void) sprintf(number, "%0*d:%02d", NDIGITS - 2,
			       (int) countleft / 60, (int) countleft % 60);
#endif

		/* Blank out the previous number .... */
		XSetForeground(display, gc, MI_WIN_BLACK_PIXEL(mi));
		XFillRectangle(display, MI_WINDOW(mi), gc,
			       bp->x - bp->delta,
			       (bp->y - bp->text_ascent) - bp->delta,
			       bp->text_width + (2 * bp->delta),
		     (bp->text_ascent + bp->text_descent) + (2 * bp->delta));

		/* ... and count down */
		if (MI_NPIXELS(mi) <= 2)
			crayon = MI_WIN_WHITE_PIXEL(mi);
		else
#ifdef COLOUR_CHANGE		/* Blue to red */
			crayon = MI_PIXEL(mi, countleft * MI_NPIXELS(mi) / bp->startcountdown);
#else
			crayon = MI_PIXEL(mi, 1);
#endif
		if (!(countleft % 60)) {
			if (bp->moveok) {
				detonator(mi, 0);
				bp->loc.x = NRAND(bp->width / 2);
				bp->loc.y = NRAND(bp->height * 3 / 5);
				bp->x = bp->loc.x + bp->width / 4 - (bp->text_width / 2);
				bp->y = bp->loc.y + bp->height / 5;	/* Central-ish */
				if (MI_NPIXELS(mi) > 2)
					bp->color = NRAND(MI_NPIXELS(mi));
				detonator(mi, 1);
				bp->moveok = 0;
			}
		} else {
			bp->moveok = 1;
		}
		XSetForeground(display, gc, crayon);
		XDrawString(display, MI_WINDOW(mi), gc, bp->x, bp->y,
			    number, strlen(number));
	}
}

void
release_bomb(ModeInfo * mi)
{
	if (bombs != NULL) {
		(void) free((void *) bombs);
		bombs = NULL;
	}
}

void
refresh_bomb(ModeInfo * mi)
{
	detonator(mi, 1);
}

void
change_bomb(ModeInfo * mi)
{
	bombstruct *bp = &bombs[MI_SCREEN(mi)];

	detonator(mi, 0);
	bp->loc.x = NRAND(bp->width / 2);
	bp->loc.y = NRAND(bp->height * 3 / 5);
	bp->x = bp->loc.x + bp->width / 4 - (bp->text_width / 2);
	bp->y = bp->loc.y + bp->height / 5;	/* Central-ish */
	if (MI_NPIXELS(mi) > 2)
		bp->color = NRAND(MI_NPIXELS(mi));
	detonator(mi, 1);
	bp->moveok = 0;
}

#endif
