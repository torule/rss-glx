/*
 * Copyright (C) 2000  Jeremie Allard (Hufo / N.A.A.)
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

// hufo_smoke screen saver

#include <math.h>
#include <stdio.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "driver.h"
#include "rsRand.h"
#include <rsMath/rsMatrix.h>

const char *hack_name = "Hufo/N.A.A. particle system";


#include "FMotion.h"
#include "rsRand.h"

#define HVCtrX 0.0
#define HVCtrY 0.0
float FireFoc = 2.0f;

#define FireFocX FireFoc
#define FireFocY FireFoc

#define ProjEX(p) (HVCtrX+FireFocX*(p).x()/(p).y())
#define ProjEY(p) (HVCtrY+FireFocY*(p).z()/(p).y())
#define ProjEZ(ez,p) (ez)=-((p).y()*(float)(1.0/20.0)-1.0)

double nrnd (double d)		// normal distribution randomizer
{
	double r, v1, v2;

	do {
		v1 = rsRandf (2.0) - 1.0;
		v2 = rsRandf (2.0) - 1.0;
		r = v1 * v1 + v2 * v2;
	} while (r >= 1.0 || r == 0.0);

	r = sqrt (-2.0 * log (r) / r);

	return v1 * d * r;
}

#define NBPARTMAX 2048
#define PARTLIFE 2.0
#define PARTINTERV (PARTLIFE/NBPARTMAX)
#define FIRESIZE 1.0f
#define FIREDS 0.7
#define FIREALPHA 0.15f
#define FIREDA 0.4f
#define FIRECYLR 0.2f

struct Particle {
	rsVec p;		// position
	rsVec v;		// dp/dt
	rsVec s;		// size
	float a;		// alpha
	float t;		// time to death
	float ex, ey, ez;	// screen pos
	float dx, dy;		// screen size
} TblP[NBPARTMAX];

int np;
float LastPartTime;
Bool FireAnim, FireRotate, FireRecalc;
Bool FireStop;
float FireRot;
float FireAng;

rsVec FireSrc (0.0f, 25.0f, -13.0f);
rsVec FireDS1 (6.0f, 0.0f, 0.0f);
rsVec FireDS2 (0.0f, 2.0f, 0.0f);
rsVec FireDir (0.0f, 0.0f, 10.0f);
rsVec FPartA (0.0f, 0.0f, 4.0f);

rsMatrix FireM;
rsVec FireO;

void FireInit ()
{
	//NoiseInit();
	np = 0;
	LastPartTime = 0.0;
	FireAnim = True;
	FireRotate = True;
	FireRot = M_PI / 5.0;
	FireAng = 0.0;
	FireStop = False;
	FireRecalc = True;
	FMotionInit ();
/*	np=1;
	TblP[0].p=FireSrc;
	TblP[0].v.Zero();
	TblP[0].a=1.0;;
	TblP[0].s.x=2.0;
	TblP[0].s.y=2.0;
	TblP[0].s.z=2.0;
	TblP[0].t=-1;*/
}

void CalcFire (float t, float dt)
{
	int n;
	Particle *p = TblP;	//+1;

	n = 0;
	float da = pow (FIREDA, dt);
	//float ds = pow (FIREDS, dt);
	//rsVec v;

	if (FireAnim) {
		FMotionAnimate (dt);
		while (n < np) {
			if ((p->t -= dt) <= 0) {	// kill it
				*p = TblP[--np];
			} else {	// animate it
				//a=p->p; a.z-=5*t;
				//a=FPartA; //+Noise(a)*(p->p.z-FireSrc.z)*1.0; //a.x*=4.0f; a.y*=4.0f;
				p->v += FPartA * dt;
				//GetFMotion(p->p,a); p->v=a+FPartA;
				p->p += p->v * dt;
				//p->p+=FireDir*dt;
				//p->p.z+=dt;
				FMotionWarp (p->p, dt);
				//p->s*=ds;
				//p->s.x=0.67*FIRESIZE+0.4*FIRESIZE*sin(PI*(p->t)/PARTLIFE);
				//p->s.z=0.05*FIRESIZE+0.768*FIRESIZE*(1.0-p->t/PARTLIFE);
				p->a *= da;
				++n;
				++p;
			}
		}
		if (!FireStop)
			while (np < NBPARTMAX && t - LastPartTime >= PARTINTERV) {
				LastPartTime += (float)PARTINTERV;
				p = TblP + (np++);
				p->p = FireSrc + FireDS1 * nrnd (0.25) + FireDS2 * nrnd (0.25);
				p->v = FireDir;
				FMotionWarp (p->p, (t - LastPartTime));
				p->t = PARTLIFE + LastPartTime - t;
				float size = FIRESIZE * (0.5f + nrnd (0.5));
				float alpha = FIREALPHA * (float)pow (size / FIRESIZE, 0.5f);

				p->s.x() = size;
				p->s.y() = size;
				p->s.z() = size;
				p->a = alpha * pow (FIREDA, (t - LastPartTime));
				p->s *= 0.5f + nrnd (0.5);
			}
	} else
		LastPartTime += dt;

	n = 0;
	p = TblP;
	rsVec v;

	if (FireRotate) {
		FireAng += FireRot * dt;
		FireRecalc = True;
	}

	FireM.makeRot (FireAng, 0.0, 0.0, 1.0);
	FireO = FireSrc;
	FireO.transVec(FireM);
	FireO = FireSrc - FireO;
	while (n < np) {
		v = p->p;
		v.transVec(FireM);
		v += FireO;
		v = p->p;
		if (v.y() > 1.0f) {
			p->ex = ProjEX (v);
			p->ey = ProjEY (v);
			ProjEZ (p->ez, v);
			p->dx = FireFocX * p->s.x() / v.y();
			p->dy = FireFocY * p->s.z() / v.y();
		} else
			p->dx = 0;
		++n;
		++p;
	}
}

#define XSTD (4.0/3.0)
float TFire;			// fire time

bool AffGrid = false;
float fr = 1.0f, fg = 1.0f, fb = 1.0f;
float br = 1.0f, bg = 1.0f, bb = 1.0f;

void hack_reshape (xstuff_t * XStuff)
{
	float x = (float)XStuff->windowWidth / (float)XStuff->windowHeight;	// Correct the viewing ratio of the window in the X axis.

	// Window initialization
	glViewport (0, 0, XStuff->windowWidth, XStuff->windowHeight);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	if (x > XSTD)
		gluOrtho2D (-x, x, -1, 1);	// Reset to a 2D screen space.
	else
		gluOrtho2D (-XSTD, XSTD, -XSTD / x, XSTD / x);	// Reset to a 2D screen space.
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
}

// Startup Stuff.
void hack_init (xstuff_t * XStuff)	// Called right after the window is created, and OpenGL is initialized.
{
	hack_reshape (XStuff);

	glCullFace (GL_FRONT);	// reject fliped faces
	glDepthFunc (GL_LESS);

	glBlendFunc (GL_SRC_ALPHA, GL_ONE);
	glEnable (GL_BLEND);

	TFire = 0.0;
	FireInit ();		// initialise fire
}

void hack_cleanup (xstuff_t * XStuff)
{
}

#define fSQRT_3_2 0.8660254038f
void AffParticle (float ex, float ey, float dx, float dy, float a)
{
	float hdx = 0.5f * dx;
	float s32_dy = fSQRT_3_2 * dy;

	glBegin (GL_TRIANGLE_FAN);
	glColor4f (fr, fg, fb, a);
	glVertex2f (ex, ey);
	glColor4f (br, bg, bb, 0.0f);
	glVertex2f (ex - dx, ey);
	glVertex2f (ex - hdx, ey + s32_dy);
	glVertex2f (ex + hdx, ey + s32_dy);
	glVertex2f (ex + dx, ey);
	glVertex2f (ex + hdx, ey - s32_dy);
	glVertex2f (ex - hdx, ey - s32_dy);
	glVertex2f (ex - dx, ey);
	glEnd ();
}

// Draw all the scene related stuff.
void hack_draw (xstuff_t * XStuff, double currentTime, float frameTime)
{
	TFire += frameTime;
	CalcFire (TFire, frameTime);	// animate the fire

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	int n;

	glEnable (GL_BLEND);
	Particle *p = TblP;

	n = np;
	while (n) {
		if (p->dx)
			AffParticle (p->ex, p->ey, p->dx, p->dy, p->a);

		++p;
		--n;
	}

	glDisable (GL_BLEND);

	if (AffGrid) {
		glColor3f (1.0f, 1.0f, 1.0f);
		AffFMotion ();
	}
}

void hack_handle_opts (int argc, char **argv)
{
	int change_flag = 0;

	while (1) {
		int c;

#ifdef HAVE_GETOPT_H
		static struct option long_options[] = {
			{"help", 0, 0, 'h'},
			DRIVER_OPTIONS_LONG 
			{"foreground", 1, 0, 'f'},
			{"background", 1, 0, 'b'},
			{0, 0, 0, 0}
		};

		c = getopt_long (argc, argv, DRIVER_OPTIONS_SHORT "hf:b:", long_options, NULL);
#else
		c = getopt (argc, argv, DRIVER_OPTIONS_SHORT "hf:b:");
#endif
		if (c == -1)
			break;

		switch (c) {
		DRIVER_OPTIONS_CASES case 'h':
			printf ("%s:"
#ifndef HAVE_GETOPT_H
				" Not built with GNU getopt.h, long options *NOT* enabled."
#endif
				"\n" DRIVER_OPTIONS_HELP "\t--foreground/-f\n" "\t--background/-b\n", argv[0]);
			exit (1);
		case 'f':{
				int color = strtol_minmaxdef (optarg, 16, 0, 16777216, 0, 0xffffff, "--foreground: ");

				change_flag = 1;

				fr = ((color & 0xFF0000) >> 16) / 256.0f;
				fg = ((color & 0xFF00) >> 8) / 256.0f;
				fb = (color & 0xFF) / 256.0f;
			}
			break;
		case 'b':{
				int color = strtol_minmaxdef (optarg, 16, 0, 16777216, 0, 0xffffff, "--background: ");

				change_flag = 1;

				br = ((color & 0xFF0000) >> 16) / 256.0f;
				bg = ((color & 0xFF00) >> 8) / 256.0f;
				bb = (color & 0xFF) / 256.0f;
			}
			break;
		}
	}

	if (!change_flag) {
		do {
			fr = rsRandf (256) / 256;
			fg = rsRandf (256) / 256;
			fb = rsRandf (256) / 256;

			br = rsRandi (256) / 256;
			bg = rsRandi (256) / 256;
			bb = rsRandi (256) / 256;
		} while ((fr + fg + fb < 1) && (br + bg + bb < 1));
	}
}
