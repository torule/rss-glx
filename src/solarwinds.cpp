/*
 * Copyright (C) 2002  Terence M. Welsh
 * Ported to Linux by Tugrul Galatali <tugrul@galatali.com>
 *
 * Solar Winds is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 *
 * Solar Winds is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Solar Winds screen saver

#include <math.h>
#include <stdio.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "driver.h"
#include "rsDefines.h"
#include "rsRand.h"

const char *hack_name = "solarwinds";

#define DEFAULTS1 1
#define DEFAULTS2 2
#define DEFAULTS3 3
#define DEFAULTS4 4
#define DEFAULTS5 5
#define DEFAULTS6 6

#define NUMCONSTS 9
#define LIGHTSIZE 64

class wind;

// Global variables
wind *winds;
float lumdiff;
float cosCameraAngle, sinCameraAngle;
unsigned char lightTexture[LIGHTSIZE][LIGHTSIZE];

// Parameters edited in the dialog box
int dWinds;
int dEmitters;
int dParticles;
int dGeometry;
int dSize;
int dParticlespeed;
int dEmitterspeed;
int dWindspeed;
int dBlur;

class wind {
      public:
	float **emitters;
	float **particles;
	int **linelist;
	int *lastparticle;
	int whichparticle;
	float c[NUMCONSTS];
	float ct[NUMCONSTS];
	float cv[NUMCONSTS];

	  wind ();
	 ~wind ();
	void update ();
};

wind::wind ()
{
	int i;

	emitters = new float *[dEmitters];

	for (i = 0; i < dEmitters; i++) {
		emitters[i] = new float[3];

		emitters[i][0] = rsRandf (60.0f) - 30.0f;
		emitters[i][1] = rsRandf (60.0f) - 30.0f;
		emitters[i][2] = rsRandf (30.0f) - 15.0f;
	}

	particles = new float *[dParticles];

	for (i = 0; i < dParticles; i++) {
		particles[i] = new float[6];	// 3 for pos, 3 for color

		particles[i][2] = 100.0f;	// start particles behind viewer
	}

	whichparticle = 0;

	if (dGeometry == 2) {	// allocate memory for lines
		linelist = new int *[dParticles];

		for (i = 0; i < dParticles; i++) {
			linelist[i] = new int[2];

			linelist[i][0] = -1;
			linelist[i][1] = -1;
		}
		lastparticle = new int[dEmitters];

		for (i = 0; i < dEmitters; i++)
			lastparticle[i] = i;
	}

	for (i = 0; i < NUMCONSTS; i++) {
		ct[i] = rsRandf (PIx2);
		cv[i] = rsRandf (0.00005f * float (dWindspeed) * float (dWindspeed))
		+ 0.00001f * float (dWindspeed) * float (dWindspeed);
	}
}

wind::~wind ()
{
	int i;

	for (i = 0; i < dEmitters; i++)
		delete[]emitters[i];
	delete[]emitters;

	for (i = 0; i < dParticles; i++)
		delete[]particles[i];
	delete[]particles;

	if (dGeometry == 2) {
		for (i = 0; i < dParticles; i++)
			delete[]linelist[i];
		delete[]linelist;
		delete[]lastparticle;
	}
}

void
  wind::update ()
{
	int i;
	float x, y, z;
	float temp;
	static float evel = float (dEmitterspeed) * 0.01f;
	static float pvel = float (dParticlespeed) * 0.01f;
	static float pointsize = 0.04f * float (dSize);
	static float linesize = 0.005f * float (dSize);

	// update constants
	for (i = 0; i < NUMCONSTS; i++) {
		ct[i] += cv[i];
		if (ct[i] > PIx2)
			ct[i] -= PIx2;
		c[i] = cos (ct[i]);
	}

	// calculate emissions
	for (i = 0; i < dEmitters; i++) {
		emitters[i][2] += evel;	// emitter moves toward viewer
		if (emitters[i][2] > 15.0f) {	// reset emitter
			emitters[i][0] = rsRandf (60.0f) - 30.0f;
			emitters[i][1] = rsRandf (60.0f) - 30.0f;
			emitters[i][2] = -15.0f;
		}
		particles[whichparticle][0] = emitters[i][0];
		particles[whichparticle][1] = emitters[i][1];
		particles[whichparticle][2] = emitters[i][2];
		if (dGeometry == 2) {	// link particles to form lines
			if (linelist[whichparticle][0] >= 0)
				linelist[linelist[whichparticle][0]][1] = -1;
			linelist[whichparticle][0] = -1;
			if (emitters[i][2] == -15.0f)
				linelist[whichparticle][1] = -1;
			else
				linelist[whichparticle][1] = lastparticle[i];
			linelist[lastparticle[i]][0] = whichparticle;
			lastparticle[i] = whichparticle;
		}
		whichparticle++;
		if (whichparticle >= dParticles)
			whichparticle = 0;
	}

	// calculate particle positions and colors
	// first modify constants that affect colors
	c[6] *= 9.0f / float (dParticlespeed);
	c[7] *= 9.0f / float (dParticlespeed);
	c[8] *= 9.0f / float (dParticlespeed);

	// then update each particle
	for (i = 0; i < dParticles; i++) {
		// store old positions
		x = particles[i][0];
		y = particles[i][1];
		z = particles[i][2];
		// make new positions
		particles[i][0] = x + (c[0] * y + c[1] * z) * pvel;
		particles[i][1] = y + (c[2] * z + c[3] * x) * pvel;
		particles[i][2] = z + (c[4] * x + c[5] * y) * pvel;
		// calculate colors
		particles[i][3] = float (fabs ((particles[i][0] - x) * c[6]));
		particles[i][4] = float (fabs ((particles[i][1] - y) * c[7]));
		particles[i][5] = float (fabs ((particles[i][2] - z) * c[8]));

		// clamp colors
		if (particles[i][3] > 1.0f)
			particles[i][3] = 1.0f;
		if (particles[i][4] > 1.0f)
			particles[i][4] = 1.0f;
		if (particles[i][5] > 1.0f)
			particles[i][5] = 1.0f;
	}

	// draw particles
	switch (dGeometry) {
	case 0:		// lights
		for (i = 0; i < dParticles; i++) {
			glColor3fv (&particles[i][3]);
			glPushMatrix ();
			glTranslatef (particles[i][0], particles[i][1], particles[i][2]);
			glCallList (1);
			glPopMatrix ();
		}
		break;
	case 1:		// points
		for (i = 0; i < dParticles; i++) {
			temp = particles[i][2] + 40.0f;
			if (temp < 0.01f)
				temp = 0.01f;
			glPointSize (pointsize * temp);
			glBegin (GL_POINTS);
			glColor3fv (&particles[i][3]);
			glVertex3fv (particles[i]);
			glEnd ();
		}
		break;
	case 2:		// lines
		for (i = 0; i < dParticles; i++) {
			temp = particles[i][2] + 40.0f;
			if (temp < 0.01f)
				temp = 0.01f;
			glLineWidth (linesize * temp);
			glBegin (GL_LINES);
			if (linelist[i][1] >= 0) {
				glColor3fv (&particles[i][3]);
				if (linelist[i][0] == -1)
					glColor3f (0.0f, 0.0f, 0.0f);
				glVertex3fv (particles[i]);
				glColor3fv (&particles[linelist[i][1]][3]);
				if (linelist[linelist[i][1]][1] == -1)
					glColor3f (0.0f, 0.0f, 0.0f);
				glVertex3fv (particles[linelist[i][1]]);
			}
			glEnd ();
		}
	}
}

void hack_draw (xstuff_t * XStuff, double currentTime, float frameTime)
{
	int i;

	if (!dBlur) {
		glClear (GL_COLOR_BUFFER_BIT);
	} else {
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
			glLoadIdentity();
			glOrtho(0.0, 1.0, 0.0, 1.0, 1.0, -1.0);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glColor4f(0.0f, 0.0f, 0.0f, 0.5f - (float(dBlur) * 0.0049f));
				glBegin(GL_TRIANGLE_STRIP);
					glVertex3f(0.0f, 0.0f, 0.0f);
					glVertex3f(1.0f, 0.0f, 0.0f);
					glVertex3f(0.0f, 1.0f, 0.0f);
					glVertex3f(1.0f, 1.0f, 0.0f);
				glEnd();
				if(dGeometry == 0)
					glBlendFunc(GL_ONE, GL_ONE);
				else
					glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // Necessary for point and line smoothing (I don't know why)
						// Maybe it's just my video card...
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0, 0.0, -15.0);

	// You should need to draw twice if using blur, once to each buffer.
	// But wglSwapLayerBuffers appears to copy the back to the
	// front instead of just switching the pointers to them.  It turns
	// out that both NVidia and 3dfx prefer to use PFD_SWAP_COPY instead
	// of PFD_SWAP_EXCHANGE in the PIXELFORMATDESCRIPTOR.  I don't know
	// why...
	// So this may not work right on other platforms or all video cards.

	// Update surfaces
	for (i = 0; i < dWinds; i++)
		winds[i].update ();

	glFlush ();
}

void hack_reshape (xstuff_t * XStuff)
{
	// Window initialization
	glViewport (0, 0, XStuff->windowWidth, XStuff->windowHeight);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	gluPerspective (90.0, float (XStuff->windowWidth) / float (XStuff->windowHeight), 1.0, 10000);

	glTranslatef (0.0, 0.0, -15.0);
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();

	glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
	glClear (GL_COLOR_BUFFER_BIT);
}

void hack_init (xstuff_t * XStuff)
{
	int i, j;
	float x, y, temp;

	hack_reshape (XStuff);

	if (!dGeometry)
		glBlendFunc (GL_ONE, GL_ONE);
	else
		glBlendFunc (GL_SRC_ALPHA, GL_ONE);	// Necessary for point and line smoothing (I don't know why)

	glEnable (GL_BLEND);

	if (!dGeometry) {	// Init lights
		for (i = 0; i < LIGHTSIZE; i++) {
			for (j = 0; j < LIGHTSIZE; j++) {
				x = float (i - LIGHTSIZE / 2) / float (LIGHTSIZE / 2);
				y = float (j - LIGHTSIZE / 2) / float (LIGHTSIZE / 2);
				temp = 1.0f - float (sqrt ((x * x) + (y * y)));

				if (temp > 1.0f)
					temp = 1.0f;
				if (temp < 0.0f)
					temp = 0.0f;
				lightTexture[i][j] = char (255.0f * temp);
			}
		}
		glEnable (GL_TEXTURE_2D);
		glBindTexture (GL_TEXTURE_2D, 1);
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D (GL_TEXTURE_2D, 0, 1, LIGHTSIZE, LIGHTSIZE, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, lightTexture);
		temp = 0.02f * float (dSize);

		glNewList (1, GL_COMPILE);
		glBindTexture (GL_TEXTURE_2D, 1);
		glBegin (GL_TRIANGLE_STRIP);
		glTexCoord2f (0.0f, 0.0f);
		glVertex3f (-temp, -temp, 0.0f);
		glTexCoord2f (1.0f, 0.0f);
		glVertex3f (temp, -temp, 0.0f);
		glTexCoord2f (0.0f, 1.0f);
		glVertex3f (-temp, temp, 0.0f);
		glTexCoord2f (1.0f, 1.0f);
		glVertex3f (temp, temp, 0.0f);
		glEnd ();
		glEndList ();
	}

	if (dGeometry == 1) {	// init point smoothing
		glEnable (GL_POINT_SMOOTH);
		glHint (GL_POINT_SMOOTH_HINT, GL_NICEST);
	}

	if (dGeometry == 2) {	// init line smoothing
		glEnable (GL_LINE_SMOOTH);
		glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);
	}
	// Initialize surfaces
	winds = new wind[dWinds];

	glFlush ();
}

void hack_cleanup (xstuff_t * XStuff)
{
	glDeleteLists (1, 1);
}

void setDefaults (int which)
{
	switch (which) {
	case DEFAULTS1:	// Regular
		dWinds = 1;
		dEmitters = 30;
		dParticles = 2000;
		dGeometry = 0;
		dSize = 50;
		dWindspeed = 20;
		dEmitterspeed = 15;
		dParticlespeed = 10;
		dBlur = 40;
		break;
	case DEFAULTS2:	// Cosmic Strings
		dWinds = 1;
		dEmitters = 50;
		dParticles = 3000;
		dGeometry = 2;
		dSize = 20;
		dWindspeed = 10;
		dEmitterspeed = 10;
		dParticlespeed = 10;
		dBlur = 10;
		break;
	case DEFAULTS3:	// Cold Pricklies
		dWinds = 1;
		dEmitters = 300;
		dParticles = 3000;
		dGeometry = 2;
		dSize = 5;
		dWindspeed = 20;
		dEmitterspeed = 100;
		dParticlespeed = 15;
		dBlur = 70;
		break;
	case DEFAULTS4:	// Space Fur
		dWinds = 2;
		dEmitters = 400;
		dParticles = 1600;
		dGeometry = 2;
		dSize = 15;
		dWindspeed = 20;
		dEmitterspeed = 15;
		dParticlespeed = 10;
		dBlur = 0;
		break;
	case DEFAULTS5:	// Jiggly
		dWinds = 1;
		dEmitters = 40;
		dParticles = 1200;
		dGeometry = 1;
		dSize = 20;
		dWindspeed = 100;
		dEmitterspeed = 20;
		dParticlespeed = 4;
		dBlur = 50;
		break;
	case DEFAULTS6:	// Undertow
		dWinds = 1;
		dEmitters = 400;
		dParticles = 1200;
		dGeometry = 0;
		dSize = 40;
		dWindspeed = 20;
		dEmitterspeed = 1;
		dParticlespeed = 100;
		dBlur = 50;
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
			{"preset", 1, 0, 'R'},
			{"regular", 0, 0, 10},
			{"cosmicstrings", 0, 0, 11},
			{"coldpricklies", 0, 0, 12},
			{"spacefur", 0, 0, 13},
			{"jiggy", 0, 0, 14},
			{"undertow", 0, 0, 15},
			{"winds", 1, 0, 'w'},
			{"emitters", 1, 0, 'e'},
			{"particles", 1, 0, 'p'},
			{"geometry", 1, 0, 'g'},
			{"lights", 0, 0, 20},
			{"points", 0, 0, 21},
			{"lines", 0, 0, 22},
			{"size", 1, 0, 's'},
			{"windspeed", 1, 0, 'W'},
			{"emitterspeed", 1, 0, 'E'},
			{"particlespeed", 1, 0, 'P'},
			{"blur", 1, 0, 'b'},
			{0, 0, 0, 0}
		};

		c = getopt_long (argc, argv, DRIVER_OPTIONS_SHORT "hR:w:e:p:s:g:W:E:P:b:", long_options, NULL);
#else
		c = getopt (argc, argv, DRIVER_OPTIONS_SHORT "hR:w:e:p:s:g:W:E:P:b:");
#endif
		if (c == -1)
			break;

		switch (c) {
		DRIVER_OPTIONS_CASES case 'h':
			printf ("%s:"
#ifndef HAVE_GETOPT_H
				" Not built with GNU getopt.h, long options *NOT* enabled."
#endif
				"\n" DRIVER_OPTIONS_HELP
				"\t--preset/-R <arg>\n" 
				"\t--regular\n"
				"\t--cosmicstrings\n"
				"\t--coldpricklies\n"
				"\t--spacefur\n"
				"\t--jiggy\n"
				"\t--undertow\n"
				"\t--winds/-w <arg>\n" "\t--emitters/-e <arg>\n" "\t--particles/-p <arg>\n"
				"\t--geometry/-g <arg>\n" "\t--lights\n" "\t--points\n" "\t--lines\n"
				"\t--size/-s <arg>\n" 
				"\t--windspeed/-W <arg>\n" "\t--emitterspeed/-E <arg>\n" "\t--particlespeed/-P <arg>\n"
				"\t--blur/-b <arg>\n", argv[0]);
			exit (1);
		case 'R':
			change_flag = 1;
			setDefaults (strtol_minmaxdef(optarg, 10, 1, 6, 0, 1, "--preset: "));
			break;
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			change_flag = 1;
			setDefaults (c - 9);
			break;
		case 'w':
			change_flag = 1;
			dWinds = strtol_minmaxdef(optarg, 10, 1, 10, 1, 1, "--winds: ");
			break;
		case 'e':
			change_flag = 1;
			dEmitters = strtol_minmaxdef(optarg, 10, 1, 1000, 1, 30, "--emitters: ");
			break;
		case 'p':
			change_flag = 1;
			dParticles = strtol_minmaxdef(optarg, 10, 0, 10000, 1, 2000, "--particles: ");
			break;
		case 's':
			change_flag = 1;
			dSize = strtol_minmaxdef(optarg, 10, 0, 100, 1, 50, "--size: ");
			break;
		case 'g':
			change_flag = 1;
			dGeometry = strtol_minmaxdef(optarg, 10, 0, 2, 0, 0, "--geometry: ");
			break;
		case 20:
		case 21:
		case 22:
			change_flag = 1;
			dGeometry = c - 20;
			break;
		case 'W':
			change_flag = 1;
			dWindspeed = strtol_minmaxdef(optarg, 10, 1, 100, 1, 20, "--windspeed: ");
			break;
		case 'E':
			change_flag = 1;
			dEmitterspeed = strtol_minmaxdef(optarg, 10, 1, 100, 1, 15, "--emitterspeed: ");
			break;
		case 'P':
			change_flag = 1;
			dParticlespeed = strtol_minmaxdef(optarg, 10, 1, 100, 1, 10, "--particlespeed: ");
			break;
		case 'b':
			change_flag = 1;
			dBlur = strtol_minmaxdef(optarg, 10, 0, 100, 1, 40, "--blur: ");
			break;
		}
	}

	if (!change_flag) {
		setDefaults (rsRandi (6) + 1);
	}
}
