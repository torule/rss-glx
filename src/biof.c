/*
 * Copyright (C) 2002  <hk@dgmr.nl>
 * Ported to Linux by Tugrul Galatali <tugrul@galatali.com>
 *
 * BioF is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 *
 * BioF is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// BioF screen saver

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "driver.h"
#include "rsRand.h"

const char *hack_name = "BioF";

#define TRIANGLES 1
#define SPHERES 2
#define BIGSPHERES 3
#define POINTS 4
#define LIGHTMAP 5

int NRPOINTS, NRLINES;
int dGeometry;
int dOffAngle;

GLUquadricObj *shapes;

void Horn (int ribs, double bend, double stack, double twist, double twisttrans, double grow)
{
	int i;

	for (i = 0; i < ribs; i++) {
		glPushMatrix ();
		glRotated (i * bend / ribs, 1, 0, 0);
		glTranslated (0, 0, i * stack / ribs);
		glTranslated (twisttrans, 0, 0);
		glRotated (i * twist / ribs, 0, 0, 1);
		glTranslated (-twisttrans, 0, 0);

		switch (dGeometry) {
		case TRIANGLES:
			glBegin (GL_TRIANGLES);
			glColor4d (0.9, 0.8, 0.5, 1);
			glVertex3d (2, 2, 0);
			glColor4d (0.9, 0.8, 0.5, 0.0);
			glVertex3d (0, 2, 0);
			glVertex3d (2, 0, 0);
			glEnd ();

			break;

		case BIGSPHERES:
			gluQuadricDrawStyle (shapes, GLU_FILL);
			gluQuadricNormals (shapes, GLU_SMOOTH);
			gluSphere (shapes, 7 * exp (i / ribs * log (grow)), 20, 10);

			break;

		case SPHERES:
			gluQuadricDrawStyle (shapes, GLU_FILL);
			gluQuadricNormals (shapes, GLU_SMOOTH);
			//gluSphere(shapes, power(grow,i/ribs), 5, 5);
			gluSphere (shapes, exp (i / ribs * log (grow)), 6, 4);

			break;

		case LIGHTMAP:
		case POINTS:
			glBegin (GL_POINTS);
			glVertex3d (0, 0, 0);
			glEnd ();

			break;
		}

		glPopMatrix ();
	}
}

void hack_reshape (xstuff_t * XStuff)
{
	glViewport (0, 0, (GLsizei) XStuff->windowWidth, (GLsizei) XStuff->windowHeight);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	gluPerspective (30, (float)XStuff->windowWidth / (float)XStuff->windowHeight, 100, 300);
}

void hack_init (xstuff_t * XStuff)	// Called right after the window is created, and OpenGL is initialized.
{
	float glfLightPosition[4] = { 100.0, 100.0, 100.0, 0.0 };
	float glfFog[4] = { 0.0, 0.0, 0.3, 1.0 };
	float DiffuseLightColor[4] = { 1, 0.8, 0.4, 1.0 };
	int i, j;
	double x, y, r, d;
	unsigned char texbuf[64][64];

	glEnable (GL_DEPTH_TEST);

	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable (GL_BLEND);

	if ((dGeometry == SPHERES) || (dGeometry == BIGSPHERES)) {
		glEnable (GL_CULL_FACE);
		glCullFace (GL_BACK);

		glLightfv (GL_LIGHT0, GL_POSITION, glfLightPosition);
		glEnable (GL_LIGHTING);
		glEnable (GL_LIGHT0);

		glLightfv (GL_LIGHT0, GL_DIFFUSE, DiffuseLightColor);

		shapes = gluNewQuadric ();
	} else if (dGeometry == POINTS) {
		glHint (GL_POINT_SMOOTH_HINT, GL_NICEST);
		glPointSize (3);
		glEnable (GL_POINT_SMOOTH);

		glFogi (GL_FOG_MODE, GL_LINEAR);
		glHint (GL_FOG_HINT, GL_NICEST);
		glFogfv (GL_FOG_COLOR, glfFog);
		glFogf (GL_FOG_START, 150);
		glFogf (GL_FOG_END, 250);
		glEnable (GL_FOG);
	} else if (dGeometry == LIGHTMAP) {
		r = 32;

		memset ((void *)&texbuf, 0, 4096);

		for (i = 0; i < 64; i++) {
			for (j = 0; j < 64; j++) {
				x = abs (i - 32);
				y = abs (j - 32);
				d = sqrt (x * x + y * y);

				if (d < r) {
					d = 1 - (d / r);
					texbuf[i][j] = (char)(255.0f * d * d);
				}
			}
		}

		gluBuild2DMipmaps (GL_TEXTURE_2D, 1, 64, 64, GL_LUMINANCE, GL_UNSIGNED_BYTE, texbuf);

		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	hack_reshape (XStuff);
}

void hack_cleanup (xstuff_t * XStuff)
{
	if ((dGeometry == SPHERES) || (dGeometry == BIGSPHERES))
		gluDeleteQuadric(shapes);
}

void hack_draw (xstuff_t * XStuff, double currentTime, float lastFrameDuration)
{
	if (dGeometry == LIGHTMAP) {
		int i, j, offset;
		float fb_buffer[NRPOINTS * NRLINES * 3];
		double x, y, w;

		glDisable (GL_DEPTH_TEST);
		glFeedbackBuffer (NRPOINTS * NRLINES * 3, GL_2D, fb_buffer);
		glRenderMode (GL_FEEDBACK);

		glMatrixMode (GL_MODELVIEW);
		glPushMatrix ();
		glLoadIdentity ();

		gluLookAt (200, 0, 0, 0, 0, 0, 0, 0, 1);

		glRotated (90, 0, 1, 0);
		glRotated (10 * currentTime, 0, 0, 1);

		glNewList (1, GL_COMPILE);
		Horn (NRPOINTS, 315 * sin (currentTime * 0.237), 45 /* + 20 * sin(currentTime * 0.133) */ , 1500 * sin (currentTime * 0.213), 7.5 + 2.5 * sin (currentTime * 0.173), 1);
		glEndList ();

		if (dOffAngle)
			glRotated (45, 1, 1, 0);

		for (i = 0; i < NRLINES; i++) {
			glPushMatrix ();

			glRotated (360 * i / NRLINES, 0, 0, 1);
			glRotated (45, 1, 0, 0);
			glTranslated (0, 0, 2);

			glCallList (1);

			glPopMatrix ();
		}

		glPopMatrix ();

		glRenderMode (GL_RENDER);

		glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glMatrixMode (GL_PROJECTION);
		glPushMatrix ();
		glLoadIdentity ();

		glMatrixMode (GL_MODELVIEW);
		glPushMatrix ();
		glLoadIdentity ();

		glScalef (2.0 / XStuff->windowWidth, 2.0 / XStuff->windowHeight, 1.0);
		glTranslatef (-0.5 * XStuff->windowWidth, -0.5 * XStuff->windowHeight, 0.0);

		glClear (GL_COLOR_BUFFER_BIT);

		glEnable (GL_TEXTURE_2D);
		glBlendFunc (GL_ONE, GL_ONE);
		glEnable (GL_BLEND);

		glBegin (GL_QUADS);

		w = XStuff->windowWidth >> 7;

		for (j = 0; j < NRPOINTS; j++) {
			glColor3f (0.25 - 0.15 * j / NRPOINTS, 0.15 + 0.10 * j / NRPOINTS, 0.06 + 0.14 * j / NRPOINTS);
			for (i = 0; i < NRLINES; i++) {
				offset = 3 * (i * NRPOINTS + j);
				x = fb_buffer[offset + 1];
				y = fb_buffer[offset + 2];

				glTexCoord2f (0.0, 0.0);
				glVertex2f (x - w, y - w);

				glTexCoord2f (1.0, 0.0);
				glVertex2f (x + w, y - w);

				glTexCoord2f (1.0, 1.0);
				glVertex2f (x + w, y + w);

				glTexCoord2f (0.0, 1.0);
				glVertex2f (x - w, y + w);
			}
		}

		glEnd ();

		glMatrixMode (GL_MODELVIEW);
		glPopMatrix ();
		glMatrixMode (GL_PROJECTION);
		glPopMatrix ();
	} else {
		int i;

		glClear (GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		glMatrixMode (GL_MODELVIEW);
		glLoadIdentity ();
		gluLookAt (200, 0, 0, 0, 0, 0, 0, 0, 1);

		glPushMatrix ();
		glRotated (90, 0, 1, 0);
		glRotated (10 * currentTime, 0, 0, 1);

		glColor3d (0.9, 0.7, 0.5);

		glNewList (1, GL_COMPILE);
		Horn (NRPOINTS, 315 * sin (currentTime * 0.237), 50 + 20 * sin (currentTime * 0.133), 1500 * sin (currentTime * 0.213), 7.5 + 2.5 * sin (currentTime * 0.173), 0.6 + 0.1 * sin (currentTime * 0.317));
		glEndList ();

		if (dOffAngle)
			glRotated (45, 1, 1, 0);

		for (i = 0; i < NRLINES; i++) {
			glPushMatrix ();
			glRotated (360 * i / NRLINES, 0, 0, 1);
			glRotated (45, 1, 0, 0);
			glTranslated (0, 0, 2);
			glCallList (1);
			glPopMatrix ();
		}

		glPopMatrix ();
	}
}

void setDefaults (int preset)
{
	dGeometry = preset;

	switch (preset) {
	case TRIANGLES:
		NRPOINTS = 80;
		NRLINES = 32;

		break;
	case SPHERES:
		NRPOINTS = 80;
		NRLINES = 30;

		break;
	case BIGSPHERES:
		NRPOINTS = 20;
		NRLINES = 5;

		break;
	case POINTS:
		NRPOINTS = 250;
		NRLINES = 30;

		break;
	case LIGHTMAP:
		NRPOINTS = 150;
		NRLINES = 20;

		break;
	}
}

void hack_handle_opts (int argc, char **argv)
{
	int change_flag = 0;

	setDefaults(LIGHTMAP);
	dOffAngle = 0;

	while (1) {
		int c;

#ifdef HAVE_GETOPT_H
		static struct option long_options[] = {
			{"help", 0, 0, 'h'},
			DRIVER_OPTIONS_LONG {"preset", 1, 0, 'P'},
			{"triangles", 0, 0, 0},
			{"spheres", 0, 0, 1},
			{"bigspheres", 0, 0, 2},
			{"points", 0, 0, 3},
			{"lightmap", 0, 0, 4},
			{"lines", 1, 0, 'l'},
			{"points", 1, 0, 'p'},
			{"offangle", 0, 0, 'o'},
			{0, 0, 0, 0}
		};

		c = getopt_long (argc, argv, DRIVER_OPTIONS_SHORT "hP:l:p:o", long_options, NULL);
#else
		c = getopt (argc, argv, DRIVER_OPTIONS_SHORT "hP:l:p:o");
#endif
		if (c == -1)
			break;

		switch (c) {
			DRIVER_OPTIONS_CASES case 'h':printf ("%s:"
#ifndef HAVE_GETOPT_H
							      " Not built with GNU getopt.h, long options *NOT* enabled."
#endif
							      "\n" DRIVER_OPTIONS_HELP "\t--preset/-P <arg>\n" "\t--triangles\n"
							      "\t--spheres\n" "\t--bigspheres\n" "\t--points\n" "\t--lightmap\n" 
							      "\t--lines/-l <arg>\n" "\t--points/-p <arg>\n" "\t--offangle\n", argv[0]);
			exit (1);
		case 'P':
			change_flag = 1;
			setDefaults (strtol_minmaxdef (optarg, 10, 1, 5, 0, 1, "--preset: "));
			break;
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
			change_flag = 1;
			setDefaults (c + 1);
			break;
		case 'l':
			change_flag = 1;
			NRLINES = strtol_minmaxdef (optarg, 10, 1, 1024, 0, 1, "--lines: ");
			break;
		case 'p':
			change_flag = 1;
			NRPOINTS = strtol_minmaxdef (optarg, 10, 1, 4096, 0, 1, "--points: ");
			break;
		case 'o':
			change_flag = 1;
			dOffAngle = 1;
		}
	}

	if (!change_flag) {
		dGeometry = rsRandi (5) + 1;
		dOffAngle = rsRandi (2);

		switch (dGeometry) {
		case TRIANGLES:
		case SPHERES:
			NRLINES = rsRandi (7) + 2;
			NRPOINTS = rsRandi (96) + 32;
			break;

		case BIGSPHERES:
			NRLINES = rsRandi (7) + 4;
			NRPOINTS = rsRandi (32) + 4;
			break;

		case POINTS:
		case LIGHTMAP:
			NRLINES = rsRandi (32) + 4;
			NRPOINTS = rsRandi (64) + 64;
		}
	}
}

