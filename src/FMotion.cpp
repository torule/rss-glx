/*
 * Copyright (C) 2002  Jeremie Allard (Hufo / N.A.A.)
 *
 * Note: Credits for the 2d warping field fly to
 * Hugo Elias: http://freespace.virgin.net/hugo.elias
 * I've transposed to 3d.
 *
 * Ported to Linux by Tugrul Galatali <tugrul@galatali.com>
 *
 * hufo_smoke is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * hufo_smoke is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <cmath>

// In case cmath doesn't pull in all the usual suspects from math.h
#ifndef M_PI
#include <math.h>
#endif

#include <GL/gl.h>

#include "FMotion.h"
#include "rsRand.h"

#define TILE 1.0f
#define NBTILEZ 8
#define NBTILEY 4
#define NBTILEX 8

#define WARPFACT (TILE*5.0f)
#define AFFFACT ((TILE/16.0f)/WARPFACT)
#define SPRINGFACT 0.01f
#define DYNFACT 0.998f
#define MAXLENGTH (WARPFACT*4)

#define ANGFACT 1
#define ANGRND 0.16f
#define ANG0 0.16f

#define SHFT 8

#define FLOATTOINTCONST2 (((65536.0*16)))
inline int f2int2 (float f)
{
	f += FLOATTOINTCONST2;
	return ((*((int *)(void *)&f)) & 0x007fffff) - 0x00400000;
}

#define FLOATTOINTCONST (((1.5*65536*256)))
inline int f2int (float f)
{
	f += FLOATTOINTCONST;
	return ((*((int *)(void *)&f)) & 0x007fffff) - 0x00400000;
}

#define Float2Int(f) (f2int(f))
#define Float2Int2(f) (f2int2(f))

typedef float real;

#define SQRT_2 1.414213562f
#define SQRT_3 1.732050808f
#define SQRT_5 2.236067977f
#define SQRT_6 2.449489743f

#define D0     0
#define D1     1
#define D2     2
#define D1_1   SQRT_2
#define D2_1   SQRT_5
#define D2_2   2*SQRT_2
#define D1_1_1 SQRT_3
#define D2_1_1 SQRT_6
#define D2_2_1 3
#define D2_2_2 3*SQRT_2

real norms[3][3][3] = { 
	{ 
		{ TILE * D0, TILE * D1, TILE * D2 },
		{ TILE * D1, TILE * D1_1, TILE * D2_1 },
		{ TILE * D2, TILE * D2_1, TILE * D2_2 }, 
	}, 
	{
		{ TILE * D1, TILE * D1_1, TILE * D2_1 },
		{ TILE * D1_1, TILE * D1_1_1, TILE * D2_1_1 },
		{ TILE * D2_1, TILE * D2_1_1, TILE * D2_2_1 }
	},
	{
		{ TILE * D2, TILE * D2_1, TILE * D2_2 },
		{ TILE * D2_1, TILE * D2_1_1, TILE * D2_2_1 },
		{ TILE * D2_2, TILE * D2_2_1, TILE * D2_2_2 }
	}
};

real distvar[3] = { -TILE, 0, TILE };

struct PARTICLE offset[NBTILEZ][NBTILEY][NBTILEZ];

real deplfact[1 << SHFT];

bool FMotionInit ()
{
	int i;

	for (i = 0; i < (1 << SHFT); ++i)
		deplfact[i] = i / ((real) (1 << SHFT));
	int x, y, z;
	PARTICLE *p;

	//initialise gelly
	real t, t1, t2, t3, tt, tt1, tt2, tt3, ttt, ttt1, ttt2, ttt3, xf, yf, zf;
	real f, f1, f2, f3, ff, ff1, ff2, ff3, fff, fff1, fff2, fff3;

	ttt = rsRandf (2 * M_PI);
	f = rsRandf (ANGRND) + ANG0;
	ff = rsRandf (ANGRND) + ANG0;
	fff = rsRandf (ANGRND) + ANG0;
	ttt1 = rsRandf (2 * M_PI);
	f1 = rsRandf (ANGRND) + ANG0;
	ff1 = rsRandf (ANGRND) + ANG0;
	fff1 = rsRandf (ANGRND) + ANG0;
	ttt2 = rsRandf (2 * M_PI);
	f2 = rsRandf (ANGRND) + ANG0;
	ff2 = rsRandf (ANGRND) + ANG0;
	fff2 = rsRandf (ANGRND) + ANG0;
	ttt3 = rsRandf (2 * M_PI);
	f3 = rsRandf (ANGRND) + ANG0;
	ff3 = rsRandf (ANGRND) + ANG0;
	fff3 = rsRandf (ANGRND) + ANG0;
	p = &offset[0][0][0];
	for (z = 0; z < NBTILEZ; z++) {
		zf = real (z);
		tt = ttt;
		tt1 = ttt1;
		tt2 = ttt3;
		tt3 = ttt3;
		ttt += fff;
		ttt1 += fff1;
		ttt2 += fff2;
		ttt3 += fff3;
		for (y = 0; y < NBTILEY; y++) {
			yf = real (y);
			t = tt;
			t1 = tt1;
			t2 = tt3;
			t3 = tt3;
			tt += ff;
			tt1 += ff1;
			tt2 += ff2;
			tt3 += ff3;
			for (x = 0; x < NBTILEX; x++, p++) {
				xf = real (x);
				t += f;
				t1 += f1;
				t2 += f2;
				t3 += f3;
				p->p.x() = (real) (WARPFACT * (sin (ANGFACT * (t2 - xf)) + cos (ANGFACT * (t2 + yf)) + sin (ANGFACT * (t - yf)) - cos (ANGFACT * (t + xf))));	//+x*TILE);
				p->p.y() = (real) (WARPFACT * (sin (ANGFACT * (t1 - xf)) + cos (ANGFACT * (t3 + yf)) + sin (ANGFACT * (t3 - yf)) - cos (ANGFACT * (t1 + xf))));	//+y*TILE);
				p->p.z() = (real) (WARPFACT * (sin (ANGFACT * (t - xf)) + cos (ANGFACT * (t2 + yf)) + sin (ANGFACT * (t - yf)) - cos (ANGFACT * (t2 + xf))));	//+z*TILE);
				p->v.zero ();
			}
		}
	}
	return 1;
}

void FMotionQuit ()
{
}

void FMotionAnimate (const float &dt)
{
// Handles the wobbling gel, which is used to give the WarpMap it's shape
	int x, y, z, xi, yi, zi, xh, xl, yh, yl, zh, zl;
	rsVec spring;
	real norm;
	PARTICLE *p, *p2;
	real ft = SPRINGFACT * dt;

	p = &offset[0][0][0];
	for (z = 0; z < NBTILEZ; z++) {
		zh = z + 1;
		zl = z - 1;	//if (zl<0)   zl=0; if (zh>tz)        zh=tz;
		for (y = 0; y < NBTILEY; y++) {
			yh = y + 1;
			yl = y - 1;	//if (yl<0)   yl=0; if (yh>ty)        yh=ty;
			for (x = 0; x < NBTILEX; x++, p++) {
				// Add attraction back to origin of this point
				p->v += p->p * (-p->p.length() * ft);
				xh = x + 1;
				xl = x - 1;	//if (xl<0)   xl=0; if (xh>tx)        xh=tx;
				for (zi = zl; zi <= zh; zi++) {
					for (yi = yl; yi <= yh; yi++) {
						for (xi = xl; xi <= xh; xi++) {
							if ((xi != x) || (yi != y) || (zi != z)) {
								norm = norms[abs (zi - z)][abs (yi - y)][abs (xi - x)];
								p2 = &offset[zi & (NBTILEZ - 1)][yi & (NBTILEY - 1)][xi & (NBTILEX - 1)];
								spring = p2->p;
								spring -= p->p;
								spring.x() += distvar[xi - x + 1];
								spring.y() += distvar[yi - y + 1];
								spring.z() += distvar[zi - z + 1];
								spring *= (norm - spring.length ()) * ft;
								p2->v += spring;
							}
						}
					}
				}
			}
		}
	}
	p = &offset[0][0][0];
	ft = (real) pow (DYNFACT, dt);
	for (z = 0; z < NBTILEZ; z++) {
		for (y = 0; y < NBTILEY; y++) {
			for (x = 0; x < NBTILEX; x++, p++) {
				p->p += p->v * dt;
				if (p->p.length2 () > (MAXLENGTH * MAXLENGTH)) {
					p->v.zero ();
				} else
					p->v *= ft;
			}
		}
	}
}

#define Point2STX(p) ( (Float2Int((p.x())*(TILE*(1<<SHFT)))+((NBTILEX/2)<<SHFT)) & ((NBTILEX<<SHFT)-1) )
#define Point2STY(p) ( (Float2Int((p.y())*(TILE*(1<<SHFT)))+((NBTILEY/2)<<SHFT)) & ((NBTILEY<<SHFT)-1) )
#define Point2STZ(p) ( (Float2Int((p.z())*(TILE*(1<<SHFT)))) & ((NBTILEZ<<SHFT)-1) )

void FMotionWarp (rsVec &p, const float &dt)
{
	int sx, sy, sz, x2, y2, z2;

	sx = Point2STX (p);
	sy = Point2STY (p);
	sz = Point2STZ (p);

	real fx, fy, fz;

	fx = deplfact[sx & ((1 << SHFT) - 1)];
	fy = deplfact[sy & ((1 << SHFT) - 1)];
	fz = deplfact[sz & ((1 << SHFT) - 1)];
	sx >>= SHFT;
	x2 = (sx + 1) & (NBTILEX - 1);
	sy >>= SHFT;
	y2 = (sy + 1) & (NBTILEY - 1);
	sz >>= SHFT;
	z2 = (sz + 1) & (NBTILEZ - 1);

	static rsVec v0, v1, v;

	v0.linearInterp (offset[sz][sy][sx].p, offset[sz][sy][sx + 1].p, fx);
	v1.linearInterp (offset[z2][sy][sx].p, offset[z2][sy][sx + 1].p, fx);
	v.linearInterp (v0, v1, fz);

	v *= dt;
	p += v;
}

#define HVCtrX 0.0
#define HVCtrY 0.0
extern float FireFoc;
extern rsVec FireSrc;
extern rsMatrix FireM;
extern rsVec FireO;

#define FireFocX FireFoc
#define FireFocY FireFoc

#define ProjEX(p) (HVCtrX+FireFocX*(p).x()/(p).y())
#define ProjEY(p) (HVCtrY+FireFocY*(p).z()/(p).y())
#define ProjEZ(ez,p) (ez)=-((p).y()*(1.0f/20.0f)-1.0f)

void AffFMotion ()
{
	int x, y, z;
	PARTICLE *p;
	real xf, yf, zf;
	real ex, ey;
	real ez;
	rsVec p1, p2, o;

	p = &offset[0][0][0];

	glBegin (GL_LINES);

	o = FireSrc;
	o.x() -= TILE * NBTILEX / 2;
	o.y() -= TILE * NBTILEY / 2;
	o.transVec(FireM);
	o += FireO;
	for (z = 0; z < NBTILEZ; z++) {
		zf = z * TILE;
		for (y = 0; y < NBTILEY; y++) {
			yf = y * TILE;
			for (x = 0; x < NBTILEX; x++, p++) {
				xf = x * TILE;
				p1.x() = xf;
				p1.y() = yf;
				p1.z() = zf;
				p2 = p1 + p->p * AFFFACT;
				p1.transVec(FireM);
				p1 += o;
				p2.transVec(FireM);
				p2 += o;
				if (p1.y() > 0 && p2.y() > 0) {
					ex = ProjEX (p1);
					ey = ProjEY (p1);
					ProjEZ (ez, p1);
					glVertex3f (ex, ey, ez);
					ex = ProjEX (p2);
					ey = ProjEY (p2);
					ProjEZ (ez, p2);
					glVertex3f (ex, ey, ez);
				}
			}
		}
	}

	glEnd ();
}
