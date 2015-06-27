/*
 * Copyright (C) 2002  Terence M. Welsh
 * Ported to Linux by Tugrul Galatali <tugrul@galatali.com>
 *
 * Cyclone is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 *
 * Cyclone is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Cyclone screen saver

#include <math.h>
#include <stdio.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <time.h>
#include <sys/time.h>

#include "driver.h"
#include "rgbhsl.h"
#include "rsDefines.h"
#include "rsRand.h"
#include "rsMath/rsVec.h"

const char *hack_name = "cyclone";

class cyclone;
class particle;

#define wide 200
#define high 200

float elapsedTime = 0;
int lastTime = -1;

// Parameters edited in the dialog box
int dCyclones;
int dParticles;
int dSize;
int dComplexity;
int dSpeed;
int dStretch;
int dShowCurves;

// Other globals
cyclone **cyclones;
particle **particles;
float fact[13];

// useful factorial function
int factorial (int x)
{
	int returnval = 1;

	if (x == 0)
		return (1);
	else {
		do {
			returnval *= x;
			x -= 1;
		}
		while (x != 0);
	}
	return (returnval);
}

class cyclone {
      public:
	float **targetxyz;
	float **xyz;
	float **oldxyz;
	float *targetWidth;
	float *width;
	float *oldWidth;
	float targethsl[3];
	float hsl[3];
	float oldhsl[3];
	float **xyzChange;
	float **widthChange;
	float hslChange[2];

	  cyclone ();
	  virtual ~ cyclone () {
	};
	void update ();
};

cyclone::cyclone ()
{
	int i;

	// Initialize position stuff
	targetxyz = new float *[dComplexity + 3];
	xyz = new float *[dComplexity + 3];
	oldxyz = new float *[dComplexity + 3];

	for (i = 0; i < int (dComplexity) + 3; i++) {
		targetxyz[i] = new float[3];
		xyz[i] = new float[3];
		oldxyz[i] = new float[3];
	}
	xyz[dComplexity + 2][0] = rsRandf (float (wide * 2)) - float (wide);
	xyz[dComplexity + 2][1] = float (high);
	xyz[dComplexity + 2][2] = rsRandf (float (wide * 2)) - float (wide);

	xyz[dComplexity + 1][0] = xyz[dComplexity + 2][0];
	xyz[dComplexity + 1][1] = rsRandf (float (high / 3)) + float (high / 4);

	xyz[dComplexity + 1][2] = xyz[dComplexity + 2][2];
	for (i = dComplexity; i > 1; i--) {
		xyz[i][0] = xyz[i + 1][0] + rsRandf (float (wide)) - float (wide / 2);
		xyz[i][1] = rsRandf (float (high * 2)) - float (high);
		xyz[i][2] = xyz[i + 1][2] + rsRandf (float (wide)) - float (wide / 2);
	}
	xyz[1][0] = xyz[2][0] + rsRandf (float (wide / 2)) - float (wide / 4);
	xyz[1][1] = -rsRandf (float (high / 2)) - float (high / 4);
	xyz[1][2] = xyz[2][2] + rsRandf (float (wide / 2)) - float (wide / 4);
	xyz[0][0] = xyz[1][0] + rsRandf (float (wide / 8)) - float (wide / 16);
	xyz[0][1] = float (-high);
	xyz[0][2] = xyz[1][2] + rsRandf (float (wide / 8)) - float (wide / 16);

	// Initialize width stuff
	targetWidth = new float[dComplexity + 3];
	width = new float[dComplexity + 3];
	oldWidth = new float[dComplexity + 3];

	width[dComplexity + 2] = rsRandf (175.0f) + 75.0f;
	width[dComplexity + 1] = rsRandf (60.0f) + 15.0f;
	for (i = dComplexity; i > 1; i--)
		width[i] = rsRandf (25.0f) + 15.0f;
	width[1] = rsRandf (25.0f) + 5.0f;
	width[0] = rsRandf (15.0f) + 5.0f;
	// Initialize transition stuff
	xyzChange = new float *[dComplexity + 3];
	widthChange = new float *[dComplexity + 3];

	for (i = 0; i < (dComplexity + 3); i++) {
		xyzChange[i] = new float[2];	// 0 = step 1 = total steps
		widthChange[i] = new float[2];

		xyzChange[i][0] = 0;
		xyzChange[i][1] = 0;
		widthChange[i][0] = 0;
		widthChange[i][1] = 0;
	}
	// Initialize color stuff
	hsl[0] = oldhsl[0] = rsRandf (1.0f);
	hsl[1] = oldhsl[1] = rsRandf (1.0f);
	hsl[2] = oldhsl[2] = 0.0f;
	targethsl[0] = rsRandf (1.0f);
	targethsl[1] = rsRandf (1.0f);
	targethsl[2] = 1.0f;
	hslChange[0] = 0;
	hslChange[1] = 10;
}

void
  cyclone::update ()
{
	int i;
	int temp;
	float between;
	float diff;
	int direction;
	float point[3];
	float step;
	float blend;

	// update cyclone's path
	temp = dComplexity + 2;
	if (xyzChange[temp][0] >= xyzChange[temp][1]) {
		oldxyz[temp][0] = xyz[temp][0];
		oldxyz[temp][1] = xyz[temp][1];
		oldxyz[temp][2] = xyz[temp][2];
		targetxyz[temp][0] = rsRandf (float (wide * 2)) - float (wide);
		targetxyz[temp][1] = float (high);
		targetxyz[temp][2] = rsRandf (float (wide * 2)) - float (wide);

		xyzChange[temp][0] = 0;
		xyzChange[temp][1] = rsRandf (150.0f / dSpeed) + 75.0f / dSpeed;
	}
	temp = dComplexity + 1;
	if (xyzChange[temp][0] >= xyzChange[temp][1]) {
		oldxyz[temp][0] = xyz[temp][0];
		oldxyz[temp][1] = xyz[temp][1];
		oldxyz[temp][2] = xyz[temp][2];
		targetxyz[temp][0] = xyz[temp + 1][0];
		targetxyz[temp][1] = rsRandf (float (high / 3)) + float (high / 4);

		targetxyz[temp][2] = xyz[temp + 1][2];
		xyzChange[temp][0] = 0;
		xyzChange[temp][1] = rsRandf (100.0f / dSpeed) + 75.0f / dSpeed;
	}
	for (i = dComplexity; i > 1; i--) {
		if (xyzChange[i][0] >= xyzChange[i][1]) {
			oldxyz[i][0] = xyz[i][0];
			oldxyz[i][1] = xyz[i][1];
			oldxyz[i][2] = xyz[i][2];
			targetxyz[i][0] = targetxyz[i + 1][0] + (targetxyz[i + 1][0] - targetxyz[i + 2][0]) / 2.0f + rsRandf (float (wide / 2)) - float (wide / 4);
			targetxyz[i][1] = (targetxyz[i + 1][1] + targetxyz[i - 1][1]) / 2.0f + rsRandf (float (high / 8)) - float (high / 16);
			targetxyz[i][2] = targetxyz[i + 1][2] + (targetxyz[i + 1][2] - targetxyz[i + 2][2]) / 2.0f + rsRandf (float (wide / 2)) - float (wide / 4);

			if (targetxyz[i][1] > high)
				targetxyz[i][1] = high;
			if (targetxyz[i][1] < -high)
				targetxyz[i][1] = -high;
			xyzChange[i][0] = 0;
			xyzChange[i][1] = rsRandf (75.0f / dSpeed) + 50.0f / dSpeed;
		}
	}
	if (xyzChange[1][0] >= xyzChange[1][1]) {
		oldxyz[1][0] = xyz[1][0];
		oldxyz[1][1] = xyz[1][1];
		oldxyz[1][2] = xyz[1][2];
		targetxyz[1][0] = targetxyz[2][0] + rsRandf (float (wide / 2)) - float (wide / 4);
		targetxyz[1][1] = -rsRandf (float (high / 2)) - float (high / 4);
		targetxyz[1][2] = targetxyz[2][2] + rsRandf (float (wide / 2)) - float (wide / 4);

		xyzChange[1][0] = 0;
		xyzChange[1][1] = rsRandf (50.0f / dSpeed) + 30.0f / dSpeed;
	}
	if (xyzChange[0][0] >= xyzChange[0][1]) {
		oldxyz[0][0] = xyz[0][0];
		oldxyz[0][1] = xyz[0][1];
		oldxyz[0][2] = xyz[0][2];
		targetxyz[0][0] = xyz[1][0] + rsRandf (float (wide / 8)) - float (wide / 16);
		targetxyz[0][1] = float (-high);
		targetxyz[0][2] = xyz[1][2] + rsRandf (float (wide / 8)) - float (wide / 16);

		xyzChange[0][0] = 0;
		xyzChange[0][1] = rsRandf (100.0f / dSpeed) + 75.0f / dSpeed;
	}
	for (i = 0; i < (dComplexity + 3); i++) {
		between = xyzChange[i][0] / xyzChange[i][1] * PIx2;

		between = (1.0f - float (cos (between)))/2.0f;
		xyz[i][0] = ((targetxyz[i][0] - oldxyz[i][0]) * between) + oldxyz[i][0];
		xyz[i][1] = ((targetxyz[i][1] - oldxyz[i][1]) * between) + oldxyz[i][1];
		xyz[i][2] = ((targetxyz[i][2] - oldxyz[i][2]) * between) + oldxyz[i][2];
		xyzChange[i][0] += elapsedTime;
	}

	// Update cyclone's widths
	temp = dComplexity + 2;
	if (widthChange[temp][0] >= widthChange[temp][1]) {
		oldWidth[temp] = width[temp];
		targetWidth[temp] = rsRandf (225.0f) + 75.0f;
		widthChange[temp][0] = 0;
		widthChange[temp][1] = rsRandf (50.0f / dSpeed) + 5000 / dSpeed;
	}
	temp = dComplexity + 1;
	if (widthChange[temp][0] >= widthChange[temp][1]) {
		oldWidth[temp] = width[temp];
		targetWidth[temp] = rsRandf (100.0f) + 15.0f;
		widthChange[temp][0] = 0;
		widthChange[temp][1] = rsRandi (50.0f / dSpeed) + 50.0f / dSpeed;
	}
	for (i = dComplexity; i > 1; i--) {
		if (widthChange[i][0] >= widthChange[i][1]) {
			oldWidth[i] = width[i];
			targetWidth[i] = rsRandf (50.0f) + 15.0f;
			widthChange[i][0] = 0;
			widthChange[i][1] = rsRandi (50.0f / dSpeed) + 40.0f / dSpeed;
		}
	}
	if (widthChange[1][0] >= widthChange[1][1]) {
		oldWidth[1] = width[1];
		targetWidth[1] = rsRandf (40.0f) + 5.0f;
		widthChange[1][0] = 0;
		widthChange[1][1] = rsRandi (50.0f / dSpeed) + 30.0f / dSpeed;
	}
	if (widthChange[0][0] >= widthChange[0][1]) {
		oldWidth[0] = width[0];
		targetWidth[0] = rsRandf (30.0f) + 5.0f;
		widthChange[0][0] = 0;
		widthChange[0][1] = rsRandi (50.0f / dSpeed) + 20.0f / dSpeed;
	}
	for (i = 0; i < (dComplexity + 3); i++) {
		between = float (widthChange[i][0]) / float (widthChange[i][1]);

		width[i] = ((targetWidth[i] - oldWidth[i]) * between) + oldWidth[i];
		widthChange[i][0] += elapsedTime;
	}

	// Update cyclones color
	if (hslChange[0] >= hslChange[1]) {
		oldhsl[0] = hsl[0];
		oldhsl[1] = hsl[1];
		oldhsl[2] = hsl[2];
		targethsl[0] = rsRandf (1.0f);
		targethsl[1] = rsRandf (1.0f);
		targethsl[2] = rsRandf (1.0f) + 0.5f;
		if (targethsl[2] > 1.0f)
			targethsl[2] = 1.0f;
		hslChange[0] = 0;
		hslChange[1] = rsRandf (30.0f) + 2.0f;
	}
	between = hslChange[0] / hslChange[1];

	diff = targethsl[0] - oldhsl[0];
	direction = 0;
	if ((targethsl[0] > oldhsl[0] && diff > 0.5f)
	    || (targethsl[0] < oldhsl[0] && diff < -0.5f))
		if (diff > 0.5f)
			direction = 1;
	hslTween (oldhsl[0], oldhsl[1], oldhsl[2], targethsl[0], targethsl[1], targethsl[2], between, direction, hsl[0], hsl[1], hsl[2]);
	hslChange[0] += elapsedTime;

	if (dShowCurves) {
		glDisable (GL_LIGHTING);
		glColor3f (0.0f, 1.0f, 0.0f);
		glBegin (GL_LINE_STRIP);
		for (step = 0.0; step < 1.0; step += 0.02f) {
			point[0] = point[1] = point[2] = 0.0f;
			for (i = 0; i < (dComplexity + 3); i++) {
				blend = fact[dComplexity + 2] / (fact[i]
								 * fact[dComplexity + 2 - i]) * pow (step, float

												       (i))
				* pow ((1.0f - step), float (dComplexity + 2 - i));

				point[0] += xyz[i][0] * blend;
				point[1] += xyz[i][1] * blend;
				point[2] += xyz[i][2] * blend;
			}
			glVertex3fv (point);
		}
		glEnd ();
		glColor3f (1.0f, 0.0f, 0.0f);
		glBegin (GL_LINE_STRIP);
		for (i = 0; i < (dComplexity + 3); i++)
			glVertex3fv (&xyz[i][0]);
		glEnd ();
		glEnable (GL_LIGHTING);
	}
}

class particle {
      public:
	float r, g, b;
	float xyz[3], lastxyz[3];
	float width;
	float step;
	float spinAngle;
	cyclone *cy;

	particle (cyclone *);
	virtual ~ particle () {
	};
	void init ();
	void update ();
};

particle::particle (cyclone * c)
{
	cy = c;
	init ();
}

void
  particle::init ()
{
	width = rsRandf (0.8f) + 0.2f;
	step = 0.0f;
	spinAngle = rsRandf (360);
	hsl2rgb (cy->hsl[0], cy->hsl[1], cy->hsl[2], r, g, b);
}

void particle::update ()
{
	int i;
	float scale = 0, temp;
	float newStep;
	float newSpinAngle;
	float cyWidth;
	float between;
	rsVec dir, crossVec, up(0.0f, 1.0f, 0.0f);
	float tiltAngle;
	float blend;

	lastxyz[0] = xyz[0];
	lastxyz[1] = xyz[1];
	lastxyz[2] = xyz[2];
	if (step > 1.0f)
		init ();
	xyz[0] = xyz[1] = xyz[2] = 0.0f;
	for (i = 0; i < (dComplexity + 3); i++) {
		blend = fact[dComplexity + 2] / (fact[i]
						 * fact[dComplexity + 2 - i]) * pow (step, float (i))
		* pow ((1.0f - step), float (dComplexity + 2 - i));

		xyz[0] += cy->xyz[i][0] * blend;
		xyz[1] += cy->xyz[i][1] * blend;
		xyz[2] += cy->xyz[i][2] * blend;
	}
	dir[0] = dir[1] = dir[2] = 0.0f;
	for (i = 0; i < (dComplexity + 3); i++) {
		blend = fact[dComplexity + 2] / (fact[i]
						 * fact[dComplexity + 2 - i]) * pow (step - 0.01f, float (i))
		* pow ((1.0f - (step - 0.01f)), float (dComplexity + 2 - i));

		dir[0] += cy->xyz[i][0] * blend;
		dir[1] += cy->xyz[i][1] * blend;
		dir[2] += cy->xyz[i][2] * blend;
	}
	dir[0] = xyz[0] - dir[0];
	dir[1] = xyz[1] - dir[1];
	dir[2] = xyz[2] - dir[2];
	dir.normalize();
	crossVec.cross(dir, up);
	tiltAngle = -acos (dir.dot(up)) * 180.0f / PI;
	i = int (step * (float (dComplexity) + 2.0f));

	if (i >= (dComplexity + 2))
		i = dComplexity + 1;
	between = (step - (float (i) / float (dComplexity + 2))) * float (dComplexity + 2);

	cyWidth = cy->width[i] * (1.0f - between) + cy->width[i + 1] * (between);
	newStep = (0.2f * float (dSpeed) * elapsedTime) / (width * width * cyWidth);
	step += newStep;
	newSpinAngle = (40.0f * float (dSpeed)) / (width * cyWidth);
	spinAngle += newSpinAngle;
	if (dStretch) {
		scale = width * cyWidth * newSpinAngle * 0.02f;
		temp = cyWidth * 2.0f / float (dSize);

		if (scale > temp)
			scale = temp;
		if (scale < 3.0f)
			scale = 3.0f;
	}
	glColor3f (r, g, b);
	glPushMatrix ();
	glLoadIdentity ();
	glTranslatef (xyz[0], xyz[1], xyz[2]);
	glRotatef (tiltAngle, crossVec[0], crossVec[1], crossVec[2]);
	glRotatef (spinAngle, 0, 1, 0);
	glTranslatef (width * cyWidth, 0, 0);
	if (dStretch)
		glScalef (1.0f, 1.0f, scale);
	glCallList (1);
	glPopMatrix ();
}

void hack_draw (xstuff_t * XStuff, double currentTime, float frameTime)
{
	int i, j;

	elapsedTime = frameTime;

	glMatrixMode (GL_MODELVIEW);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	for (i = 0; i < dCyclones; i++) {
		cyclones[i]->update ();
		for (j = (i * dParticles); j < ((i + 1) * dParticles); j++) {
			particles[j]->update ();
		}
	}
}

void hack_reshape (xstuff_t * XStuff)
{
	// Window initialization
	glViewport (0, 0, XStuff->windowWidth, XStuff->windowHeight);

	glEnable (GL_DEPTH_TEST);
	glFrontFace (GL_CCW);
	glEnable (GL_CULL_FACE);
	glClearColor (0.0, 0.0, 0.0, 1.0);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	gluPerspective (80.0, float (XStuff->windowWidth) / float (XStuff->windowHeight), 50, 3000);

	if (!rsRandi (500)) {	// Easter egg view
		glRotatef (90, 1, 0, 0);
		glTranslatef (0.0f, -(wide * 2), 0.0f);
	} else			// Normal view
		glTranslatef (0.0f, 0.0f, -(wide * 2));
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();

	glNewList (1, GL_COMPILE);
	GLUquadricObj *qobj = gluNewQuadric ();
	gluSphere (qobj, float (dSize) / 4.0f, 3, 2);

	gluDeleteQuadric (qobj);
	glEndList ();

	glEnable (GL_LIGHTING);
	glEnable (GL_LIGHT0);
	float ambient[4] = {
		0.25f,
		0.25f,
		0.25f,
		0.0f
	};
	float diffuse[4] = {
		1.0f,
		1.0f,
		1.0f,
		0.0f
	};
	float specular[4] = {
		1.0f,
		1.0f,
		1.0f,
		0.0f
	};
	float position[4] = { float (wide * 2), -float (high), float (wide * 2),
		0.0f
	};

	glLightfv (GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv (GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv (GL_LIGHT0, GL_SPECULAR, specular);
	glLightfv (GL_LIGHT0, GL_POSITION, position);
	glEnable (GL_COLOR_MATERIAL);
	glMaterialf (GL_FRONT, GL_SHININESS, 20.0f);
	glColorMaterial (GL_FRONT, GL_SPECULAR);
	glColor3f (0.7f, 0.7f, 0.7f);
	glColorMaterial (GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
}

void hack_init (xstuff_t * XStuff)
{
	int i, j;

	hack_reshape (XStuff);

	// Initialize cyclones and their particles
	for (i = 0; i < 13; i++)
		fact[i] = float (factorial (i));

	cyclones = new cyclone *[dCyclones];
	particles = new particle *[dParticles * dCyclones];
	for (i = 0; i < dCyclones; i++) {
		cyclones[i] = new cyclone;
		for (j = i * dParticles; j < ((i + 1) * dParticles); j++)
			particles[j] = new particle (cyclones[i]);
	}
}

void hack_cleanup (xstuff_t * XStuff)
{
	glDeleteLists (1, 1);
}

void hack_handle_opts (int argc, char **argv)
{
	dCyclones = 1;
	dParticles = 400;
	dSize = 7;
	dComplexity = 3;
	dSpeed = 10;
	dStretch = TRUE;
	dShowCurves = FALSE;

	while (1) {
		int c;

#ifdef HAVE_GETOPT_H
		static struct option long_options[] = {
			{"help", 0, 0, 'h'},
			DRIVER_OPTIONS_LONG {"cyclones", 1, 0, 'c'},
			{"particles", 1, 0, 'p'},
			{"size", 1, 0, 'i'},
			{"complexity", 1, 0, 'C'},
			{"speed", 1, 0, 'e'},
			{"stretch", 0, 0, 's'},
			{"no-stretch", 0, 0, 'S'},
			{"showcurves", 0, 0, 'v'},
			{"no-showcurves", 0, 0, 'V'},
			{0, 0, 0, 0}
		};

		c = getopt_long (argc, argv, DRIVER_OPTIONS_SHORT "hrx:c:p:i:C:e:sSvV", long_options, NULL);
#else
		c = getopt (argc, argv, DRIVER_OPTIONS_SHORT "hc:p:i:C:e:sSvV");
#endif
		if (c == -1)
			break;

		switch (c) {
			DRIVER_OPTIONS_CASES case 'h':printf ("%s:"
#ifndef HAVE_GETOPT_H
							      " Not built with GNU getopt.h, long options *NOT* enabled."
#endif
							      "\n" DRIVER_OPTIONS_HELP "\t--cyclones/-c <arg>\n" "\t--particles/-p <arg>\n" "\t--size/-i <arg>\n"
							      "\t--complexity/-C <arg>\n" "\t--speed/-e <arg>\n" "\t--stretch/-s\n" "\t--no-stretch/-S\n" "\t--showcurves/-v\n"
							      "\t--no-showcurves/-V\n", argv[0]);
			exit (1);
		case 'c':
			dCyclones = strtol_minmaxdef (optarg, 10, 1, 10, 1, 1, "--cyclones: ");
			break;
		case 'p':
			dParticles = strtol_minmaxdef (optarg, 10, 1, 10000, 1, 400, "--particles: ");
			break;
		case 'i':
			dSize = strtol_minmaxdef (optarg, 10, 1, 100, 1, 7, "--size: ");
			break;
		case 'C':
			dComplexity = strtol_minmaxdef (optarg, 10, 1, 10, 1, 3, "--complexity: ");
			break;
		case 'e':
			dSpeed = strtol_minmaxdef (optarg, 10, 1, 100, 1, 10, "--speed: ");
			break;
		case 's':
			dStretch = 1;
			break;
		case 'S':
			dStretch = 0;
			break;
		case 'v':
			dShowCurves = 1;
			break;
		case 'V':
			dShowCurves = 0;
			break;
		}
	}
}
