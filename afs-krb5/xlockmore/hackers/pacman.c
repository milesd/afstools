#ifndef lint
static char sccsid[] = "@(#)pacman.c	3.11h 96/09/30";

#endif

/*-
 * pacman.c - pacman for xlock, the X Window System lockscreen.
 *
 * Copyright (c) 1995 by Heath Rice <rice@asl.dl.nec.com>.
 *
 * See xlock.c for copying information.
 *
 */

/*-
 * Pacman eats screen.  Ghosts put screen back.
 * Pacman eats ghosts when he encounters them.
 * After all ghosts are eaten, pacman continues
 * eating the screen until all of it is gone. Then
 * it starts over.
 */


#include "xlock.h"

/* aliases for vars defined in the bitmap file */
#define CELL_WIDTH   image_width
#define CELL_HEIGHT    image_height
#define CELL_BITS    image_bits

#include "ghost.xbm"

#define MINGHOSTS 1
#define NUMPACMEN 1
#define MAXMOUTH 11
#define MINGRIDSIZE 4
#define MINSIZE 1

#define NONE 0x0000
#define LT   0x1000
#define RT   0x0001
#define RB   0x0010
#define LB   0x0100
#define ALL  0x1111

#define YELLOW 9
#define GREEN 23
#define BLUE 45

typedef struct {
	int         col, row;
	int         nextbox, lastbox, nextcol, nextrow;
	int         dead;
	int         mouthstage, mouthdirection;
	/*int         color; */
} beingstruct;

typedef struct {
  int         pixelmode;
	int         width, height;
	int         nrows, ncols;
	int         xs, ys, xb, yb;
	int         inc;
	GC          stippledGC;
	Pixmap      ghostPixmap;
	beingstruct pacmen[NUMPACMEN];
	beingstruct *ghosts;
	int         nghosts;
	unsigned int *eaten;
	Pixmap      pacmanPixmap[4][MAXMOUTH];
} pacmangamestruct;

ModeSpecOpt pacman_opts =
{0, NULL, 0, NULL, NULL};

static pacmangamestruct *pacmangames = NULL;
static int  icon_width, icon_height;

#if 0
static void
clearcorners(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	GC          gc = MI_GC(mi);
	Window      window = MI_WINDOW(mi);
	pacmangamestruct *pp = &pacmangames[MI_SCREEN(mi)];

					XSetForeground(display, gc, MI_PIXEL(mi, GREEN));
	if ((pp->pacmen[0].nextrow == 0) && (pp->pacmen[0].nextcol == 0)) {
		XFillRectangle(display, window, gc, 0, 0,
		((pp->width / pp->ncols) / 2), ((pp->width / pp->ncols) / 2));
		pp->eaten[(pp->pacmen[0].nextcol * pp->nrows) + pp->pacmen[0].nextrow] |=
			RT | LT | RB | LB;
	} else if ((pp->pacmen[0].nextrow == pp->nrows - 1) &&
		   (pp->pacmen[0].nextcol == 0)) {
		XFillRectangle(display, window, gc,
		      0, (pp->nrows * pp->ys) - ((pp->width / pp->ncols) / 2),
		((pp->width / pp->ncols) / 2), ((pp->width / pp->ncols) / 2));
		pp->eaten[(pp->pacmen[0].nextcol * pp->nrows) + pp->pacmen[0].nextrow] |=
			RT | LT | RB | LB;
	} else if ((pp->pacmen[0].nextrow == 0) &&
		   (pp->pacmen[0].nextcol == pp->ncols - 1)) {
		XFillRectangle(display, window, gc,
		     (pp->ncols * pp->xs) - ((pp->width / pp->ncols) / 2), 0,
		((pp->width / pp->ncols) / 2), ((pp->width / pp->ncols) / 2));
		pp->eaten[(pp->pacmen[0].nextcol * pp->nrows) + pp->pacmen[0].nextrow] |=
			RT | LT | RB | LB;
	} else if ((pp->pacmen[0].nextrow == pp->nrows - 1) &&
		   (pp->pacmen[0].nextcol == pp->ncols - 1)) {
		XFillRectangle(display, window, gc,
			(pp->ncols * pp->xs) - ((pp->width / pp->ncols) / 2),
			(pp->nrows * pp->ys) - ((pp->width / pp->ncols) / 2),
		((pp->width / pp->ncols) / 2), ((pp->width / pp->ncols) / 2));
		pp->eaten[(pp->pacmen[0].nextcol * pp->nrows) + pp->pacmen[0].nextrow] |=
			RT | LT | RB | LB;
	}
}
#endif
static void
repopulate(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	pacmangamestruct *pp = &pacmangames[MI_SCREEN(mi)];
	int         ghost;

	if (pp->eaten)
		(void) free((void *) pp->eaten);
	pp->eaten = (unsigned int *) calloc((pp->nrows * pp->ncols),
					    sizeof (unsigned int));

	XClearWindow(display, window);

	for (ghost = 0; ghost < pp->nghosts; ghost++) {
		do {
			pp->ghosts[ghost].col = NRAND(pp->ncols);
			pp->ghosts[ghost].row = NRAND(pp->nrows);
			if ((pp->ghosts[ghost].row + pp->ghosts[ghost].col) % 2 !=
			    (pp->pacmen[0].row + pp->pacmen[0].col) % 2) {
				if (pp->ghosts[ghost].col != 0)
					pp->ghosts[ghost].col--;
				else
					pp->ghosts[ghost].col++;
			}
		}
		while ((pp->ghosts[ghost].col == pp->pacmen[0].col) &&
		       (pp->ghosts[ghost].row == pp->pacmen[0].row));
		pp->ghosts[ghost].dead = 0;
		pp->ghosts[ghost].lastbox = -1;

	}
}

static void
movepac(ModeInfo * mi)
{
	typedef struct {
		int         cfactor, rfactor;
		int         cf, rf;
		int         oldcf, oldrf;
	} being;
	being      *g, p[NUMPACMEN];

	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	GC          gc = MI_GC(mi);
	pacmangamestruct *pp = &pacmangames[MI_SCREEN(mi)];
	int         ghost, alldead, dir;
	XPoint      delta;

#ifndef FLASH
	extern void XEraseImage(Display * display, Window window, GC gc,
		   int x, int y, int xlast, int ylast, int xsize, int ysize);

#endif

	if (pp->pacmen[0].nextcol > pp->pacmen[0].col) {
		p[0].cfactor = 1;
		if (pp->eaten) {
			pp->eaten[(pp->pacmen[0].col * pp->nrows) + pp->pacmen[0].row] |=
				RT | RB;
			pp->eaten[((pp->pacmen[0].col + 1) * pp->nrows) + pp->pacmen[0].row] |=
				LT | LB;
		}
	} else if (pp->pacmen[0].col > pp->pacmen[0].nextcol) {
		p[0].cfactor = -1;
		if (pp->eaten) {
			pp->eaten[(pp->pacmen[0].col * pp->nrows) + pp->pacmen[0].row] |=
				LT | LB;
			pp->eaten[((pp->pacmen[0].col - 1) * pp->nrows) + pp->pacmen[0].row] |=
				RT | RB;
		}
	} else {
		p[0].cfactor = 0;
	}

	if (pp->pacmen[0].nextrow > pp->pacmen[0].row) {
		p[0].rfactor = 1;
		if (pp->eaten) {
			pp->eaten[(pp->pacmen[0].col * pp->nrows) + pp->pacmen[0].row] |=
				RB | LB;
			pp->eaten[(pp->pacmen[0].col * pp->nrows) + (pp->pacmen[0].row + 1)] |=
				RT | LT;
		}
	} else if (pp->pacmen[0].row > pp->pacmen[0].nextrow) {
		p[0].rfactor = -1;
		if (pp->eaten) {
			pp->eaten[(pp->pacmen[0].col * pp->nrows) + pp->pacmen[0].row] |=
				RT | LT;
			pp->eaten[(pp->pacmen[0].col * pp->nrows) + (pp->pacmen[0].row - 1)] |=
				RB | LB;
		}
	} else {
		p[0].rfactor = 0;
	}

	p[0].oldcf = pp->pacmen[0].col * pp->xs + pp->xb;
	p[0].oldrf = pp->pacmen[0].row * pp->ys + pp->yb;
	g = (being *) malloc(pp->nghosts * sizeof (being));

	for (ghost = 0; ghost < pp->nghosts; ghost++) {
		if (pp->ghosts[ghost].dead == 0) {
			pp->eaten[(pp->ghosts[ghost].col * pp->nrows) +
				  pp->ghosts[ghost].row] = NONE;
			if (pp->ghosts[ghost].nextcol > pp->ghosts[ghost].col) {
				g[ghost].cfactor = 1;
			} else if (pp->ghosts[ghost].col > pp->ghosts[ghost].nextcol) {
				g[ghost].cfactor = -1;
			} else {
				g[ghost].cfactor = 0;
			}
			if (pp->ghosts[ghost].nextrow > pp->ghosts[ghost].row) {
				g[ghost].rfactor = 1;
			} else if (pp->ghosts[ghost].row > pp->ghosts[ghost].nextrow) {
				g[ghost].rfactor = -1;
			} else {
				g[ghost].rfactor = 0;
			}

			g[ghost].oldcf = pp->ghosts[ghost].col * pp->xs +
				pp->xb;
			g[ghost].oldrf = pp->ghosts[ghost].row * pp->ys +
				pp->yb;
		}
	}
	for (delta.x = pp->inc, delta.y = pp->inc;
	     delta.x <= pp->xs + pp->inc - 1 &&
	     delta.y <= pp->ys + pp->inc - 1;
	     delta.x += pp->inc, delta.y += pp->inc) {
		if (delta.x > pp->xs)
			delta.x = pp->xs;
		if (delta.y > pp->ys)
			delta.y = pp->ys;
		p[0].cf = pp->pacmen[0].col * pp->xs + delta.x * p[0].cfactor +
			pp->xb;
		p[0].rf = pp->pacmen[0].row * pp->ys + delta.y * p[0].rfactor +
			pp->yb;

		dir = (ABS(p[0].cfactor) * (2 - p[0].cfactor) +
		       ABS(p[0].rfactor) * (1 + p[0].rfactor)) % 4;
		XSetForeground(display, gc, MI_WIN_BLACK_PIXEL(mi));
#ifdef FLASH
		XFillRectangle(display, window, gc,
			       p[0].oldcf, p[0].oldrf,
			       pp->xs, pp->ys);
#else
		XEraseImage(display, window, gc, p[0].cf, p[0].rf,
			    p[0].oldcf, p[0].oldrf, pp->xs, pp->ys);
#endif
		XSetTSOrigin(display, pp->stippledGC, p[0].cf, p[0].rf);
		XSetForeground(display, pp->stippledGC, MI_PIXEL(mi, YELLOW));
		XSetStipple(display, pp->stippledGC,
			    pp->pacmanPixmap[dir][pp->pacmen[0].mouthstage]);
#ifdef FLASH
		XSetFillStyle(display, pp->stippledGC, FillStippled);
#else
		XSetFillStyle(display, pp->stippledGC, FillOpaqueStippled);
#endif
		XFillRectangle(display, window, pp->stippledGC,
			       p[0].cf, p[0].rf, pp->xs, pp->ys);
		pp->pacmen[0].mouthstage += pp->pacmen[0].mouthdirection;
		if ((pp->pacmen[0].mouthstage >= MAXMOUTH) ||
		    (pp->pacmen[0].mouthstage < 0)) {
			pp->pacmen[0].mouthdirection *= -1;
			pp->pacmen[0].mouthstage += pp->pacmen[0].mouthdirection * 2;
		}
		p[0].oldcf = p[0].cf;
		p[0].oldrf = p[0].rf;

		for (ghost = 0; ghost < pp->nghosts; ghost++) {
			if (!pp->ghosts[ghost].dead) {
				g[ghost].cf = pp->ghosts[ghost].col * pp->xs +
					delta.x * g[ghost].cfactor + pp->xb;
				g[ghost].rf = pp->ghosts[ghost].row * pp->ys +
					delta.y * g[ghost].rfactor + pp->yb;
				XSetForeground(display, gc, MI_WIN_BLACK_PIXEL(mi));
				if (pp->pixelmode) {
					XFillRectangle(display, window, gc,
					      g[ghost].oldcf, g[ghost].oldrf,
						       pp->xs, pp->ys);
					XSetForeground(display, gc, MI_PIXEL(mi, BLUE));
					XFillRectangle(display, window, gc,
						       g[ghost].cf, g[ghost].rf, pp->xs, pp->ys);
					XFlush(display);
				} else {
#ifdef FLASH
					XFillRectangle(display, window, gc,
					      g[ghost].oldcf, g[ghost].oldrf,
						       pp->xs, pp->ys);
#else
					XEraseImage(display, window, gc, g[ghost].cf, g[ghost].rf,
						    g[ghost].oldcf, g[ghost].oldrf, pp->xs, pp->ys);
#endif
					XSetTSOrigin(display, pp->stippledGC, g[ghost].cf, g[ghost].rf);
					XSetForeground(display, pp->stippledGC, MI_PIXEL(mi, BLUE));
					XSetStipple(display, pp->stippledGC, pp->ghostPixmap);
#ifdef FLASH
					XSetFillStyle(display, pp->stippledGC, FillStippled);
#else
					XSetFillStyle(display, pp->stippledGC, FillOpaqueStippled);
#endif
					XFillRectangle(display, window, pp->stippledGC,
						       g[ghost].cf, g[ghost].rf, pp->xs, pp->ys);
					XFlush(display);
				}
				g[ghost].oldcf = g[ghost].cf;
				g[ghost].oldrf = g[ghost].rf;
			}
		}
		XFlush(display);
	}
#if 0
	clearcorners(mi);
#endif
	(void) free((void *) g);

	alldead = 1;
	for (ghost = 0; ghost < pp->nghosts; ghost++) {
		if (pp->ghosts[ghost].dead == 0) {
#if 0
			if ((pp->ghosts[ghost].nextrow >= pp->pacmen[0].nextrow - 1) &&
			    (pp->ghosts[ghost].nextrow <= pp->pacmen[0].nextrow + 1) &&
			    (pp->ghosts[ghost].nextcol >= pp->pacmen[0].nextcol - 1) &&
			    (pp->ghosts[ghost].nextcol <= pp->pacmen[0].nextcol + 1)) {
				(void) printf("%d %d\n", pp->ghosts[ghost].nextrow,
					      pp->ghosts[ghost].nextcol);
			}
#endif
			if (((pp->ghosts[ghost].nextrow == pp->pacmen[0].nextrow) &&
			     (pp->ghosts[ghost].nextcol == pp->pacmen[0].nextcol)) ||
			 ((pp->ghosts[ghost].nextrow == pp->pacmen[0].row) &&
			  (pp->ghosts[ghost].nextcol == pp->pacmen[0].col) &&
			  (pp->ghosts[ghost].row == pp->pacmen[0].nextrow) &&
			  (pp->ghosts[ghost].col == pp->pacmen[0].nextcol))) {
				pp->ghosts[ghost].dead = 1;
				XSetForeground(display, gc, MI_WIN_BLACK_PIXEL(mi));
				/*XFillRectangle(display, window, gc,
				   pp->ghosts[ghost].col * pp->xs + pp->xb,
				   pp->ghosts[ghost].row * pp->ys + pp->yb,
				   pp->xs, pp->ys); */
				XFillRectangle(display, window, gc,
				 pp->ghosts[ghost].nextcol * pp->xs + pp->xb,
				 pp->ghosts[ghost].nextrow * pp->ys + pp->yb,
					       pp->xs, pp->ys);
			} else {
				pp->ghosts[ghost].row = pp->ghosts[ghost].nextrow;
				pp->ghosts[ghost].col = pp->ghosts[ghost].nextcol;
				alldead = 0;
			}
		}
	}
	pp->pacmen[0].row = pp->pacmen[0].nextrow;
	pp->pacmen[0].col = pp->pacmen[0].nextcol;

	if (alldead && pp->eaten) {
		for (ghost = 0; ghost < (pp->nrows * pp->ncols); ghost++)
			if (pp->eaten[ghost] != ALL)
				break;
		if (ghost == pp->nrows * pp->ncols)
			repopulate(mi);
	}
}

void
init_pacman(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	int         size = MI_SIZE(mi);
	pacmangamestruct *pp;
	XGCValues   gcv;
	int         ghost, dir, mouth;
	GC          fg_gc, bg_gc;

	if (pacmangames == NULL) {
		if ((pacmangames = (pacmangamestruct *) calloc(MI_NUM_SCREENS(mi),
					 sizeof (pacmangamestruct))) == NULL)
			return;
		icon_width = CELL_WIDTH;
		icon_height = CELL_HEIGHT;
	}
	pp = &pacmangames[MI_SCREEN(mi)];
	if (pp->stippledGC == None) {
		gcv.foreground = MI_WIN_BLACK_PIXEL(mi);
		gcv.background = MI_WIN_BLACK_PIXEL(mi);
		if ((pp->stippledGC = XCreateGC(display, window,
				 GCForeground | GCBackground, &gcv)) == None)
			return;
	}
	pp->width = MI_WIN_WIDTH(mi);
	pp->height = MI_WIN_HEIGHT(mi);
	if (size == 0 ||
	 MINGRIDSIZE * size > pp->width || MINGRIDSIZE * size > pp->height) {
		if (pp->width > MINGRIDSIZE * icon_width &&
		    pp->height > MINGRIDSIZE * icon_height) {
			pp->xs = icon_width;
			pp->ys = icon_height;
      pp->pixelmode = 0;
			if (pp->ghostPixmap == None) {
				pp->ghostPixmap = XCreateBitmapFromData(display, window,
				(char *) CELL_BITS, CELL_WIDTH, CELL_HEIGHT);
			}
			pp->inc = pp->xs / 10 + 1;
		} else {
      pp->pixelmode = 1;
			pp->xs = pp->ys = MAX(MINSIZE, MIN(pp->width, pp->height) / MINGRIDSIZE);
			pp->inc = pp->xs / 10 + 1;
		}
	} else {
    pp->pixelmode = 1;
		if (size < -MINSIZE)
			pp->ys = NRAND(MIN(-size, MAX(MINSIZE, MIN(pp->width, pp->height) /
				      MINGRIDSIZE)) - MINSIZE + 1) + MINSIZE;
		else if (size < MINSIZE)
			pp->ys = MINSIZE;
		else
			pp->ys = MIN(size, MAX(MINSIZE, MIN(pp->width, pp->height) /
					       MINGRIDSIZE));
		pp->xs = pp->ys;
		pp->inc = pp->xs / 10 + 1;
	}
	pp->ncols = MAX(pp->width / pp->xs, 2);
	pp->nrows = MAX(pp->height / pp->ys, 2);
	pp->xb = pp->width - pp->ncols * pp->xs;
	pp->yb = pp->height - pp->nrows * pp->ys;

	if (pp->pacmanPixmap[0][0] != None)
		for (dir = 0; dir < 4; dir++)
			for (mouth = 0; mouth < MAXMOUTH; mouth++)
				XFreePixmap(display, pp->pacmanPixmap[dir][mouth]);


	for (dir = 0; dir < 4; dir++)
		for (mouth = 0; mouth < MAXMOUTH; mouth++) {
			pp->pacmanPixmap[dir][mouth] = XCreatePixmap(display, MI_WINDOW(mi),
							  pp->xs, pp->ys, 1);
			gcv.foreground = 1;
			fg_gc = XCreateGC(display, pp->pacmanPixmap[dir][mouth],
					  GCForeground, &gcv);
			gcv.foreground = 0;
			bg_gc = XCreateGC(display, pp->pacmanPixmap[dir][mouth],
					  GCForeground, &gcv);
			XFillRectangle(display, pp->pacmanPixmap[dir][mouth], bg_gc,
				       0, 0, pp->xs, pp->ys);
			if (pp->xs == 1 && pp->ys == 1)
				XFillRectangle(display, pp->pacmanPixmap[dir][mouth], fg_gc,
					       0, 0, pp->xs, pp->ys);
			else
				XFillArc(display, pp->pacmanPixmap[dir][mouth], fg_gc,
					 0, 0, pp->xs, pp->ys,
					 ((90 - dir * 90) + mouth * 5) * 64,
					 (360 + (-2 * mouth * 5)) * 64);
			XFreeGC(display, fg_gc);
			XFreeGC(display, bg_gc);
		}
	pp->pacmen[0].lastbox = -1;
	pp->pacmen[0].mouthdirection = 1;

	pp->nghosts = MI_BATCHCOUNT(mi);
	if (pp->nghosts < -MINGHOSTS) {
		/* if pp->nghosts is random ... the size can change */
		if (pp->ghosts != NULL) {
			(void) free((void *) pp->ghosts);
			pp->ghosts = NULL;
		}
		pp->nghosts = NRAND(-pp->nghosts - MINGHOSTS + 1) + MINGHOSTS;
	} else if (pp->nghosts < MINGHOSTS)
		pp->nghosts = MINGHOSTS;

	if (!pp->ghosts)
		pp->ghosts = (beingstruct *) malloc(pp->nghosts * sizeof (beingstruct));

	pp->pacmen[0].row = NRAND(pp->nrows);
	pp->pacmen[0].col = NRAND(pp->ncols);

	XClearWindow(display, window);

	pp->pacmen[0].mouthstage = MAXMOUTH - 1;
	for (ghost = 0; ghost < pp->nghosts; ghost++) {
		do {
			pp->ghosts[ghost].col = NRAND(pp->ncols);
			pp->ghosts[ghost].row = NRAND(pp->nrows);
		}
		while ((pp->ghosts[ghost].col == pp->pacmen[0].col) &&
		       (pp->ghosts[ghost].row == pp->pacmen[0].row));
		pp->ghosts[ghost].dead = 0;
		pp->ghosts[ghost].lastbox = -1;

	}
	if (pp->eaten)
		(void) free((void *) pp->eaten);
	pp->eaten = (unsigned int *) malloc((pp->nrows * pp->ncols) *
					    sizeof (unsigned int));

	if (pp->eaten)
		for (ghost = 0; ghost < (pp->nrows * pp->ncols); ghost++)
			pp->eaten[ghost] = NONE;
}

void
draw_pacman(ModeInfo * mi)
{
	pacmangamestruct *pp = &pacmangames[MI_SCREEN(mi)];
	int         g;

	do {
		if (NRAND(3) == 2)
			pp->pacmen[0].nextbox = NRAND(5);

		switch (pp->pacmen[0].nextbox) {
			case 0:
				if ((pp->pacmen[0].row == 0) || (pp->pacmen[0].lastbox == 2))
					pp->pacmen[0].nextbox = 99;
				else {
					pp->pacmen[0].nextrow = pp->pacmen[0].row - 1;
					pp->pacmen[0].nextcol = pp->pacmen[0].col;
				}
				break;

			case 1:
				if ((pp->pacmen[0].col == pp->ncols - 1) ||
				    (pp->pacmen[0].lastbox == 3))
					pp->pacmen[0].nextbox = 99;
				else {
					pp->pacmen[0].nextrow = pp->pacmen[0].row;
					pp->pacmen[0].nextcol = pp->pacmen[0].col + 1;
				}
				break;

			case 2:
				if ((pp->pacmen[0].row == pp->nrows - 1) ||
				    (pp->pacmen[0].lastbox == 0))
					pp->pacmen[0].nextbox = 99;
				else {
					pp->pacmen[0].nextrow = pp->pacmen[0].row + 1;
					pp->pacmen[0].nextcol = pp->pacmen[0].col;
				}
				break;

			case 3:
				if ((pp->pacmen[0].col == 0) || (pp->pacmen[0].lastbox == 1))
					pp->pacmen[0].nextbox = 99;
				else {
					pp->pacmen[0].nextrow = pp->pacmen[0].row;
					pp->pacmen[0].nextcol = pp->pacmen[0].col - 1;
				}
				break;

			default:
				pp->pacmen[0].nextbox = 99;
				break;
		}
	}
	while (pp->pacmen[0].nextbox == 99);


	for (g = 0; g < pp->nghosts; g++) {
		if (pp->ghosts[g].dead == 0) {
			do {
				if (NRAND(3) == 2)
					pp->ghosts[g].nextbox = NRAND(5);

				switch (pp->ghosts[g].nextbox) {
					case 0:
						if ((pp->ghosts[g].row == 0) || (pp->ghosts[g].lastbox == 2))
							pp->ghosts[g].nextbox = 99;
						else {
							pp->ghosts[g].nextrow = pp->ghosts[g].row - 1;
							pp->ghosts[g].nextcol = pp->ghosts[g].col;
						}
						break;

					case 1:
						if ((pp->ghosts[g].col == pp->ncols - 1) ||
						(pp->ghosts[g].lastbox == 3))
							pp->ghosts[g].nextbox = 99;
						else {
							pp->ghosts[g].nextrow = pp->ghosts[g].row;
							pp->ghosts[g].nextcol = pp->ghosts[g].col + 1;
						}
						break;

					case 2:
						if ((pp->ghosts[g].row == pp->nrows - 1) ||
						(pp->ghosts[g].lastbox == 0))
							pp->ghosts[g].nextbox = 99;
						else {
							pp->ghosts[g].nextrow = pp->ghosts[g].row + 1;
							pp->ghosts[g].nextcol = pp->ghosts[g].col;
						}
						break;

					case 3:
						if ((pp->ghosts[g].col == 0) || (pp->ghosts[g].lastbox == 1))
							pp->ghosts[g].nextbox = 99;
						else {
							pp->ghosts[g].nextrow = pp->ghosts[g].row;
							pp->ghosts[g].nextcol = pp->ghosts[g].col - 1;
						}
						break;

					default:
						pp->ghosts[g].nextbox = 99;
						break;
				}
			}
			while (pp->ghosts[g].nextbox == 99);
			pp->ghosts[g].lastbox = pp->ghosts[g].nextbox;
		}
	}
	if (pp->pacmen[0].lastbox != pp->pacmen[0].nextbox)
		pp->pacmen[0].mouthstage = 0;
	pp->pacmen[0].lastbox = pp->pacmen[0].nextbox;
	movepac(mi);

}

void
release_pacman(ModeInfo * mi)
{
	if (pacmangames != NULL) {
		int         screen, dir, mouth;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
			pacmangamestruct *pp = &pacmangames[screen];
			Display    *display = MI_DISPLAY(mi);

			if (pp->ghosts != NULL)
				(void) free((void *) pp->ghosts);
			if (pp->stippledGC != None)
				XFreeGC(display, pp->stippledGC);
			if (pp->ghostPixmap != None)
				XFreePixmap(display, pp->ghostPixmap);
			if (pp->pacmanPixmap[0][0] != None)
				for (dir = 0; dir < 4; dir++)
					for (mouth = 0; mouth < MAXMOUTH; mouth++)
						XFreePixmap(display, pp->pacmanPixmap[dir][mouth]);
		}
		(void) free((void *) pacmangames);
		pacmangames = NULL;
	}
}

void
refresh_pacman(ModeInfo * mi)
{
	/* Redraw dots */
}
