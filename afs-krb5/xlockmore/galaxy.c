
#ifndef lint
static char sccsid[] = "@(#)galaxy.c	3.11 96/09/20 xlockmore";

#endif

/*-
 * galaxy.c - Spinning galaxies for xlockmore
 *
 * Originally done by Uli Siegmund (uli@wombat.okapi.sub.org) on Amiga
 *   for EGS in Cluster
 * Port from Cluster/EGS to C/Intuition by Harald Backert
 * Port to X11 and incorporation into xlockmore by Hubert Feyrer
 *   (hubert.feyrer@rz.uni-regensburg.de)
 *
 * Revision History:
 * 09-Mar-94: VMS can generate a random number 0.0 which results in a
 *            division by zero, corrected by Jouk Jansen
 *            <joukj@crys.chem.uva.nl>
 * 30-Sep-94: Initial port by Hubert Feyer
 * 10-Oct-94: Add colors by Hubert Feyer
 * 23-Oct-94: Modified by David Bagley <bagleyd@megahertz.njit.edu>
 */

#include <math.h>
#include "xlock.h"

#define FLOATRAND ((double) LRAND() / ((double) MAXRAND))

#if 0
#define WRAP       1		/* Warp around edges */
#define BOUNCE     1		/* Bounce from borders */
#endif

#define MINSIZE       1
#define MINGALAXIES    1
#define MAX_STARS    300
#define MAX_IDELTAT    50
/* These come originally from the Cluster-version */
#define DEFAULT_GALAXIES  2
#define DEFAULT_STARS    1000
#define DEFAULT_HITITERATIONS  7500
#define DEFAULT_IDELTAT    200	/* 0.02 */

#define GALAXYRANGESIZE  0.1
#define GALAXYMINSIZE  0.1
#define QCONS    0.001

#define COLORBASE  8
  /* Colors for stars start here */
#define COLORSTEP  (NUMCOLORS/COLORBASE)	/* 8 colors per galaxy */

#define drawStar(x,y,size) if(size<=1) XDrawPoint(display,window,gc,x,y);\
  else XFillArc(display,window,gc,x,y,size,size,0,23040)

ModeSpecOpt galaxy_opts =
{0, NULL, 0, NULL, NULL};

typedef double Vector[3];
typedef double Matrix[3][3];

typedef struct {
	Vector      pos, vel;
	int         px, py;
	int         color;
} Star;
typedef struct {
	int         mass;
	int         nstars;
	Star       *stars;
	Vector      pos, vel;
	int         galcol;
} Galaxy;

typedef struct {
	struct {
		int         left;	/* x minimum */
		int         right;	/* x maximum */
		int         top;	/* y minimum */
		int         bottom;	/* y maximum */
	} clip;
	Matrix      mat;	/* Movement of stars(?) */
	double      scale;	/* Scale */
	int         midx;	/* Middle of screen, x */
	int         midy;	/* Middle of screen, y */
	double      size, star_size;	/* */
	Vector      diff;	/* */
	Galaxy     *galaxies;	/* the Whole Universe */
	int         ngalaxies;	/* # galaxies */
	double      f_deltat;	/* quality of calculation, calc'd by d_ideltat */
	int         f_hititerations;	/* # iterations before restart */
	int         step;	/* */
} unistruct;

static unistruct *universes = NULL;

static void
free_galaxies(unistruct * gp)
{
	int         gn;

	if (gp->galaxies) {
		for (gn = 0; gn < gp->ngalaxies; gn++)
			if (gp->galaxies[gn].stars)
				(void) free((void *) gp->galaxies[gn].stars);
		(void) free((void *) gp->galaxies);
		gp->galaxies = NULL;
	}
}

static void
startover(ModeInfo * mi)
{
	unistruct  *gp = &universes[MI_SCREEN(mi)];
	int         i, j;	/* more tmp */
	double      w1, w2;	/* more tmp */
	double      d, v, w, h;	/* yet more tmp */

	gp->step = 0;

	gp->ngalaxies = MI_BATCHCOUNT(mi);
	if (gp->ngalaxies < -MINGALAXIES) {
		free_galaxies(gp);
		gp->ngalaxies = NRAND(-gp->ngalaxies - MINGALAXIES + 1) + MINGALAXIES;
	} else if (gp->ngalaxies < MINGALAXIES)
		gp->ngalaxies = MINGALAXIES;
	if (!gp->galaxies)
		gp->galaxies = (Galaxy *) calloc(gp->ngalaxies, sizeof (Galaxy));

	for (i = 0; i < gp->ngalaxies; ++i) {
		double      sinw1, sinw2, cosw1, cosw2;

		gp->galaxies[i].galcol = NRAND(COLORBASE - 2);
		if (gp->galaxies[i].galcol > 1)
			gp->galaxies[i].galcol += 2;	/* Mult 8; 16..31 no green stars */
		/* Galaxies still may have some green stars but are not all green. */

		if (gp->galaxies[i].stars)
			(void) free((void *) gp->galaxies[i].stars);
		gp->galaxies[i].nstars = (NRAND(MAX_STARS / 2)) + MAX_STARS / 2;
		gp->galaxies[i].stars =
			(Star *) malloc(gp->galaxies[i].nstars * sizeof (Star));
		w1 = 2.0 * M_PI * FLOATRAND;
		w2 = 2.0 * M_PI * FLOATRAND;
		sinw1 = sin(w1);
		sinw2 = sin(w2);
		cosw1 = cos(w1);
		cosw2 = cos(w2);

		gp->mat[0][0] = cosw2;
		gp->mat[0][1] = -sinw1 * sinw2;
		gp->mat[0][2] = cosw1 * sinw2;
		gp->mat[1][0] = 0.0;
		gp->mat[1][1] = cosw1;
		gp->mat[1][2] = sinw1;
		gp->mat[2][0] = -sinw2;
		gp->mat[2][1] = -sinw1 * cosw2;
		gp->mat[2][2] = cosw1 * cosw2;

		gp->galaxies[i].vel[0] = FLOATRAND * 2.0 - 1.0;
		gp->galaxies[i].vel[1] = FLOATRAND * 2.0 - 1.0;
		gp->galaxies[i].vel[2] = FLOATRAND * 2.0 - 1.0;
		gp->galaxies[i].pos[0] = -gp->galaxies[i].vel[0] * gp->f_deltat *
			gp->f_hititerations + FLOATRAND - 0.5;
		gp->galaxies[i].pos[1] = -gp->galaxies[i].vel[1] * gp->f_deltat *
			gp->f_hititerations + FLOATRAND - 0.5;
		gp->galaxies[i].pos[2] = -gp->galaxies[i].vel[2] * gp->f_deltat *
			gp->f_hititerations + FLOATRAND - 0.5;

		gp->galaxies[i].mass = (int) (FLOATRAND * 1000.0) + 1;

		gp->size = GALAXYRANGESIZE * FLOATRAND + GALAXYMINSIZE;

		for (j = 0; j < gp->galaxies[i].nstars; ++j) {
			double      sinw, cosw;

			w = 2.0 * M_PI * FLOATRAND;
			sinw = sin(w);
			cosw = cos(w);
			d = FLOATRAND * gp->size;
			h = FLOATRAND * exp(-2.0 * (d / gp->size)) / 5.0 * gp->size;
			if (FLOATRAND < 0.5)
				h = -h;
			gp->galaxies[i].stars[j].pos[0] = gp->mat[0][0] * d * cosw +
				gp->mat[1][0] * d * sinw + gp->mat[2][0] * h +
				gp->galaxies[i].pos[0];
			gp->galaxies[i].stars[j].pos[1] =
				gp->mat[0][1] * d * cosw + gp->mat[1][1] * d * sinw +
				gp->mat[2][1] * h + gp->galaxies[i].pos[1];
			gp->galaxies[i].stars[j].pos[2] =
				gp->mat[0][2] * d * cosw + gp->mat[1][2] * d * sinw +
				gp->mat[2][2] * h + gp->galaxies[i].pos[2];

			v = sqrt(gp->galaxies[i].mass * QCONS / sqrt(d * d + h * h));
			gp->galaxies[i].stars[j].vel[0] =
				-gp->mat[0][0] * v * sinw + gp->mat[1][0] * v * cosw +
				gp->galaxies[i].vel[0];
			gp->galaxies[i].stars[j].vel[1] =
				-gp->mat[0][1] * v * sinw + gp->mat[1][1] * v * cosw +
				gp->galaxies[i].vel[1];
			gp->galaxies[i].stars[j].vel[2] =
				-gp->mat[0][2] * v * sinw + gp->mat[1][2] * v * cosw +
				gp->galaxies[i].vel[2];

			gp->galaxies[i].stars[j].color =
				COLORSTEP * gp->galaxies[i].galcol + j % COLORSTEP;

			gp->galaxies[i].stars[j].px = 0;
			gp->galaxies[i].stars[j].py = 0;
		}
	}

	XClearWindow(MI_DISPLAY(mi), MI_WINDOW(mi));

#if 0
	(void) printf("ngalaxies=%d, f_hititerations=%d\n",
		      gp->ngalaxies, gp->f_hititerations);
	(void) printf("f_deltat=%g\n", gp->f_deltat);
	(void) printf("Screen: ");
	(void) printf("%dx%d pixel (%d-%d, %d-%d)\n",
	  (gp->clip.right - gp->clip.left), (gp->clip.bottom - gp->clip.top),
	       gp->clip.left, gp->clip.right, gp->clip.top, gp->clip.bottom);
#endif /*0 */
}

void
init_galaxy(ModeInfo * mi)
{
	unistruct  *gp = &universes[MI_SCREEN(mi)];
	int         size = MI_SIZE(mi);

	if (universes == NULL) {
		if ((universes = (unistruct *) calloc(MI_NUM_SCREENS(mi),
						sizeof (unistruct))) == NULL)
			return;
	}
	gp = &universes[MI_SCREEN(mi)];

	gp->f_hititerations = MI_CYCLES(mi);
	gp->f_deltat = ((double) MAX_IDELTAT) / 10000.0;

	gp->clip.left = 0;
	gp->clip.top = 0;
	gp->clip.right = MI_WIN_WIDTH(mi);
	gp->clip.bottom = MI_WIN_HEIGHT(mi);

	gp->scale = (double) (gp->clip.right) / 4.0;
	gp->midx = gp->clip.right / 2;
	gp->midy = gp->clip.bottom / 2;
	if (size < -MINSIZE)
		gp->star_size = NRAND(-size - MINSIZE + 1) + MINSIZE;
	else if (size < MINSIZE)
		gp->star_size = MINSIZE;
	else
		gp->star_size = size;
	startover(mi);
}

void
draw_galaxy(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	GC          gc = MI_GC(mi);
	unistruct  *gp = &universes[MI_SCREEN(mi)];
	double      d;		/* tmp */
	int         i, j, k;	/* more tmp */

	for (i = 0; i < gp->ngalaxies; ++i) {
		for (j = 0; j < gp->galaxies[i].nstars; ++j) {
			for (k = 0; k < gp->ngalaxies; ++k) {
				gp->diff[0] = gp->galaxies[k].pos[0] - gp->galaxies[i].stars[j].pos[0];
				gp->diff[1] = gp->galaxies[k].pos[1] - gp->galaxies[i].stars[j].pos[1];
				gp->diff[2] = gp->galaxies[k].pos[2] - gp->galaxies[i].stars[j].pos[2];
				d = gp->diff[0] * gp->diff[0] + gp->diff[1] * gp->diff[1] +
					gp->diff[2] * gp->diff[2];
				d = gp->galaxies[k].mass / (d * sqrt(d)) * gp->f_deltat * QCONS;
				gp->diff[0] *= d;
				gp->diff[1] *= d;
				gp->diff[2] *= d;
				gp->galaxies[i].stars[j].vel[0] += gp->diff[0];
				gp->galaxies[i].stars[j].vel[1] += gp->diff[1];
				gp->galaxies[i].stars[j].vel[2] += gp->diff[2];
			}
			gp->galaxies[i].stars[j].pos[0] += gp->galaxies[i].stars[j].vel[0] *
				gp->f_deltat;
			gp->galaxies[i].stars[j].pos[1] += gp->galaxies[i].stars[j].vel[1] *
				gp->f_deltat;
			gp->galaxies[i].stars[j].pos[2] += gp->galaxies[i].stars[j].vel[2] *
				gp->f_deltat;

			if (gp->galaxies[i].stars[j].px >= gp->clip.left &&
			    gp->galaxies[i].stars[j].px <= gp->clip.right - gp->star_size &&
			    gp->galaxies[i].stars[j].py >= gp->clip.top &&
			    gp->galaxies[i].stars[j].py <= gp->clip.bottom - gp->star_size) {
				XSetForeground(display, gc, MI_WIN_BLACK_PIXEL(mi));
				drawStar(gp->galaxies[i].stars[j].px, gp->galaxies[i].stars[j].py,
					 gp->star_size);
			}
			gp->galaxies[i].stars[j].px = (int) (gp->galaxies[i].stars[j].pos[0] *
						       gp->scale) + gp->midx;
			gp->galaxies[i].stars[j].py = (int) (gp->galaxies[i].stars[j].pos[1] *
						       gp->scale) + gp->midy;


#ifdef WRAP
			if (gp->galaxies[i].stars[j].px < gp->clip.left) {
				(void) printf("wrap l -> r\n");
				gp->galaxies[i].stars[j].px = gp->clip.right;
			}
			if (gp->galaxies[i].stars[j].px > gp->clip.right) {
				(void) printf("wrap r -> l\n");
				gp->galaxies[i].stars[j].px = gp->clip.left;
			}
			if (gp->galaxies[i].stars[j].py > gp->clip.bottom) {
				(void) printf("wrap b -> t\n");
				gp->galaxies[i].stars[j].py = gp->clip.top;
			}
			if (gp->galaxies[i].stars[j].py < gp->clip.top) {
				(void) printf("wrap t -> b\n");
				gp->galaxies[i].stars[j].py = gp->clip.bottom;
			}
#endif /*WRAP */


			if (gp->galaxies[i].stars[j].px >= gp->clip.left &&
			    gp->galaxies[i].stars[j].px <= gp->clip.right - gp->star_size &&
			    gp->galaxies[i].stars[j].py >= gp->clip.top &&
			    gp->galaxies[i].stars[j].py <= gp->clip.bottom - gp->star_size) {
				if (MI_NPIXELS(mi) > 2)
					XSetForeground(display, gc,
						       MI_PIXEL(mi, gp->galaxies[i].stars[j].color));
				else
					XSetForeground(display, gc, MI_WIN_WHITE_PIXEL(mi));
				drawStar(gp->galaxies[i].stars[j].px, gp->galaxies[i].stars[j].py,
					 gp->star_size);
			}
		}

		for (k = i + 1; k < gp->ngalaxies; ++k) {
			gp->diff[0] = gp->galaxies[k].pos[0] - gp->galaxies[i].pos[0];
			gp->diff[1] = gp->galaxies[k].pos[1] - gp->galaxies[i].pos[1];
			gp->diff[2] = gp->galaxies[k].pos[2] - gp->galaxies[i].pos[2];
			d = gp->diff[0] * gp->diff[0] + gp->diff[1] * gp->diff[1] +
				gp->diff[2] * gp->diff[2];
			d = gp->galaxies[i].mass * gp->galaxies[k].mass / (d * sqrt(d)) *
				gp->f_deltat * QCONS;
			gp->diff[0] *= d;
			gp->diff[1] *= d;
			gp->diff[2] *= d;
			gp->galaxies[i].vel[0] += gp->diff[0] / gp->galaxies[i].mass;
			gp->galaxies[i].vel[1] += gp->diff[1] / gp->galaxies[i].mass;
			gp->galaxies[i].vel[2] += gp->diff[2] / gp->galaxies[i].mass;
			gp->galaxies[k].vel[0] -= gp->diff[0] / gp->galaxies[k].mass;
			gp->galaxies[k].vel[1] -= gp->diff[1] / gp->galaxies[k].mass;
			gp->galaxies[k].vel[2] -= gp->diff[2] / gp->galaxies[k].mass;
		}
		gp->galaxies[i].pos[0] += gp->galaxies[i].vel[0] * gp->f_deltat;
		gp->galaxies[i].pos[1] += gp->galaxies[i].vel[1] * gp->f_deltat;
		gp->galaxies[i].pos[2] += gp->galaxies[i].vel[2] * gp->f_deltat;
	}

	gp->step++;
	if (gp->step > gp->f_hititerations * 4)
		startover(mi);
}

void
release_galaxy(ModeInfo * mi)
{
	if (universes != NULL) {
		int         screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++)
			free_galaxies(&universes[screen]);
		(void) free((void *) universes);
		universes = NULL;
	}
}

void
refresh_galaxy(ModeInfo * mi)
{
	/* Do nothing, it will refresh by itself */
}
