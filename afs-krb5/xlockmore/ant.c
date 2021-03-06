#ifndef lint
static char sccsid[] = "@(#)ant.c	3.11 96/09/20 xlockmore";

#endif

/*-
 * ant.c - Chris Langton's generalized turing machine ants (also known
 *           as Greg Turk's turmites) whose tape is the screen, for the
 *           X Window System lockscreen.
 *
 * Copyright (c) 1995 by David Bagley.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 04-Apr-96: -neighbors 6 runtime-time option added for hexagonal ants
 *            (bees), coded from an idea of Jim Propp's in Science News,
 *            Oct 28, 1995 VOL. 148 page 287
 * 20-Sep-95: Memory leak in ant fixed.  Now random colors.
 * 05-Sep-95: Coded from A.K. Dewdney's "Computer Recreations", Scientific
 *            American Magazine" Sep 1989 pp 180-183, Mar 1990 p 121
 *            Also used Ian Stewart's Mathematical Recreations, Scientific
 *            American Jul 1994 pp 104-107
 *            also used demon.c and life.c as a guide.
 */

#include "xlock.h"

#define BITS(n,w,h)\
  ap->pixmaps[ap->init_bits++]=\
  XCreatePixmapFromBitmapData(display,window,(char *)n,w,h,1,0,1)

/*-
  Species Grid     Number of Neigbors
  ------- ----     ------------------
  Ants    Square   4 (or 8)          <- 8 is not implemented
  Bees    Hexagon  6
  ?       Triangle (3, 9? or 12)     <- 3, 9 and 12 are not implemented
*/

#define DEF_TRUCHET  "False"

extern int  neighbors;
static Bool truchet;

static XrmOptionDescRec opts[] =
{
	{"-truchet", ".ant.truchet", XrmoptionNoArg, (caddr_t) "on"},
	{"+truchet", ".ant.truchet", XrmoptionNoArg, (caddr_t) "off"}
};
static argtype vars[] =
{
	{(caddr_t *) & truchet, "truchet", "Truchet", DEF_TRUCHET, t_Bool}
};
static OptionStruct desc[] =
{
	{"-/+truchet", "turn on/off Truchet lines"}
};

ModeSpecOpt ant_opts =
{2, opts, 1, vars, desc};

/* If you change the table you may have to change the following 2 constants */
#define STATES 2
#define COLORS 11
#define MINANTS 1
#define PATTERNSIZE 8
#define REDRAWSTEP 2000		/* How much tape to draw per cycle */
#define MINGRIDSIZE 24
#define MINSIZE 1
#define ANGLES 360

static unsigned char patterns[COLORS - 1][PATTERNSIZE] =
{
	{0x11, 0x22, 0x11, 0x22, 0x11, 0x22, 0x11, 0x22},	/* grey+white | stripe */
	{0x00, 0x66, 0x66, 0x00, 0x00, 0x66, 0x66, 0x00},	/* spots */
	{0x89, 0x44, 0x22, 0x11, 0x88, 0x44, 0x22, 0x11},	/* lt. / stripe */
	{0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66},	/* | bars */
	{0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa},	/* 50% grey */
	{0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00},	/* - bars */
	{0xee, 0xdd, 0xbb, 0x77, 0xee, 0xdd, 0xbb, 0x76},	/* dark \ stripe */
	{0xff, 0x99, 0x99, 0xff, 0xff, 0x99, 0x99, 0xff},	/* spots */
	{0xaa, 0xff, 0xff, 0x55, 0xaa, 0xff, 0xff, 0x55},	/* black+grey - stripe */
	{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}	/* black */
};

typedef struct {
	unsigned char color, direction, next;
} statestruct;

typedef struct {
	int         col, row;
  short       direction;
  unsigned char state;
} antstruct;

typedef struct {
	int         init_bits;
	int         neighbors;
	int         generation;
	int         xs, ys;
	int         xb, yb;
	int         nrows, ncols;
	int         width, height;
	unsigned char ncolors, nstates;
	int         n;
	int         redrawing, redrawpos;
	int         truchet;	/* Only for Turk modes */
	statestruct machine[COLORS * STATES];
	unsigned char *tape;
	unsigned char *truchet_state;
	antstruct  *ants;
	unsigned char colors[COLORS - 1];
	GC          stippledGC;
	Pixmap      pixmaps[COLORS - 1];
	XPoint      hexagonList[7];
} antfarmstruct;

static XPoint hexagonUnit[7] =
{
	{0, 0},
	{1, 1},
	{0, 2},
	{-1, 1},
	{-1, -1},
	{0, -2},
	{1, -1}
};

/* Relative ant moves */
#define F 0
#define R 1
#define HR 2
#define B 3
#define HL 4
#define L 5

static antfarmstruct *antfarms = NULL;

/* LANGTON'S ANT (10) Chaotic after 500, Builder after 10,000 (104p) */
/* TURK'S 100 ANT Always chaotic?, tested past 150,000,000 */
/* TURK'S 101 ANT Always chaotic? */
/* TURK'S 110 ANT Builder at 150 (18p) */
/* TURK'S 1000 ANT Always chaotic? */
/* TURK'S 1100 SYMMETRIC ANT  all even run 1's and 0's are symmetric */
/* other examples 1001, 110011, 110000, 1001101 */
/* TURK'S 1101 ANT Builder after 250,000 (388p) */
/* Once saw a chess horse type builder (i.e. non-45 degree builder) */

/* BEE ONLY */
/* All alternating 10 appear symmetric, no proof (i.e. 10, 1010, etc) */
/* Even runs of 0's and 1's are also symmetric */
/* I have yet to see Hexagonal builders. */

static unsigned char tables[][3 * COLORS * STATES + 2] =
{
  /* First 2 numbers are the size (ncolors, nstates) */
	{			/* SPIRALING PATTERN */
		2, 2,
		1, L, 0, 0, F, 1,
		1, R, 0, 1, R, 0
	},
	{			/* SQUARE (HEXAGON) BUILDER */
		2, 2,
		1, L, 0, 0, F, 1,
		0, R, 0, 1, R, 0
	}
};

#define NTABLES   (sizeof tables / sizeof tables[0])

static void
fillcell(ModeInfo * mi, int col, int row, int stipple)
{
	antfarmstruct *ap = &antfarms[MI_SCREEN(mi)];

	if (ap->neighbors == 6) {
		int         ccol = 2 * col + !(row & 1), crow = 2 * row;

		ap->hexagonList[0].x = ap->xb + ccol * ap->xs;
		ap->hexagonList[0].y = ap->yb + crow * ap->ys;
		if (ap->xs == 1 && ap->ys == 1)
			XFillRectangle(MI_DISPLAY(mi), MI_WINDOW(mi),
				       (stipple) ? ap->stippledGC : MI_GC(mi),
			   ap->hexagonList[0].x, ap->hexagonList[0].y, 1, 1);
		else
			XFillPolygon(MI_DISPLAY(mi), MI_WINDOW(mi),
				     (stipple) ? ap->stippledGC : MI_GC(mi),
			      ap->hexagonList, 6, Convex, CoordModePrevious);
	} else {
		XFillRectangle(MI_DISPLAY(mi), MI_WINDOW(mi),
			       (stipple) ? ap->stippledGC : MI_GC(mi),
		ap->xb + ap->xs * col, ap->yb + ap->ys * row, ap->xs, ap->ys);
	}
}

static void
truchetcell(ModeInfo * mi, int col, int row, int truchetstate)
{
	antfarmstruct *ap = &antfarms[MI_SCREEN(mi)];

	if (ap->neighbors == 6) {
		int         ccol = 2 * col + !(row & 1), crow = 2 * row;
		int         side;
		XPoint      hex, hex2;

		/* Very crude approx of Sqrt 3, so it will not cause drawing errors. */
		hex.x = ap->xb + ccol * ap->xs - (int) ((double) ap->xs * 1.6 / 2.0);
		hex.y = ap->yb + crow * ap->ys - (int) ((double) ap->ys * 1.6 / 2.0);
		for (side = 0; side < 6; side++) {
			if (side > 0) {
				hex.x += ap->hexagonList[side].x;
				hex.y += ap->hexagonList[side].y;
			}
			hex2.x = hex.x + ap->hexagonList[side + 1].x / 2;
			hex2.y = hex.y + ap->hexagonList[side + 1].y / 2;
			if (truchetstate == side % 3)
				/* Crude approx of 120 deg, so it will not cause drawing errors. */
				XDrawArc(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
					 hex2.x, hex2.y,
					 (int) ((double) ap->xs * 1.5), (int) ((double) ap->ys * 1.5),
				  ((555 - (side * 60)) % 360) * 64, 90 * 64);
		}
	} else {
		if (truchetstate) {
			XDrawArc(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
				 ap->xb + ap->xs * col - ap->xs / 2,
				 ap->yb + ap->ys * row + ap->ys / 2,
				 ap->xs, ap->ys,
				 1 * 64, 88 * 64);
			XDrawArc(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
				 ap->xb + ap->xs * col + ap->xs / 2,
				 ap->yb + ap->ys * row - ap->ys / 2,
				 ap->xs, ap->ys,
				 -91 * 64, -88 * 64);
		} else {
			XDrawArc(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
				 ap->xb + ap->xs * col - ap->xs / 2,
				 ap->yb + ap->ys * row - ap->ys / 2,
				 ap->xs, ap->ys,
				 -1 * 64, -88 * 64);
			XDrawArc(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
				 ap->xb + ap->xs * col + ap->xs / 2,
				 ap->yb + ap->ys * row + ap->ys / 2,
				 ap->xs, ap->ys,
				 91 * 64, 88 * 64);
		}
	}
}

static void
drawcell(ModeInfo * mi, int col, int row, unsigned char color)
{
	antfarmstruct *ap = &antfarms[MI_SCREEN(mi)];

	if (!color) {
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_WIN_BLACK_PIXEL(mi));
		fillcell(mi, col, row, 0);
		return;
	}
	if (MI_NPIXELS(mi) > 2) {
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi),
			       MI_PIXEL(mi, ap->colors[color - 1]));
		fillcell(mi, col, row, 0);
	} else {
		XGCValues   gcv;

		gcv.stipple = ap->pixmaps[color - 1];
		gcv.foreground = MI_WIN_WHITE_PIXEL(mi);
		gcv.background = MI_WIN_BLACK_PIXEL(mi);
		XChangeGC(MI_DISPLAY(mi), ap->stippledGC,
			  GCStipple | GCForeground | GCBackground, &gcv);
		fillcell(mi, col, row, 1);
	}
}

static void
drawtruchet(ModeInfo * mi, int col, int row,
	    unsigned char color, unsigned char truchetstate)
{
	antfarmstruct *ap = &antfarms[MI_SCREEN(mi)];

	if (!color)
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_WIN_WHITE_PIXEL(mi));
	else if (MI_NPIXELS(mi) > 2 || color > ap->ncolors / 2)
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_WIN_BLACK_PIXEL(mi));
	else
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_WIN_WHITE_PIXEL(mi));
	truchetcell(mi, col, row, truchetstate);
}

static void
draw_anant(ModeInfo * mi, int col, int row)
{

	XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_WIN_WHITE_PIXEL(mi));
	fillcell(mi, col, row, 0);
#if 0				/* Can not see eyes */
	{
		antfarmstruct *ap = &antfarms[MI_SCREEN(mi)];
		Display    *display = MI_DISPLAY(mi);
		Window      window = MI_WINDOW(mi);

		if (ap->xs > 2 && ap->ys > 2) {		/* Draw Eyes */

			XSetForeground(display, MI_GC(mi), MI_WIN_BLACK_PIXEL(mi));
			switch (direction) {
				case 0:
					XDrawPoint(display, window, MI_GC(mi),
					    ap->xb + ap->xs - 1, ap->yb + 1);
					XDrawPoint(display, window, MI_GC(mi),
						   ap->xb + ap->xs - 1, ap->yb + ap->ys - 2);
					break;
				case 180:
					XDrawPoint(display, window, MI_GC(mi), ap->xb, ap->yb + 1);
					XDrawPoint(display, window, MI_GC(mi), ap->xb, ap->yb + ap->ys - 2);
					break;
#ifdef BEE
#else
				case 90:
					XDrawPoint(display, window, MI_GC(mi), ap->xb + 1, ap->yb);
					XDrawPoint(display, window, MI_GC(mi),
						ap->xb + ap->xs - 2, ap->yb);
					break;
				case 270:
					XDrawPoint(display, window, MI_GC(mi),
					    ap->xb + 1, ap->yb + ap->ys - 1);
					XDrawPoint(display, window, MI_GC(mi),
						   ap->xb + ap->xs - 2, ap->yb + ap->ys - 1);
					break;
#endif
				default:
			}
		}
	}
#endif
}

#if 0
static void
RandomSoup(mi)
	ModeInfo   *mi;
{
	antfarmstruct *ap = &antfarms[MI_SCREEN(mi)];
	int         row, col, mrow = 0;

	for (row = 0; row < ap->nrows; ++row) {
		for (col = 0; col < ap->ncols; ++col) {
			ap->old[col + mrow] = (unsigned char) NRAND((int) ap->ncolors);
			drawcell(mi, col, row, ap->old[col + mrow]);
		}
		mrow += ap->nrows;
	}
}

#endif

static void
GetTable(ModeInfo * mi, int i)
{
	antfarmstruct *ap = &antfarms[MI_SCREEN(mi)];
	int         j, total;
	unsigned char *patptr;

	patptr = &tables[i][0];
	ap->ncolors = *patptr++;
	ap->nstates = *patptr++;
	total = ap->ncolors * ap->nstates;
	if (MI_WIN_IS_VERBOSE(mi))
		(void) printf("table number %d, colors %d, states %d\n",
			      i, ap->ncolors, ap->nstates);
	for (j = 0; j < total; j++) {
		ap->machine[j].color = *patptr++;
		ap->machine[j].direction = *patptr++;
		ap->machine[j].next = *patptr++;
	}
	ap->truchet = False;
}
static void
GetTurk(ModeInfo * mi, int i)
{
	antfarmstruct *ap = &antfarms[MI_SCREEN(mi)];
	int         power2, j, number, total;

	/* To force a number, say <i = 2;> has  i + 2 (or 4) digits in binary */
	power2 = 1 << (i + 1);
	/* Dont want numbers which in binary are all 1's. */
	number = NRAND(power2 - 1) + power2;
	/* To force a particular number, say <number = 10;> */

	ap->ncolors = i + 2;
	ap->nstates = 1;
	total = ap->ncolors * ap->nstates;
	for (j = 0; j < total; j++) {
		ap->machine[j].color = (j + 1) % total;
		ap->machine[j].direction = (power2 & number) ? R : L;
		ap->machine[j].next = 0;
		power2 >>= 1;
	}
	if (truchet)
		ap->truchet = 1;
	if (MI_WIN_IS_VERBOSE(mi))
		(void) printf("Turk's number %d, colors %d\n", number, ap->ncolors);
}

void
init_ant(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	int         size = MI_SIZE(mi);
	XGCValues   gcv;
	antfarmstruct *ap;
	int         col, row, i, dir;

	if (antfarms == NULL) {
		if ((antfarms = (antfarmstruct *) calloc(MI_NUM_SCREENS(mi),
					    sizeof (antfarmstruct))) == NULL)
			return;
	}
	ap = &antfarms[MI_SCREEN(mi)];
	ap->redrawing = 0;
	if ((MI_NPIXELS(mi) <= 2) && (ap->init_bits == 0)) {
		gcv.fill_style = FillOpaqueStippled;
		ap->stippledGC = XCreateGC(display, window, GCFillStyle, &gcv);
		for (i = 0; i < COLORS - 1; i++)
			BITS(patterns[i], PATTERNSIZE, PATTERNSIZE);
	}
	ap->generation = 0;
	ap->n = MI_BATCHCOUNT(mi);
	if (ap->n < -MINANTS) {
		/* if ap->n is random ... the size can change */
		if (ap->ants != NULL) {
			(void) free((void *) ap->ants);
			ap->ants = NULL;
		}
		ap->n = NRAND(-ap->n - MINANTS + 1) + MINANTS;
	} else if (ap->n < MINANTS)
		ap->n = MINANTS;

	ap->width = MI_WIN_WIDTH(mi);
	ap->height = MI_WIN_HEIGHT(mi);

	if (neighbors == 4)
		ap->neighbors = 4;
	else if (neighbors == 6)
		ap->neighbors = 6;
	else
		ap->neighbors = (LRAND() & 1) ? 4 : 6;

	if (ap->neighbors == 6) {
		int         nccols, ncrows;

		if (ap->width < 2)
			ap->width = 2;
		if (ap->height < 4)
			ap->height = 4;
		if (size < -MINSIZE)
			ap->ys = NRAND(MIN(-size, MAX(MINSIZE, MIN(ap->width, ap->height) /
				      MINGRIDSIZE)) - MINSIZE + 1) + MINSIZE;
		else if (size < MINSIZE) {
			if (!size)
				ap->ys = MAX(MINSIZE, MIN(ap->width, ap->height) / MINGRIDSIZE);
			else
				ap->ys = MINSIZE;
		} else
			ap->ys = MIN(size, MAX(MINSIZE, MIN(ap->width, ap->height) /
					       MINGRIDSIZE));
		ap->xs = ap->ys;
		nccols = MAX(ap->width / ap->xs - 2, 2);
		ncrows = MAX(ap->height / ap->ys - 1, 2);
		ap->ncols = nccols / 2;
		ap->nrows = 2 * (ncrows / 4);
		ap->xb = (ap->width - ap->xs * nccols) / 2 + ap->xs;
		ap->yb = (ap->height - ap->ys * ncrows) / 2 + ap->ys;
		for (i = 0; i < 7; i++) {
			ap->hexagonList[i].x = (ap->xs - 1) * hexagonUnit[i].x;
			ap->hexagonList[i].y = (ap->ys - 1) * hexagonUnit[i].y / 2;
		}
	} else {
		if (size < -MINSIZE)
			ap->ys = NRAND(MIN(-size, MAX(MINSIZE, MIN(ap->width, ap->height) /
				      MINGRIDSIZE)) - MINSIZE + 1) + MINSIZE;
		else if (size < MINSIZE) {
			if (!size)
				ap->ys = MAX(MINSIZE, MIN(ap->width, ap->height) / MINGRIDSIZE);
			else
				ap->ys = MINSIZE;
		} else
			ap->ys = MIN(size, MAX(MINSIZE, MIN(ap->width, ap->height) /
					       MINGRIDSIZE));
		ap->xs = ap->ys;
		ap->ncols = MAX(ap->width / ap->xs, 2);
		ap->nrows = MAX(ap->height / ap->ys, 2);
		ap->xb = (ap->width - ap->xs * ap->ncols) / 2;
		ap->yb = (ap->height - ap->ys * ap->nrows) / 2;
	}
	XClearWindow(display, MI_WINDOW(mi));

	if (!NRAND(COLORS))
		GetTable(mi, (int) (NRAND(NTABLES)));
	else
		GetTurk(mi, (int) (NRAND(COLORS - 1)));
	if (MI_NPIXELS(mi) > 2)
		for (i = 0; i < (int) ap->ncolors; i++)
			ap->colors[i] = (NRAND(MI_NPIXELS(mi)) +
			     i * MI_NPIXELS(mi)) / ((int) (ap->ncolors - 1));
	if (ap->ants == NULL)
		ap->ants = (antstruct *) malloc(ap->n * sizeof (antstruct));
	if (ap->tape != NULL)
		(void) free((char *) ap->tape);
	ap->tape = (unsigned char *)
		calloc(ap->ncols * ap->nrows, sizeof (unsigned char));

	if (ap->truchet_state != NULL)
		(void) free((char *) ap->truchet_state);
	ap->truchet_state = (unsigned char *)
		calloc(ap->ncols * ap->nrows, sizeof (unsigned char));

	col = ap->ncols / 2;
	row = ap->nrows / 2;
	dir = NRAND(ap->neighbors) * ANGLES / ap->neighbors;
	/* Have them all start in the same spot, why not? */
	for (i = 0; i < ap->n; i++) {
		ap->ants[i].col = col;
		ap->ants[i].row = row;
		ap->ants[i].direction = dir;
		ap->ants[i].state = 0;
	}
	draw_anant(mi, col, row);
}

void
draw_ant(ModeInfo * mi)
{
	antfarmstruct *ap = &antfarms[MI_SCREEN(mi)];
	antstruct  *anant;
	statestruct *status;
	int         i, state_pos, tape_pos;
	unsigned char color;
  short chg_dir, old_dir;

	for (i = 0; i < ap->n; i++) {
		anant = &ap->ants[i];
		tape_pos = anant->col + anant->row * ap->ncols;
		color = ap->tape[tape_pos];	/* read tape */
		state_pos = color + anant->state * ap->ncolors;
		status = &(ap->machine[state_pos]);
		drawcell(mi, anant->col, anant->row, status->color);
		ap->tape[tape_pos] = status->color;	/* write on tape */

		/* Find direction of Bees or Ants. */
    /* Translate relative direction to actual direction */
		old_dir = anant->direction;
		chg_dir = (ap->neighbors == 4 && status->direction > 1) ?
			(unsigned char) ((int) (status->direction + 1) / 2) : status->direction;
    chg_dir = (ANGLES - chg_dir * (ANGLES / ap->neighbors)) % ANGLES;
		anant->direction = (chg_dir + old_dir) % ANGLES;
		if (ap->truchet) {
			int         a, b;

			if (ap->neighbors == 4) {
				a = old_dir / 180;
				b = anant->direction / 180;
				a = ((a && !b) || (b && !a));
				drawtruchet(mi, anant->col, anant->row, status->color, a);
			} else {
				a = (old_dir / 60) % 3;
				b = (anant->direction / 60) % 3;
				a = (a + b + 1) % 3;
				drawtruchet(mi, anant->col, anant->row, status->color, a);
			}
			ap->truchet_state[tape_pos] = a + 1;
		}
		anant->state = status->next;

		/* If edge than wrap it */
		if (ap->neighbors == 6) {
			switch (anant->direction) {
				case 0:
					anant->col = (anant->col + 1 == ap->ncols) ? 0 : anant->col + 1;
					break;
				case 60:
					if (!(anant->row & 1))
						anant->col = (anant->col + 1 == ap->ncols) ? 0 : anant->col + 1;
					anant->row = (!anant->row) ? ap->nrows - 1 : anant->row - 1;
					break;
				case 120:
					if (anant->row & 1)
						anant->col = (!anant->col) ? ap->ncols - 1 : anant->col - 1;
					anant->row = (!anant->row) ? ap->nrows - 1 : anant->row - 1;
					break;
				case 180:
					anant->col = (!anant->col) ? ap->ncols - 1 : anant->col - 1;
					break;
				case 240:
					if (anant->row & 1)
						anant->col = (!anant->col) ? ap->ncols - 1 : anant->col - 1;
					anant->row = (anant->row + 1 == ap->nrows) ? 0 : anant->row + 1;
					break;
				case 300:
					if (!(anant->row & 1))
						anant->col = (anant->col + 1 == ap->ncols) ? 0 : anant->col + 1;
					anant->row = (anant->row + 1 == ap->nrows) ? 0 : anant->row + 1;
					break;
				default:
					warning("%s: wrong direction %d\n", anant->direction);
			}
		} else {
			switch (anant->direction) {
				case 0:
					anant->col = (anant->col + 1 == ap->ncols) ? 0 : anant->col + 1;
					break;
				case 90:
					anant->row = (!anant->row) ? ap->nrows - 1 : anant->row - 1;
					break;
				case 180:
					anant->col = (!anant->col) ? ap->ncols - 1 : anant->col - 1;
					break;
				case 270:
					anant->row = (anant->row + 1 == ap->nrows) ? 0 : anant->row + 1;
					break;
				default:
					warning("%s: wrong direction %d\n", anant->direction);
			}
		}
		draw_anant(mi, anant->col, anant->row);
	}
	if (++ap->generation > MI_CYCLES(mi))
		init_ant(mi);
	if (ap->redrawing) {
		int         i;

		for (i = 0; i < REDRAWSTEP; i++) {
			if (ap->tape[ap->redrawpos] ||
         (ap->truchet && ap->truchet_state[ap->redrawpos])) {
				drawcell(mi, ap->redrawpos % ap->ncols, ap->redrawpos / ap->ncols,
					 ap->tape[ap->redrawpos]);
				if (ap->truchet)
					drawtruchet(mi, ap->redrawpos % ap->ncols, ap->redrawpos / ap->ncols,
						    ap->tape[ap->redrawpos],
                ap->truchet_state[ap->redrawpos] - 1);
			}
			if (++(ap->redrawpos) >= ap->ncols * ap->nrows) {
				ap->redrawing = 0;
				break;
			}
		}
	}
}

void
release_ant(ModeInfo * mi)
{
	if (antfarms != NULL) {
		int         screen, shade;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
			antfarmstruct *ap = &antfarms[screen];

			if (ap->stippledGC != NULL) {
				XFreeGC(MI_DISPLAY(mi), ap->stippledGC);
			}
			if (ap->init_bits != 0) {
				for (shade = 0; shade < COLORS - 1; shade++)
					XFreePixmap(MI_DISPLAY(mi), ap->pixmaps[shade]);
			}
			if (ap->tape != NULL)
				(void) free((void *) ap->tape);
			if (ap->ants != NULL)
				(void) free((void *) ap->ants);
	    if (ap->truchet_state != NULL)
		    (void) free((char *) ap->truchet_state);
		}
		(void) free((void *) antfarms);
		antfarms = NULL;
	}
}

void
refresh_ant(ModeInfo * mi)
{
	antfarmstruct *ap = &antfarms[MI_SCREEN(mi)];

	ap->redrawing = 1;
	ap->redrawpos = 0;
}
