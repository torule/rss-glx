/*
 * Copyright (C) 2002  Terence M. Welsh
 * Ported to Linux by Tugrul Galatali <tugrul@galatali.com>
 *
 * Field Lines is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 *
 * Field Lines is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Field Lines screensaver

#include <math.h>
#include <stdio.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "driver.h"
#include "rsDefines.h"
#include "rsRand.h"

const char *hack_name = "fieldlines";

float wide, high, deep;

class ion;

// Globals
int readyToDraw = 0;
ion *ions;
float aspectRatio;
float elapsedTime;

// Parameters edited in the dialog box
int dIons;
int dStepSize;
int dMaxSteps;
int dWidth;
int dSpeed;
int dConstwidth;
int dElectric;

class ion {
      public:
	float charge;
	float xyz[3];
	float vel[3];
	float angle;
	float anglevel;

	  ion ();
	 ~ion () {};
	void update ();
};

ion::ion ()
{
	charge = float (rsRandi (2));

	if (rsRandi(2))
		charge = -1.0f;
	else
		charge = 1.0f;

	xyz[0] = rsRandf (2.0f * wide) - wide;
	xyz[1] = rsRandf (2.0f * high) - high;
	xyz[2] = rsRandf (2.0f * deep) - deep;
	vel[0] = rsRandf (float (dSpeed) * 4.0f) - (float (dSpeed) * 2.0f);
	vel[1] = rsRandf (float (dSpeed) * 4.0f) - (float (dSpeed) * 2.0f);
	vel[2] = rsRandf (float (dSpeed) * 4.0f) - (float (dSpeed) * 2.0f);

	angle = 0.0f;
	anglevel = 0.0005f * float (dSpeed) + 0.0005f * rsRandf (float (dSpeed));
}

void
  ion::update ()
{
	xyz[0] += vel[0] * elapsedTime;
	xyz[1] += vel[1] * elapsedTime;
	xyz[2] += vel[2] * elapsedTime;
	if (xyz[0] > wide)
		vel[0] -= 0.1f * float (dSpeed);

	if (xyz[0] < -wide)
		vel[0] += 0.1f * float (dSpeed);

	if (xyz[1] > high)
		vel[1] -= 0.1f * float (dSpeed);

	if (xyz[1] < -high)
		vel[1] += 0.1f * float (dSpeed);

	if (xyz[2] > deep)
		vel[2] -= 0.1f * float (dSpeed);

	if (xyz[2] < -deep)
		vel[2] += 0.1f * float (dSpeed);

	angle += anglevel;
	if (angle > PIx2)
		angle -= PIx2;
}

void drawfieldline (int source, float x, float y, float z)
{
	int i, j;
	float charge;
	float repulsion;
	float dist, distsquared, distrec;
	float xyz[3];
	float lastxyz[3];
	float dir[3];
	float end[3];
	float tempvec[3];
	float r, g, b;
	float lastr, lastg, lastb;
	static float brightness = 10000.0f;

	charge = ions[source].charge;
	lastxyz[0] = ions[source].xyz[0];
	lastxyz[1] = ions[source].xyz[1];
	lastxyz[2] = ions[source].xyz[2];
	dir[0] = x;
	dir[1] = y;
	dir[2] = z;

	// Do the first segment
	r = float (fabs (dir[2])) * brightness;
	g = float (fabs (dir[0])) * brightness;
	b = float (fabs (dir[1])) * brightness;

	if (r > 1.0f)
		r = 1.0f;
	if (g > 1.0f)
		g = 1.0f;
	if (b > 1.0f)
		b = 1.0f;
	lastr = r;
	lastg = g;
	lastb = b;
	glColor3f (r, g, b);
	xyz[0] = lastxyz[0] + dir[0];
	xyz[1] = lastxyz[1] + dir[1];
	xyz[2] = lastxyz[2] + dir[2];
	if (dElectric) {
		xyz[0] += rsRandf (float (dStepSize) * 0.2f) - (float (dStepSize) * 0.1f);
		xyz[1] += rsRandf (float (dStepSize) * 0.2f) - (float (dStepSize) * 0.1f);
		xyz[2] += rsRandf (float (dStepSize) * 0.2f) - (float (dStepSize) * 0.1f);
	}
	if (!dConstwidth)
		glLineWidth ((xyz[2] + 300.0f) * 0.000333f * float (dWidth));

	glBegin (GL_LINE_STRIP);
	glColor3f (lastr, lastg, lastb);
	glVertex3fv (lastxyz);
	glColor3f (r, g, b);
	glVertex3fv (xyz);
	if (!dConstwidth)
		glEnd ();

	for (i = 0; i < int (dMaxSteps); i++) {
		dir[0] = 0.0f;
		dir[1] = 0.0f;
		dir[2] = 0.0f;
		for (j = 0; j < int (dIons); j++) {
			repulsion = charge * ions[j].charge;
			tempvec[0] = xyz[0] - ions[j].xyz[0];
			tempvec[1] = xyz[1] - ions[j].xyz[1];
			tempvec[2] = xyz[2] - ions[j].xyz[2];
			distsquared = tempvec[0] * tempvec[0] + tempvec[1] * tempvec[1] + tempvec[2] * tempvec[2];
			dist = float (sqrt (distsquared));

			if (dist < float (dStepSize) && i > 2) {
				end[0] = ions[j].xyz[0];
				end[1] = ions[j].xyz[1];
				end[2] = ions[j].xyz[2];
				i = 10000;
			}
			tempvec[0] /= dist;
			tempvec[1] /= dist;
			tempvec[2] /= dist;
			if (distsquared < 1.0f)
				distsquared = 1.0f;
			dir[0] += tempvec[0] * repulsion / distsquared;
			dir[1] += tempvec[1] * repulsion / distsquared;
			dir[2] += tempvec[2] * repulsion / distsquared;
		}
		lastr = r;
		lastg = g;
		lastb = b;
		r = float (fabs (dir[2])) * brightness;
		g = float (fabs (dir[0])) * brightness;
		b = float (fabs (dir[1])) * brightness;

		if (dElectric) {
			r *= 10.0f;
			g *= 10.0f;
			b *= 10.0f;;
			if (r > b * 0.5f)
				r = b * 0.5f;
			if (g > b * 0.3f)
				g = b * 0.3f;
		}
		if (r > 1.0f)
			r = 1.0f;
		if (g > 1.0f)
			g = 1.0f;
		if (b > 1.0f)
			b = 1.0f;
		distsquared = dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2];
		distrec = float (dStepSize) / float (sqrt (distsquared));

		dir[0] *= distrec;
		dir[1] *= distrec;
		dir[2] *= distrec;
		if (dElectric) {
			dir[0] += rsRandf (float (dStepSize)) - (float (dStepSize) * 0.5f);
			dir[1] += rsRandf (float (dStepSize)) - (float (dStepSize) * 0.5f);
			dir[2] += rsRandf (float (dStepSize)) - (float (dStepSize) * 0.5f);
		}
		lastxyz[0] = xyz[0];
		lastxyz[1] = xyz[1];
		lastxyz[2] = xyz[2];
		xyz[0] += dir[0];
		xyz[1] += dir[1];
		xyz[2] += dir[2];
		if (!dConstwidth) {
			glLineWidth ((xyz[2] + 300.0f) * 0.000333f * float (dWidth));

			glBegin (GL_LINE_STRIP);
		}
		glColor3f (lastr, lastg, lastb);
		glVertex3fv (lastxyz);
		if (i != 10000) {
			if (i == (int (dMaxSteps) - 1))
				glColor3f (0.0f, 0.0f, 0.0f);
			else
				glColor3f (r, g, b);
			glVertex3fv (xyz);
			if (i == (int (dMaxSteps) - 1))
				glEnd ();
		}
	}
	if (i == 10001) {
		glColor3f (r, g, b);
		glVertex3fv (end);
		glEnd ();
	}
}

void hack_draw (xstuff_t * XStuff, double currentTime, float frameTime)
{
	int i;
	static float s = float (sqrt (float (dStepSize) * float (dStepSize) * 0.333f));

	elapsedTime = frameTime;

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	for (i = 0; i < dIons; i++)
		ions[i].update ();

	for (i = 0; i < dIons; i++) {
		drawfieldline (i, s, s, s);
		drawfieldline (i, s, s, -s);
		drawfieldline (i, s, -s, s);
		drawfieldline (i, s, -s, -s);
		drawfieldline (i, -s, s, s);
		drawfieldline (i, -s, s, -s);
		drawfieldline (i, -s, -s, s);
		drawfieldline (i, -s, -s, -s);
	}
}

void hack_reshape (xstuff_t * XStuff)
{
	// Window initialization
	glViewport (0, 0, XStuff->windowWidth, XStuff->windowHeight);

	aspectRatio = float (XStuff->windowWidth) / float (XStuff->windowHeight);
	if (XStuff->windowWidth > XStuff->windowHeight) {
		high = deep = 160.0f;
		wide = high * aspectRatio;
	} else {
		wide = deep = 160.0f;
		high = wide * aspectRatio;
	}

	glEnable (GL_DEPTH_TEST);
	glEnable (GL_LINE_SMOOTH);
	glClearColor (0.0f, 0.0f, 0.0f, 1.0f);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	gluPerspective (60.0, aspectRatio, 50, 3000);

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
	glTranslatef (0.0, 0.0, -2 * deep);
}

void hack_init (xstuff_t * XStuff)
{
	hack_reshape (XStuff);

	if (dConstwidth)
		glLineWidth (float (dWidth) * 0.1f);

	ions = new ion[dIons];
}

void hack_cleanup (xstuff_t * XStuff)
{
}

void hack_handle_opts (int argc, char **argv)
{
	dIons = 6;
	dStepSize = 10;
	dMaxSteps = 300;
	dWidth = 30;
	dSpeed = 10;
	dConstwidth = 0;
	dElectric = 0;

	while (1) {
		int c;

#ifdef HAVE_GETOPT_H
		static struct option long_options[] = {
			{"help", 0, 0, 'h'},
			DRIVER_OPTIONS_LONG {"ions", 1, 0, 'i'},
			{"stepsize", 1, 0, 's'},
			{"maxsteps", 1, 0, 'm'},
			{"width", 1, 0, 'w'},
			{"speed", 1, 0, 'S'},
			{"constwidth", 0, 0, 'c'},
			{"no-constwidth", 0, 0, 'C'},
			{"electric", 0, 0, 'e'},
			{"no-electric", 0, 0, 'E'},
			{0, 0, 0, 0}
		};

		c = getopt_long (argc, argv, DRIVER_OPTIONS_SHORT "hi:s:m:w:S:cCeE", long_options, NULL);
#else
		c = getopt (argc, argv, DRIVER_OPTIONS_SHORT "hrx:i:s:m:w:S:cCeE");
#endif
		if (c == -1)
			break;

		switch (c) {
			DRIVER_OPTIONS_CASES case 'h':printf ("%s:"
#ifndef HAVE_GETOPT_H
							      " Not built with GNU getopt.h, long options *NOT* enabled."
#endif
							      "\n" DRIVER_OPTIONS_HELP "\t--ions/-i <arg>\n" "\t--stepsize/-s <arg>\n" "\t--maxsteps/-m <arg>\n"
							      "\t--width/-w <arg>\n" "\t--speed/-S <arg>\n" "\t--constwidth/-c\n" "\t--no-constwidth/-C\n" "\t--electric/-e\n"
							      "\t--no-electric/-E\n", argv[0]);
			exit (1);
		case 'i':
			dIons = strtol_minmaxdef (optarg, 10, 1, 100, 1, 4, "--ions: ");
			break;
		case 's':
			dStepSize = strtol_minmaxdef (optarg, 10, 1, 100, 1, 15, "--stepsize: ");
			break;
		case 'm':
			dMaxSteps = strtol_minmaxdef (optarg, 10, 1, 1000, 1, 100, "--maxsteps: ");
			break;
		case 'w':
			dWidth = strtol_minmaxdef (optarg, 10, 1, 100, 1, 30, "--width: ");
			break;
		case 'S':
			dSpeed = strtol_minmaxdef (optarg, 10, 1, 100, 1, 10, "--speed: ");
			break;
		case 'c':
			dConstwidth = 1;
			break;
		case 'C':
			dConstwidth = 0;
			break;
		case 'e':
			dElectric = 1;
			break;
		case 'E':
			dElectric = 0;
			break;
		}
	}
}
