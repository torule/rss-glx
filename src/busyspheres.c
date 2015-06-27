/*
 * Copyright (C) 2002  <hk@dgmr.nl>
 * Ported to Linux by Tugrul Galatali <tugrul@galatali.com>
 *
 * BusySpheres is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 *
 * BusySpheres is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// BusySpheres screen saver

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "driver.h"
#include "rsDefines.h"
#include "rsRand.h"

const char *hack_name = "BusySpheres";

#define NRPOINTS 100

#define EFFECT_RANDOM		0
#define EFFECT_CRESCENT		1
#define EFFECT_DOT		2
#define EFFECT_RING		3
#define EFFECT_LONGITUDE	4

int OldMode = 0, NewMode;
float ConvertTime = 0;
float Points[NRPOINTS][4];

void hack_reshape (xstuff_t * XStuff)
{
	glViewport (0, 0, (GLsizei) XStuff->windowWidth, (GLsizei) XStuff->windowHeight);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

	gluPerspective (30, (float)XStuff->windowWidth / (float)XStuff->windowHeight, 7, 13);

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();

	glClearColor (0, 0, 0, 0);
}

void hack_init (xstuff_t * XStuff)	// Called right after the window is created, and OpenGL is initialized.
{
	int i, j;
	float x, y, r, d;
	unsigned char texbuf[64][64];

	for (i = 0; i < NRPOINTS; i++) {
		Points[i][0] = rsRandf (PIx2);
		Points[i][1] = rsRandf (PI) - 0.5 * PI;
		Points[i][2] = Points[i][0];
		Points[i][3] = Points[i][1];
	}

	glColorMaterial (GL_FRONT, GL_DIFFUSE);

	glDisable (GL_DEPTH_TEST);
	glDisable (GL_LIGHTING);

	memset ((void *)&texbuf, 0, 4096);

	r = 32;
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

	hack_reshape(XStuff);
}

void hack_cleanup (xstuff_t * XStuff)
{
}

void CalcPoints (float currentTime)
{
	float x1, x2, y1, y2;
	int i;
	float dt;

	dt = currentTime - ConvertTime;

	if (dt < 5) {
		for (i = 0; i < NRPOINTS; i++) {
			x1 = 0;
			y1 = 0;
			x2 = 0;
			y2 = 0;

			switch (NewMode) {
			case EFFECT_RANDOM:
				x1 = Points[i][2];
				y1 = Points[i][3];

				break;

			case EFFECT_CRESCENT:
				x1 = (i * 0.2 * (1 + sin (0.3 * currentTime))) / (PIx2);
				x1 = PIx2 * (x1 - (int)x1);
				y1 = PI * ((float)i / NRPOINTS - 0.5);

				break;

			case EFFECT_DOT:
				x1 = Points[i][2];
				y1 = ((Points[i][3] > 0) ? 0.5 : -0.5) * PI - Points[i][3] * 0.5 * (1 + sin (0.3 * currentTime));

				break;

			case EFFECT_RING:
				x1 = Points[i][2];
				y1 = Points[i][3] * 0.5 * (1 + sin (0.3 * currentTime));

				break;

			case EFFECT_LONGITUDE:
				x1 = i / 10.0f;
				x1 = PIx2 * (x1 - (int)x1);
				y1 = 0.9 * PI * ((float)i / NRPOINTS - 0.5);
			}

			switch (OldMode) {
			case EFFECT_RANDOM:
				x2 = Points[i][2];
				y2 = Points[i][3];

				break;

			case EFFECT_CRESCENT:
				x2 = (i * 0.2 * (1 + sin (0.3 * currentTime))) / (PIx2);
				x2 = PIx2 * (x2 - (int)x2);
				y2 = PI * ((float)i / NRPOINTS - 0.5);

				break;

			case EFFECT_DOT:
				x2 = Points[i][2];
				if (Points[i][3] > 0) {
					y2 = 0.5 * PI - Points[i][3] * 0.5 * (1 + sin (0.3 * currentTime));
				} else {
					y2 = -0.5 * PI - Points[i][3] * 0.5 * (1 + sin (0.3 * currentTime));
				}

				break;

			case EFFECT_RING:
				x2 = Points[i][2];
				y2 = Points[i][3] * 0.5 * (1 + sin (0.3 * currentTime));

				break;

			case EFFECT_LONGITUDE:
				x2 = i / 10.0f;
				x2 = PIx2 * (x1 - (int)x2);
				y2 = 0.9 * PI * ((float)i / NRPOINTS - 0.5);
			}

			Points[i][0] = 0.2 * (x1 * dt + x2 * (5 - dt));
			Points[i][1] = 0.2 * (y1 * dt + y2 * (5 - dt));
		}
	} else {
		switch (NewMode) {
		case EFFECT_RANDOM:
			for (i = 0; i < NRPOINTS; i++) {
				Points[i][0] = Points[i][2];
				Points[i][1] = Points[i][3];
			}

			break;

		case EFFECT_CRESCENT:
			for (i = 0; i < NRPOINTS; i++) {
				Points[i][0] = (i * 0.2 * (1 + sin (0.3 * currentTime))) / (PIx2);
				Points[i][0] = PIx2 * (Points[i][0] - (int)Points[i][0]);
				Points[i][1] = PI * ((float)i / NRPOINTS - 0.5);
			}

			break;

		case EFFECT_DOT:
			for (i = 0; i < NRPOINTS; i++) {
				Points[i][0] = Points[i][2];
				Points[i][1] = ((Points[i][3] > 0) ? 0.5 : -0.5) * PI - Points[i][3] * 0.5 * (1 + sin (0.3 * currentTime));
			}

			break;

		case EFFECT_RING:
			for (i = 0; i < NRPOINTS; i++) {
				Points[i][0] = Points[i][2];
				Points[i][1] = Points[i][3] * 0.5 * (1 + sin (0.3 * currentTime));
			}

			break;

		case EFFECT_LONGITUDE:
			for (i = 0; i < NRPOINTS; i++) {
				Points[i][0] = i / 10.0f;
				Points[i][0] = PIx2 * (Points[i][0] - (int)Points[i][0]);
				Points[i][1] = 0.9 * PI * ((float)i / NRPOINTS - 0.5);
			}
		}
	}
}

void hack_draw (xstuff_t * XStuff, double currentTime, float frameTime)
{
	float x, y, z, w, ws;
	int i, k;
	float st2, ct2, t, t2;
	float fb_buffer[NRPOINTS * 37 * 4];

	currentTime = currentTime - (int)currentTime + (int)currentTime % 86400;

	t = 5 * currentTime + 0.35 * (cos (currentTime * 0.41 + 0.123) + cos (currentTime * 0.51 + 0.234) + cos (currentTime * 0.61 + 0.623) + cos (currentTime * 0.21 + 0.723));
	t2 = 0.3 * currentTime;

	if ((currentTime - ConvertTime) > 10) {
		OldMode = NewMode;
		NewMode = rsRandi (5);
		ConvertTime = currentTime;
	}

	CalcPoints(currentTime);

	glClear (GL_COLOR_BUFFER_BIT);

	// Setup Feedback buffer for retrieval
	glFeedbackBuffer (NRPOINTS * 37 * 4, GL_3D, fb_buffer);
	glRenderMode (GL_FEEDBACK);

	// set camera position
	glMatrixMode (GL_MODELVIEW);
	glPushMatrix ();
	glLoadIdentity ();

	gluLookAt (10, 0, 0, 0, 0, 0, 0, 0, 1);

	glRotatef (2.4 * t, 1, 0, 0);
	glRotatef (2.5 * t, 0, 1, 0);
	glRotatef (2.6 * t, 0, 0, 1);

	st2 = sin (t2);
	ct2 = cos (t2);

	// Generate the points
	glBegin (GL_POINTS);
	for (i = 0; i < NRPOINTS; i++) {
		x = sin (Points[i][0]) * cos (Points[i][1]);
		y = cos (Points[i][0]) * cos (Points[i][1]);
		z = sin (Points[i][1]);

		glVertex3f (x, y, z);

		x = 0.5 * x;
		y = 0.5 * y;
		z = 0.5 * z;

		glVertex3f (x + 1.5, y, z);
		glVertex3f (x - 1.5, y, z);
		glVertex3f (x, y + 1.5, z);
		glVertex3f (x, y - 1.5, z);
		glVertex3f (x, y, z + 1.5);
		glVertex3f (x, y, z - 1.5);

		x = 0.5 * x;
		y = 0.5 * y;
		z = 0.5 * z;

		glVertex3f (x + 2.25, y, z);
		glVertex3f (x + 1.5, y + 0.75 * st2, z - 0.75 * ct2);
		glVertex3f (x + 1.5, y - 0.75 * ct2, z - 0.75 * st2);
		glVertex3f (x + 1.5, y - 0.75 * st2, z + 0.75 * ct2);
		glVertex3f (x + 1.5, y + 0.75 * ct2, z + 0.75 * st2);

		glVertex3f (x - 2.25, y, z);
		glVertex3f (x - 1.5, y - 0.75 * st2, z - 0.75 * ct2);
		glVertex3f (x - 1.5, y + 0.75 * ct2, z - 0.75 * st2);
		glVertex3f (x - 1.5, y + 0.75 * st2, z + 0.75 * ct2);
		glVertex3f (x - 1.5, y - 0.75 * ct2, z + 0.75 * st2);

		glVertex3f (x, y + 2.25, z);
		glVertex3f (x + 0.75 * st2, y + 1.5, z - 0.75 * ct2);
		glVertex3f (x - 0.75 * ct2, y + 1.5, z - 0.75 * st2);
		glVertex3f (x - 0.75 * st2, y + 1.5, z + 0.75 * ct2);
		glVertex3f (x + 0.75 * ct2, y + 1.5, z + 0.75 * st2);

		glVertex3f (x, y - 2.25, z);
		glVertex3f (x - 0.75 * st2, y - 1.5, z - 0.75 * ct2);
		glVertex3f (x + 0.75 * ct2, y - 1.5, z - 0.75 * st2);
		glVertex3f (x + 0.75 * st2, y - 1.5, z + 0.75 * ct2);
		glVertex3f (x - 0.75 * ct2, y - 1.5, z + 0.75 * st2);

		glVertex3f (x, y, z + 2.25);
		glVertex3f (x + 0.75 * st2, y - 0.75 * ct2, z + 1.5);
		glVertex3f (x - 0.75 * ct2, y - 0.75 * st2, z + 1.5);
		glVertex3f (x - 0.75 * st2, y + 0.75 * ct2, z + 1.5);
		glVertex3f (x + 0.75 * ct2, y + 0.75 * st2, z + 1.5);

		glVertex3f (x, y, z - 2.25);
		glVertex3f (x - 0.75 * st2, y - 0.75 * ct2, z - 1.5);
		glVertex3f (x + 0.75 * ct2, y - 0.75 * st2, z - 1.5);
		glVertex3f (x + 0.75 * st2, y + 0.75 * ct2, z - 1.5);
		glVertex3f (x - 0.75 * ct2, y + 0.75 * st2, z - 1.5);
	}
	glEnd ();

	glPopMatrix();

#define Wi 0
#define Xi 1
#define Yi 2
#define Zi 3
#define Ri 4
#define Gi 5
#define Bi 6

	glRenderMode (GL_RENDER);

	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity ();

	glScalef (2.0 / XStuff->windowWidth, 2.0 / XStuff->windowHeight, 1.0);
	glTranslatef (-0.5 * XStuff->windowWidth, -0.5 * XStuff->windowHeight, 0);

	glEnable (GL_TEXTURE_2D);
	glBlendFunc (GL_ONE, GL_ONE);
	glEnable (GL_BLEND);

	// Draw objects from back to front
	glBegin (GL_QUADS);
	for (i = 0, k = 0; i < NRPOINTS * 37; i++, k++) {
		if (k == 37)
			k = 0;

		x = fb_buffer[4 * i + 1];
		y = fb_buffer[4 * i + 2];
		z = fb_buffer[4 * i + 3];

		w = 1.3 - z;	// diminishing fading
		ws = 0.002 * XStuff->windowHeight * (1 - z);	// Keep Bitmaps same size realive to screensize

		if (k == 0) {	// big sphere
			glColor3f (0.6 * w, 0.5 * w, 0.3 * w);
			w = 70 * ws;
		} else if (k < 7) {
			glColor3f (0.3 * w, 0.6 * w, 0.4 * w);
			w = 30 * ws;
		} else {
			glColor3f (0.2 * w, 0.3 * w, 0.4 * w);
			w = 12 * ws;
		}

		glTexCoord2f (0.0, 0.0);
		glVertex2f (x - w, y - w);

		glTexCoord2f (1.0, 0.0);
		glVertex2f (x + w, y - w);

		glTexCoord2f (1.0, 1.0);
		glVertex2f (x + w, y + w);

		glTexCoord2f (0.0, 1.0);
		glVertex2f (x - w, y + w);
	}
	glEnd ();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
}

void hack_handle_opts (int argc, char **argv)
{
	while (1) {
		int c;

#ifdef HAVE_GETOPT_H
		static struct option long_options[] = {
			{"help", 0, 0, 'h'},
			DRIVER_OPTIONS_LONG {0, 0, 0, 0}
		};

		c = getopt_long (argc, argv, DRIVER_OPTIONS_SHORT "hq:s:t:f:b:eE", long_options, NULL);
#else
		c = getopt (argc, argv, DRIVER_OPTIONS_SHORT "hq:s:t:f:b:eE");
#endif
		if (c == -1)
			break;

		switch (c) {
			DRIVER_OPTIONS_CASES case 'h':printf ("%s:"
#ifndef HAVE_GETOPT_H
							      " Not built with GNU getopt.h, long options *NOT* enabled."
#endif
							      "\n" DRIVER_OPTIONS_HELP, argv[0]);
			exit (1);
		}
	}
}
