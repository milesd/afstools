
#ifndef lint
static char sccsid[] = "@(#)worm.c	3.11 96/09/20 xlockmore";

#endif

/*-
 * worm.c - draw wiggly worms.
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 03-Sep-96: fixed bug in allocation of space for worms, added 3d support
 *            Henrik Theiling <theiling@coli.uni-sb.de>
 * 27-Sep-95: put back malloc
 * 23-Sep-93: got rid of "rint". (David Bagley)
 * 27-Sep-91: got rid of all malloc calls since there were no calls to free().
 * 25-Sep-91: Integrated into X11R5 contrib xlock.
 *
 * Adapted from a concept in the Dec 87 issue of Scientific American p. 142.
 *
 * SunView version: Brad Taylor (brad@sun.com)
 * X11 version: Dave Lemke (lemke@ncd.com)
 * xlock version: Boris Putanec (bp@cs.brown.edu)
 */

#include	"xlock.h"
#include	<math.h>

#define MINSIZE 1

#define SEGMENTS  36
#define MINWORMS 1

#define MAXZ      750
#define MINZ      100
#define SCREENZ   200
#define GETZDIFF(z) (MI_DELTA3D(mi)*20.0*(1.0-(SCREENZ)/((float)(z)+MINZ)))
#define IRINT(x) ((int)(((x)>0.0)?(x)+0.5:(x)-0.5))

/* How many segments to draw per cycle when redrawing */
#define REDRAWSTEP 3

ModeSpecOpt worm_opts =
{0, NULL, 0, NULL, NULL};

typedef struct {
	XPoint     *circ;
	int        *diffcirc;
	int         dir, dir2;
	int         tail;
	int         x, y, z;
	int         redrawing, redrawpos;
} wormstuff;

typedef struct {
	int         xsize, ysize, zsize;
	int         wormlength;
	unsigned long monopix;
	int         nc;
	int         nw;
	int         circsize;
	wormstuff  *worm;
	XRectangle *rects[NUMCOLORS];	/* [NUMCOLORS][batchcount/NUMCOLORS+1] */
	int         size[NUMCOLORS];
	unsigned int chromo;
} wormstruct;

static float sintab[SEGMENTS];
static float costab[SEGMENTS];
static int  init_table = 0;

static wormstruct *worms = NULL;

static void
worm_doit(ModeInfo * mi, int which, unsigned long color)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	GC          gc = MI_GC(mi);
	wormstruct *wp = &worms[MI_SCREEN(mi)];
	wormstuff  *ws = &wp->worm[which];
	int         x, y, z;
	int         diff;

	ws->tail++;
	if (ws->tail == wp->wormlength)
		ws->tail = 0;

	x = ws->circ[ws->tail].x;
	y = ws->circ[ws->tail].y;

	if (MI_WIN_IS_USE3D(mi)) {
		diff = ws->diffcirc[ws->tail];
		if (MI_WIN_IS_INSTALL(mi)) {
			XSetForeground(display, gc, MI_NONE_COLOR(mi));
			XFillRectangle(display, window, gc, x - diff, y,
				       wp->circsize, wp->circsize);
			XFillRectangle(display, window, gc, x + diff, y,
				       wp->circsize, wp->circsize);
		} else {
			XClearArea(display, window, x - diff, y,
				   wp->circsize, wp->circsize, False);
			XClearArea(display, window, x + diff, y,
				   wp->circsize, wp->circsize, False);
		}
	} else
		XClearArea(display, window, x, y, wp->circsize, wp->circsize, False);

	if (LRAND() & 1)
		ws->dir = (ws->dir + 1) % SEGMENTS;
	else
		ws->dir = (ws->dir + SEGMENTS - 1) % SEGMENTS;
	if (LRAND() & 1)
		ws->dir2 = (ws->dir2 + 1) % SEGMENTS;
	else
		ws->dir2 = (ws->dir2 + SEGMENTS - 1) % SEGMENTS;

	x = (ws->x + IRINT((float) wp->circsize * costab[ws->dir]) +
	     wp->xsize) % wp->xsize;
	y = (ws->y + IRINT((float) wp->circsize * sintab[ws->dir]) +
	     wp->ysize) % wp->ysize;

	ws->circ[ws->tail].x = x;
	ws->circ[ws->tail].y = y;
	ws->x = x;
	ws->y = y;

	if (MI_WIN_IS_USE3D(mi)) {
		/* for the z-axis the wrap-around looks bad, so worms should just turn around. */
		z = (ws->z + wp->circsize * sintab[ws->dir2]);
		if (z < 0 || z >= wp->zsize)
			ws->dir2 = -ws->dir2;
		z = (ws->z + wp->circsize * sintab[ws->dir2]);

		diff = (int) (GETZDIFF(z) + 0.5);	/* ROUND */
		ws->diffcirc[ws->tail] = diff;

		ws->z = z;

		/* right eye */
		color = 0;
		wp->rects[color][wp->size[color]].x = x + diff;
		wp->rects[color][wp->size[color]].y = y;
		wp->size[color]++;

		/* left eye */
		color = 1;
		wp->rects[color][wp->size[color]].x = x - diff;
		wp->rects[color][wp->size[color]].y = y;
		wp->size[color]++;

#if 0
		if (ws->redrawing) {	/* Too hard for now */
			int         j;

			for (j = 0; j < REDRAWSTEP; j++) {
				int         k = (ws->tail - ws->redrawpos + wp->wormlength)
				% wp->wormlength;

				color = 0;
				wp->rects[color][wp->size[color]].x =
					ws->circ[k].x + ws->diffcirc[k];
				wp->rects[color][wp->size[color]].y =
					ws->circ[k].y;
				wp->size[color]++;

				color = 1;
				wp->rects[color][wp->size[color]].x =
					ws->circ[k].x - ws->diffcirc[k];
				wp->rects[color][wp->size[color]].y =
					ws->circ[k].y;
				wp->size[color]++;

				if (++(ws->redrawpos) >= wp->wormlength) {
					ws->redrawing = 0;
					break;
				}
			}
		}
#endif

	} else {

		wp->rects[color][wp->size[color]].x = x;
		wp->rects[color][wp->size[color]].y = y;
		wp->size[color]++;
		if (ws->redrawing) {
			int         j;

			ws->redrawpos++;
			/* Compensates for the changed ws->tail
			   since the last callback. */

			for (j = 0; j < REDRAWSTEP; j++) {
				int         k = (ws->tail - ws->redrawpos + wp->wormlength)
				% wp->wormlength;

				wp->rects[color][wp->size[color]].x = ws->circ[k].x;
				wp->rects[color][wp->size[color]].y = ws->circ[k].y;
				wp->size[color]++;

				if (++(ws->redrawpos) >= wp->wormlength) {
					ws->redrawing = 0;
					break;
				}
			}
		}
	}
}

static void
free_worms(wormstruct * wp)
{
	int         wn, i;

	if (wp->worm) {
		for (wn = 0; wn < wp->nw; wn++) {
			if (wp->worm[wn].circ)
				(void) free((void *) wp->worm[wn].circ);
			if (wp->worm[wn].diffcirc)
				(void) free((void *) wp->worm[wn].diffcirc);
		}
		(void) free((void *) wp->worm);
		wp->worm = NULL;
	}
	for (i = 0; i < NUMCOLORS; i++)
		if (wp->rects[i]) {
			(void) free((void *) wp->rects[i]);
			wp->rects[i] = NULL;
		}
}

void
init_worm(ModeInfo * mi)
{
	wormstruct *wp;
	int         size = MI_SIZE(mi);
	int         i, j, maxsize;

	if (worms == NULL) {
		if ((worms = (wormstruct *) calloc(MI_NUM_SCREENS(mi),
					       sizeof (wormstruct))) == NULL)
			return;
	}
	wp = &worms[MI_SCREEN(mi)];
	if (MI_NPIXELS(mi) <= 2 || MI_WIN_IS_USE3D(mi))
		wp->nc = 2;
	else
		wp->nc = MI_NPIXELS(mi);
	if (wp->nc > NUMCOLORS)
		wp->nc = NUMCOLORS;

	free_worms(wp);
	wp->nw = MI_BATCHCOUNT(mi);
	if (wp->nw < -MINWORMS)
		wp->nw = NRAND(-wp->nw - MINWORMS + 1) + MINWORMS;
	else if (wp->nw < MINWORMS)
		wp->nw = MINWORMS;
	if (!wp->worm)
		wp->worm = (wormstuff *) malloc(wp->nw * sizeof (wormstuff));

	maxsize = (REDRAWSTEP + 1) * wp->nw;	/*  / wp->nc + 1; */
	for (i = 0; i < NUMCOLORS; i++)
		if (!wp->rects[i])
			wp->rects[i] = (XRectangle *) malloc(maxsize * sizeof (XRectangle));


	if (!init_table) {
		init_table = 1;
		for (i = 0; i < SEGMENTS; i++) {
			sintab[i] = sin(i * 2 * M_PI / SEGMENTS);
			costab[i] = cos(i * 2 * M_PI / SEGMENTS);
		}
	}
	wp->xsize = MI_WIN_WIDTH(mi);
	wp->ysize = MI_WIN_HEIGHT(mi);
	wp->zsize = MAXZ - MINZ + 1;
	wp->monopix = MI_WIN_WHITE_PIXEL(mi);

	if (size < -MINSIZE)
		wp->circsize = NRAND(-size - MINSIZE + 1) + MINSIZE;
	else if (size < MINSIZE)
		wp->circsize = MINSIZE;
	else
		wp->circsize = size;

	for (i = 0; i < wp->nc; i++) {
		for (j = 0; j < maxsize; j++) {
			wp->rects[i][j].width = wp->circsize;
			wp->rects[i][j].height = wp->circsize;

		}
	}
	(void) memset((char *) wp->size, 0, wp->nc * sizeof (int));

	wp->wormlength = (int) sqrt(wp->xsize + wp->ysize) *
		MI_CYCLES(mi) / 8;	/* Fudge this to something reasonable */
	for (i = 0; i < wp->nw; i++) {
		wp->worm[i].circ = (XPoint *) malloc(wp->wormlength * sizeof (XPoint));
		wp->worm[i].diffcirc = (int *) malloc(wp->wormlength * sizeof (int));

		for (j = 0; j < wp->wormlength; j++) {
			wp->worm[i].circ[j].x = wp->xsize / 2;
			wp->worm[i].circ[j].y = wp->ysize / 2;
			if (MI_WIN_IS_USE3D(mi))
				wp->worm[i].diffcirc[j] = 0;
		}
		wp->worm[i].dir = NRAND(SEGMENTS);
		wp->worm[i].tail = 0;
		wp->worm[i].x = wp->xsize / 2;
		wp->worm[i].y = wp->ysize / 2;
		wp->worm[i].z = SCREENZ - MINZ;
		wp->worm[i].redrawing = 0;
	}

	if (MI_WIN_IS_INSTALL(mi) && MI_WIN_IS_USE3D(mi)) {
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_NONE_COLOR(mi));
		XFillRectangle(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
			       0, 0, wp->xsize, wp->ysize);
	} else
		XClearWindow(MI_DISPLAY(mi), MI_WINDOW(mi));
}

void
draw_worm(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	GC          gc = MI_GC(mi);
	wormstruct *wp = &worms[MI_SCREEN(mi)];
	unsigned long wcolor;
	int         i;

	(void) memset((char *) wp->size, 0, wp->nc * sizeof (int));

	for (i = 0; i < wp->nw; i++) {
		if (MI_NPIXELS(mi) > 2) {
			wcolor = (i + wp->chromo) % wp->nc;

			worm_doit(mi, i, wcolor);
		} else
			worm_doit(mi, i, (unsigned long) 0);
	}

	if (MI_WIN_IS_USE3D(mi)) {
		if (MI_WIN_IS_INSTALL(mi))
			XSetFunction(display, gc, GXor);
		XSetForeground(display, gc, MI_RIGHT_COLOR(mi));
		XFillRectangles(display, window, gc, wp->rects[0], wp->size[0]);

		XSetForeground(display, gc, MI_LEFT_COLOR(mi));
		XFillRectangles(display, window, gc, wp->rects[1], wp->size[1]);
		if (MI_WIN_IS_INSTALL(mi))
			XSetFunction(display, gc, GXcopy);
	} else if (MI_NPIXELS(mi) > 2) {
		for (i = 0; i < wp->nc; i++) {
			XSetForeground(display, gc, MI_PIXEL(mi, i));
			XFillRectangles(display, window, gc, wp->rects[i], wp->size[i]);
		}
	} else {
		XSetForeground(display, gc, wp->monopix);
		XFillRectangles(display, window, gc,
				wp->rects[0], wp->size[0]);
	}

	if (++wp->chromo == (unsigned long) wp->nc)
		wp->chromo = 0;
}

void
release_worm(ModeInfo * mi)
{
	if (worms != NULL) {
		int         screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++)
			free_worms(&worms[screen]);
		(void) free((void *) worms);
		worms = NULL;
	}
}

void
refresh_worm(ModeInfo * mi)
{
	if (MI_WIN_IS_USE3D(mi))
		/* The 3D code does drawing&clearing by XORing.  We do not
		   want to go to too much trouble here to make it redraw
		   correctly. */
		XClearWindow(MI_DISPLAY(mi), MI_WINDOW(mi));
	else if (worms != NULL) {
		wormstruct *wp = &worms[MI_SCREEN(mi)];
		int         i;

		for (i = 0; i < wp->nw; i++) {
			wp->worm[i].redrawing = 1;
			wp->worm[i].redrawpos = 0;
		}
	}
}
