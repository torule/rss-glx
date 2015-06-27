/*
 * Copyright (C) 2002  Terence M. Welsh
 * Ported to Linux by Tugrul Galatali <tugrul@galatali.com>
 *
 * Flux is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 *
 * Flux is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Flux screen saver

#include <math.h>
#include <stdio.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "driver.h"
#include "rgbhsl.h"
#include "rsRand.h"
#include "rsDefines.h"

const char *hack_name = "flux";

#define NUMCONSTS 8
#define LIGHTSIZE 64

#define DEFAULTS1 1
#define DEFAULTS2 2
#define DEFAULTS3 3
#define DEFAULTS4 4
#define DEFAULTS5 5
#define DEFAULTS6 6

class flux;
class particle;

// Global variables
unsigned int tex;
flux *fluxes;
float lumdiff;
int whichparticle;
float cosCameraAngle, sinCameraAngle;
unsigned char lightTexture[LIGHTSIZE][LIGHTSIZE];
float aspectRatio;
float orbitiness = 0.0f;
float prevOrbitiness = 0.0f;


// Parameters edited in the dialog box
int dFluxes;
int dParticles;
int dTrail;
int dGeometry;
int dSize;
int dComplexity;
int dRandomize;
int dExpansion;
int dRotation;
int dWind;
int dInstability;
int dBlur;
int dSmart;

// This class is poorly named.  It's actually a whole trail of particles.
class particle {
      public:
	float **vertices;
	int counter;
	float offset[3];

	  particle ();
	 ~particle ();
	float update (float *c);
};

particle::particle ()
{
	int i;

	// Offsets are somewhat like default positions for the head of each
	// particle trail.  Offsets spread out the particle trails and keep
	// them from all overlapping.
	offset[0] = cos (PIx2 * float (whichparticle) / float (dParticles));
	offset[1] = float (whichparticle) / float (dParticles) - 0.5f;
	offset[2] = sin (PIx2 * float (whichparticle) / float (dParticles));

	whichparticle++;

	// Initialize memory and set initial positions out of view of the
	// camera
	vertices = new float *[dTrail];

	for (i = 0; i < dTrail; i++) {
		vertices[i] = new float[5];	// 0,1,2 = position, 3 = hue, 4 =

		// saturation
		vertices[i][0] = 0.0f;
		vertices[i][1] = 3.0f;
		vertices[i][2] = 0.0f;
		vertices[i][3] = 0.0f;
		vertices[i][4] = 0.0f;
	}

	counter = 0;
}

particle::~particle ()
{
	for (int i = 0; i < dTrail; i++)
		delete[]vertices[i];
	delete[]vertices;
}

float
  particle::update (float *c)
{
	int i, p, growth;
	float rgb[3];
	float cx, cy, cz;	// Containment variables
	float luminosity;
	static float expander = 1.0f + 0.0005f * float (dExpansion);
	static float blower = 0.001f * float (dWind);
	float depth = 0;

	// Record old position
	int oldc = counter;
	float oldpos[3];

	oldpos[0] = vertices[oldc][0];
	oldpos[1] = vertices[oldc][1];
	oldpos[2] = vertices[oldc][2];

	counter++;
	if (counter >= dTrail)
		counter = 0;

	// Here's the iterative math for calculating new vertex positions
	// first calculate limiting terms which keep vertices from constantly
	// flying off to infinity
	cx = vertices[oldc][0] * (1.0f - 1.0f / (vertices[oldc][0] * vertices[oldc][0] + 1.0f));
	cy = vertices[oldc][1] * (1.0f - 1.0f / (vertices[oldc][1] * vertices[oldc][1] + 1.0f));
	cz = vertices[oldc][2] * (1.0f - 1.0f / (vertices[oldc][2] * vertices[oldc][2] + 1.0f));
	// then calculate new positions
	vertices[counter][0] = vertices[oldc][0] + c[6] * offset[0] - cx + c[2] * vertices[oldc][1]
		+ c[5] * vertices[oldc][2];
	vertices[counter][1] = vertices[oldc][1] + c[6] * offset[1] - cy + c[1] * vertices[oldc][2]
		+ c[4] * vertices[oldc][0];
	vertices[counter][2] = vertices[oldc][2] + c[6] * offset[2] - cz + c[0] * vertices[oldc][0]
		+ c[3] * vertices[oldc][1];

	// calculate "orbitiness" of particles
	const float xdiff(vertices[counter][0] - vertices[oldc][0]);
	const float ydiff(vertices[counter][1] - vertices[oldc][1]);
	const float zdiff(vertices[counter][2] - vertices[oldc][2]);
	const float distsq(vertices[counter][0] * vertices[counter][0]
		+ vertices[counter][1] * vertices[counter][1]
		+ vertices[counter][2] * vertices[counter][2]);
	const float oldDistsq(vertices[oldc][0] * vertices[oldc][0]
		+ vertices[oldc][1] * vertices[oldc][1]
		+ vertices[oldc][2] * vertices[oldc][2]);
	orbitiness += (xdiff * xdiff + ydiff * ydiff + zdiff * zdiff)
		/ (2.0f - fabs(distsq - oldDistsq));

	// Pick a hue
	vertices[counter][3] = cx * cx + cy * cy + cz * cz;
	if (vertices[counter][3] > 1.0f)
		vertices[counter][3] = 1.0f;
	vertices[counter][3] += c[7];
	// Limit the hue (0 - 1)
	if (vertices[counter][3] > 1.0f)
		vertices[counter][3] -= 1.0f;
	if (vertices[counter][3] < 0.0f)
		vertices[counter][3] += 1.0f;
	// Pick a saturation
	vertices[counter][4] = c[0] + vertices[counter][3];
	// Limit the saturation (0 - 1)
	if (vertices[counter][4] < 0.0f)
		vertices[counter][4] = -vertices[counter][4];
	vertices[counter][4] -= float (int (vertices[counter][4]));

	vertices[counter][4] = 1.0f - (vertices[counter][4] * vertices[counter][4]);

	// Bring particles back if they escape
	if (!counter) {
		if ((vertices[0][0] > 10000.0f)
		    || (vertices[0][0] < -10000.0f)
		    || (vertices[0][1] > 10000.0f)
		    || (vertices[0][1] < -10000.0f)
		    || (vertices[2][2] > 10000.0f)
		    || (vertices[0][2] < -10000.0f)) {
			vertices[0][0] = rsRandf (2.0f) - 1.0f;
			vertices[0][1] = rsRandf (2.0f) - 1.0f;
			vertices[0][2] = rsRandf (2.0f) - 1.0f;
		}
	}
	// Draw every vertex in particle trail
	p = counter;
	growth = 0;
	luminosity = lumdiff;
	for (i = 0; i < dTrail; i++) {
		p++;
		if (p >= dTrail)
			p = 0;
		growth++;

		// assign color to particle
		hsl2rgb (vertices[p][3], vertices[p][4], luminosity, rgb[0], rgb[1], rgb[2]);
		glColor3fv (rgb);

		glPushMatrix ();
		if (dGeometry == 1)	// Spheres
			glTranslatef (vertices[p][0], vertices[p][1], vertices[p][2]);
		else {		// Points or lights
			depth = cosCameraAngle * vertices[p][2] - sinCameraAngle * vertices[p][0];
			glTranslatef (cosCameraAngle * vertices[p][0] + sinCameraAngle * vertices[p][2], vertices[p][1], depth);
		}
		if (dGeometry) {	// Spheres or lights
			switch (dTrail - growth) {
			case 0:
				glScalef (0.259f, 0.259f, 0.259f);
				break;
			case 1:
				glScalef (0.5f, 0.5f, 0.5f);
				break;
			case 2:
				glScalef (0.707f, 0.707f, 0.707f);
				break;
			case 3:
				glScalef (0.866f, 0.866f, 0.866f);
				break;
			case 4:
				glScalef (0.966f, 0.966f, 0.966f);
			}
		}
		switch (dGeometry) {
		case 0:	// Points
			switch (dTrail - growth) {
			case 0:
				glPointSize (float
					       (dSize * (depth + 200.0f) * 0.001036f));

				break;
			case 1:
				glPointSize (float
					       (dSize * (depth + 200.0f) * 0.002f));

				break;
			case 2:
				glPointSize (float
					       (dSize * (depth + 200.0f) * 0.002828f));

				break;
			case 3:
				glPointSize (float
					       (dSize * (depth + 200.0f) * 0.003464f));

				break;
			case 4:
				glPointSize (float
					       (dSize * (depth + 200.0f) * 0.003864f));

				break;
			default:
				glPointSize (float
					       (dSize * (depth + 200.0f) * 0.004f));
			}
			glBegin (GL_POINTS);
			glVertex3f (0.0f, 0.0f, 0.0f);
			glEnd ();
			break;
		case 1:	// Spheres
		case 2:	// Lights
			glCallList (1);
		}
		glPopMatrix ();
		vertices[p][0] *= expander;
		vertices[p][1] *= expander;
		vertices[p][2] *= expander;
		vertices[p][2] += blower;
		luminosity += lumdiff;
	}

	// Find distance between new position and old position and return it
	oldpos[0] -= vertices[counter][0];
	oldpos[1] -= vertices[counter][1];
	oldpos[2] -= vertices[counter][2];
	return (float (sqrt (oldpos[0] * oldpos[0] + oldpos[1] * oldpos[1] + oldpos[2] * oldpos[2])));
}

// This class is a set of particle trails and constants that enter
// into their equations of motion.
class flux {
      public:
	particle * particles;
	int randomize;
	float c[NUMCONSTS];	// constants
	float cv[NUMCONSTS];	// constants' change velocities
	int currentSmartConstant;
	float oldDistance;

	flux ();
	~flux ();
	void update ();
};

flux::flux ()
{
	int i;

	whichparticle = 0;

	particles = new particle[dParticles];
	randomize = 1;
	for (i = 0; i < NUMCONSTS; i++) {
		c[i] = rsRandf (2.0f) - 1.0f;
		cv[i] = rsRandf (0.000005f * float (dInstability) * float (dInstability))
		+ 0.000001f * float (dInstability) * float (dInstability);
	}

	currentSmartConstant = 0;
	oldDistance = 0.0f;
}

flux::~flux ()
{
	delete[]particles;
}

void
  flux::update ()
{
	int i;

	// randomize constants
	if (dRandomize) {
		randomize--;
		if (randomize <= 0) {
			for (i = 0; i < NUMCONSTS; i++)
				c[i] = rsRandf (2.0f) - 1.0f;
			int temp = 101 - dRandomize;

			temp = temp * temp;
			randomize = temp + rsRandi (temp);
		}
	}
	// update constants
	for (i = 0; i < NUMCONSTS; i++) {
		c[i] += cv[i];
		if (c[i] >= 1.0f) {
			c[i] = 1.0f;
			cv[i] = -cv[i];
		}
		if (c[i] <= -1.0f) {
			c[i] = -1.0f;
			cv[i] = -cv[i];
		}
	}

	prevOrbitiness = orbitiness;
	orbitiness = 0.0f;

	// update all particles in this flux field
	float dist;

	for (i = 0; i < dParticles; i++)
		dist = particles[i].update (c);

	// use dist from last particle to activate smart constants
	dSmart = 0;
	if (dSmart) {
		const float upper = 0.4f;
		const float lower = 0.2f;
		int beSmart = 0;

		if (dist > upper && dist > oldDistance)
			beSmart = 1;
		if (dist < lower && dist < oldDistance)
			beSmart = 1;
		if (beSmart) {
			cv[currentSmartConstant] = -cv[currentSmartConstant];
			currentSmartConstant++;
			if (currentSmartConstant >= dSmart)
				currentSmartConstant = 0;
		}
		oldDistance = dist;
	}

	if (orbitiness < prevOrbitiness) {
		i = rsRandi(NUMCONSTS - 1);
		cv[i] = -cv[i];
	}
}

void hack_draw (xstuff_t * XStuff, double currentTime, float frameTime)
{

	int i;
	static float cameraAngle = 0.0f;

	// clear the screen
	glLoadIdentity ();
	if (dBlur) {		// partially
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
			glLoadIdentity();
			glOrtho(0.0, 1.0, 0.0, 1.0, 1.0, -1.0);
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
				glLoadIdentity();
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glEnable(GL_BLEND);
				glDisable(GL_DEPTH_TEST);
				glColor4f(0.0f, 0.0f, 0.0f, 0.5f - (float(sqrtf(sqrtf(float(dBlur)))) * 0.15495f));
				glBegin(GL_TRIANGLE_STRIP);
					glVertex3f(0.0f, 0.0f, 0.0f);
					glVertex3f(1.0f, 0.0f, 0.0f);
					glVertex3f(0.0f, 1.0f, 0.0f);
					glVertex3f(1.0f, 1.0f, 0.0f);
				glEnd();
			glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	} else			// completely
		glClear (GL_COLOR_BUFFER_BIT);

	cameraAngle += 0.01f * float (dRotation);

	if (cameraAngle >= 360.0f)
		cameraAngle -= 360.0f;
	if (dGeometry == 1)	// Only rotate for spheres
		glRotatef (cameraAngle, 0.0f, 1.0f, 0.0f);
	else {
		cosCameraAngle = cos (cameraAngle * DEG2RAD);
		sinCameraAngle = sin (cameraAngle * DEG2RAD);
	}

	// set up state for rendering particles
	switch (dGeometry) {
	case 0:		// Blending for points
		glBlendFunc (GL_SRC_ALPHA, GL_ONE);
		glEnable (GL_BLEND);
		glEnable (GL_POINT_SMOOTH);
		glHint (GL_POINT_SMOOTH_HINT, GL_NICEST);
		break;
	case 1:		// No blending for spheres, but we need
		// z-buffering
		glDisable (GL_BLEND);
		glEnable (GL_DEPTH_TEST);
		glClear (GL_DEPTH_BUFFER_BIT);
		break;
	case 2:		// Blending for lights
		glBlendFunc (GL_ONE, GL_ONE);
		glEnable (GL_BLEND);
		glBindTexture(GL_TEXTURE_2D, tex);
		glEnable(GL_TEXTURE_2D);
	}

	// Update particles
	glMatrixMode(GL_MODELVIEW);
	for (i = 0; i < dFluxes; i++)
		fluxes[i].update ();

	glFlush ();
}

void hack_reshape (xstuff_t * XStuff)
{
	int i, j;
	float x, y, temp;

	glViewport (0, 0, XStuff->windowWidth, XStuff->windowHeight);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();

	aspectRatio = float (XStuff->windowWidth) / float (XStuff->windowHeight);
	gluPerspective (100.0, aspectRatio, 0.01, 200);

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();

	glTranslatef (0.0, 0.0, -2.5);

	if (dGeometry == 0) {
		glEnable (GL_POINT_SMOOTH);
		// glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
	}

	glFrontFace (GL_CCW);
	glEnable (GL_CULL_FACE);
	glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (dGeometry == 1) {	// Spheres and their lighting
		glNewList (1, GL_COMPILE);
		GLUquadricObj *qobj = gluNewQuadric ();
		gluSphere (qobj, 0.005f * float (dSize), dComplexity + 2, dComplexity + 1);

		gluDeleteQuadric (qobj);
		glEndList ();

		glEnable (GL_LIGHTING);
		glEnable (GL_LIGHT0);
		float ambient[4] = {
			0.0f,
			0.0f,
			0.0f,
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
		float position[4] = {
			500.0f,
			500.0f,
			500.0f,
			0.0f
		};

		glLightfv (GL_LIGHT0, GL_AMBIENT, ambient);
		glLightfv (GL_LIGHT0, GL_DIFFUSE, diffuse);
		glLightfv (GL_LIGHT0, GL_SPECULAR, specular);
		glLightfv (GL_LIGHT0, GL_POSITION, position);
		glEnable (GL_COLOR_MATERIAL);
		glColorMaterial (GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	} else if (dGeometry == 2) {	// Init lights
		for (i = 0; i < LIGHTSIZE; i++) {
			for (j = 0; j < LIGHTSIZE; j++) {
				x = float (i - LIGHTSIZE / 2) / float (LIGHTSIZE / 2);
				y = float (j - LIGHTSIZE / 2) / float (LIGHTSIZE / 2);
				temp = 1.0f - float (sqrt ((x * x) + (y * y)));

				if (temp > 1.0f)
					temp = 1.0f;
				if (temp < 0.0f)
					temp = 0.0f;
				lightTexture[i][j] = char (255.0f * temp * temp);
			}
		}
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D (GL_TEXTURE_2D, 0, 1, LIGHTSIZE, LIGHTSIZE, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, lightTexture);

		temp = float (dSize) * 0.005f;

		glNewList (1, GL_COMPILE);
		glBegin (GL_TRIANGLES);
		glTexCoord2f (0.0f, 0.0f);
		glVertex3f (-temp, -temp, 0.0f);
		glTexCoord2f (1.0f, 0.0f);
		glVertex3f (temp, -temp, 0.0f);
		glTexCoord2f (1.0f, 1.0f);
		glVertex3f (temp, temp, 0.0f);
		glTexCoord2f (0.0f, 0.0f);
		glVertex3f (-temp, -temp, 0.0f);
		glTexCoord2f (1.0f, 1.0f);
		glVertex3f (temp, temp, 0.0f);
		glTexCoord2f (0.0f, 1.0f);
		glVertex3f (-temp, temp, 0.0f);
		glEnd ();
		glEndList ();
	}
	// Initialize luminosity difference
	lumdiff = 1.0f / float (dTrail);
}

void hack_init (xstuff_t * XStuff)
{
	hack_reshape (XStuff);

	// Initialize flux fields
	fluxes = new flux[dFluxes];
}

void hack_cleanup (xstuff_t * XStuff)
{
	if ((dGeometry == 1) || (dGeometry == 2)) {
		glDeleteLists (1, 1);
	}
}

void setDefaults (int which)
{
	switch (which) {
	case DEFAULTS1:	// Regular
		dFluxes = 1;
		dParticles = 20;
		dTrail = 40;
		dGeometry = 2;
		dSize = 15;
		dComplexity = 3;
		dRandomize = 0;
		dExpansion = 40;
		dRotation = 30;
		dWind = 20;
		dInstability = 20;
		dBlur = 0;
		break;
	case DEFAULTS2:	// Hypnotic
		dFluxes = 2;
		dParticles = 10;
		dTrail = 40;
		dGeometry = 2;
		dSize = 15;
		dComplexity = 3;
		dRandomize = 80;
		dExpansion = 20;
		dRotation = 0;
		dWind = 40;
		dInstability = 10;
		dBlur = 30;
		break;
	case DEFAULTS3:	// Insane
		dFluxes = 4;
		dParticles = 30;
		dTrail = 8;
		dGeometry = 2;
		dSize = 25;
		dComplexity = 3;
		dRandomize = 0;
		dExpansion = 80;
		dRotation = 60;
		dWind = 40;
		dInstability = 100;
		dBlur = 10;
		break;
	case DEFAULTS4:	// Sparklers
		dFluxes = 3;
		dParticles = 20;
		dTrail = 6;
		dGeometry = 1;
		dSize = 20;
		dComplexity = 3;
		dRandomize = 85;
		dExpansion = 60;
		dRotation = 30;
		dWind = 20;
		dInstability = 30;
		dBlur = 0;
		break;
	case DEFAULTS5:	// Paradigm
		dFluxes = 1;
		dParticles = 40;
		dTrail = 40;
		dGeometry = 2;
		dSize = 5;
		dComplexity = 3;
		dRandomize = 90;
		dExpansion = 30;
		dRotation = 20;
		dWind = 10;
		dInstability = 5;
		dBlur = 10;
		break;
	case DEFAULTS6:	// Galactic
		dFluxes = 1;
		dParticles = 2;
		dTrail = 1500;
		dGeometry = 2;
		dSize = 10;
		dComplexity = 3;
		dRandomize = 0;
		dExpansion = 5;
		dRotation = 25;
		dWind = 0;
		dInstability = 5;
		dBlur = 0;
	}
}

void hack_handle_opts (int argc, char **argv)
{
	int change_flag = 0;

	setDefaults (DEFAULTS1);

	while (1) {
		int c;

#ifdef HAVE_GETOPT_H
		static struct option long_options[] = {
			{"help", 0, 0, 'h'},
			DRIVER_OPTIONS_LONG 
			{"fluxes", 1, 0, 'f'},
			{"particles", 1, 0, 'p'},
			{"trail", 1, 0, 't'},
			{"geometry", 1, 0, 'g'},
			{"points", 0, 0, 20},
			{"spheres", 0, 0, 21},
			{"lights", 0, 0, 22},
			{"size", 1, 0, 's'},
			{"complexity", 1, 0, 'c'},
			{"randomize", 1, 0, 'R'},
			{"expansion", 1, 0, 'e'},
			{"rotation", 1, 0, 'o'},
			{"wind", 1, 0, 'w'},
			{"instability", 1, 0, 'i'},
			{"blur", 1, 0, 'b'},
			{"preset", 1, 0, 'P'},
			{"regular", 0, 0, 10},
			{"hypnotic", 0, 0, 11},
			{"insane", 0, 0, 12},
			{"sparklers", 0, 0, 13},
			{"paradigm", 0, 0, 14},
			{"fusion", 0, 0, 15},
			{0, 0, 0, 0}
		};

		c = getopt_long (argc, argv, DRIVER_OPTIONS_SHORT "hf:p:t:g:s:c:R:e:o:w:i:b:P:", long_options, NULL);
#else
		c = getopt (argc, argv, DRIVER_OPTIONS_SHORT "hf:p:t:g:s:c:R:e:o:w:i:b:P:");
#endif
		if (c == -1)
			break;

		switch (c) {
			DRIVER_OPTIONS_CASES 
			case 'h':
				printf ("%s:"
#ifndef HAVE_GETOPT_H
						" Not built with GNU getopt.h, long options *NOT* enabled."
#endif
						"\n" DRIVER_OPTIONS_HELP 
						"\t--preset <arg>\n" "\t--regular\n" "\t--hypnotic\n" "\t--insane\n" "\t--sparklers\n" "\t--paradigm\n" "\t--fusion\n"
						"\t--fluxes <arg>\n" "\t--particles <arg>\n" "\t--trail <arg>\n"
						"\t--geometry <arg>\n" "\t--points\n" "\t--spheres\n" "\t--lights\n"
						"\t--size <arg>\n" "\t--complexity <arg>\n" "\t--randomize <arg>\n" "\t--expansion <arg>\n"
						"\t--rotation <arg>\n" "\t--wind <arg>\n" "\t--instability <arg>\n" "\t--blur <arg>\n", argv[0]);
			exit (1);
		case 'f':
			change_flag = 1;
			dFluxes = strtol_minmaxdef (optarg, 10, 1, 100, 1, 1, "--fluxes: ");
			break;
		case 'p':
			change_flag = 1;
			dParticles = strtol_minmaxdef (optarg, 10, 1, 1000, 1, 1, "--particles: ");
			break;
		case 't':
			change_flag = 1;
			dTrail = strtol_minmaxdef (optarg, 10, 3, 10000, 1, 40, "--trail: ");
			break;
		case 'g':
			change_flag = 1;
			dGeometry = strtol_minmaxdef (optarg, 10, 0, 2, 0, 0, "--geometry: ");
			break;
		case 20:
		case 21:
		case 22:
			change_flag = 1;
			dGeometry = c - 20;
			break;
		case 's':
			change_flag = 1;
			dSize = strtol_minmaxdef (optarg, 10, 1, 100, 1, 15, "--size: ");
			break;
		case 'c':
			change_flag = 1;
			dComplexity = strtol_minmaxdef (optarg, 10, 0, 100, 1, 3, "--complexity: ");
			break;
		case 'R':
			change_flag = 1;
			dRandomize = strtol_minmaxdef (optarg, 10, 0, 100, 1, 0, "--randomize: ");
			break;
		case 'e':
			change_flag = 1;
			dExpansion = strtol_minmaxdef (optarg, 10, 0, 100, 1, 40, "--expansion: ");
			break;
		case 'o':
			change_flag = 1;
			dRotation = strtol_minmaxdef (optarg, 10, 0, 100, 1, 30, "--rotation: ");
			break;
		case 'w':
			change_flag = 1;
			dWind = strtol_minmaxdef (optarg, 10, 0, 100, 1, 20, "--wind: ");
			break;
		case 'i':
			change_flag = 1;
			dInstability = strtol_minmaxdef (optarg, 10, 0, 100, 1, 20, "--instability: ");
			break;
		case 'b':
			change_flag = 1;
			dBlur = strtol_minmaxdef (optarg, 10, 0, 100, 1, 0, "--blur: ");
			break;
		case 'P':
			change_flag = 1;
			setDefaults (strtol_minmaxdef (optarg, 10, 1, 6, 0, 1, "--preset: "));
			break;
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			change_flag = 1;
			setDefaults(c - 9);
			break;
		}
	}

	if (!change_flag) {
		setDefaults (rsRandi (6) + 1);
	}
}
