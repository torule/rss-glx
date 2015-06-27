/*
 * Copyright (C) 2002  Terence M. Welsh
 * Ported to Linux by Tugrul Galatali <tugrul@galatali.com>
 *
 * Plasma is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 *
 * Plasma is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Plasma screen saver

#include <math.h>
#include <stdio.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "driver.h"
#include "rsDefines.h"
#include "rsRand.h"

const char *hack_name = "plasma";

#define NUMCONSTS 18
#define MAXTEXSIZE 1024

// Globals
float aspectRatio;
float wide;
float high;
float c[NUMCONSTS];		// constant
float ct[NUMCONSTS];		// temporary value of constant
float cv[NUMCONSTS];		// velocity of constant
float ***position;
float ***plasma;
unsigned int tex;
int texsize = 256;
int plasmaWidth, plasmaHeight;
float texright, textop;
float *plasmamap;

// Parameters edited in the dialog box
int dZoom;
int dFocus;
int dSpeed;
int dResolution;

// Find absolute value and truncate to 1.0
#define fabstrunc(f) (f >= 0.0f ? (f <= 1.0f ? f : 1.0f) : (f >= -1.0f ? -f : 1.0f))

/*
float fabstrunc (float f)
{
	if (f >= 0.0f) {
		return (f <= 1.0f ? f : 1.0f);
	} else {
		return (f >= -1.0f ? -f : 1.0f);
	}
}
*/

void hack_draw (xstuff_t * XStuff, double currentTime, float frameTime)
{
	int i, j;
	float rgb[3];
	float temp;
	static float focus = float (dFocus) / 50.0f + 0.3f;
	static float maxdiff = 0.004f * float (dSpeed);
	static int index;

	// Update constants
	for (i = 0; i < NUMCONSTS; i++) {
		ct[i] += cv[i];
		if (ct[i] > PIx2)
			ct[i] -= PIx2;
		c[i] = sin (ct[i]) * focus;
	}

	// Update colors
	for (i = 0; i < plasmaHeight; i++) {
		for (j = 0; j < plasmaWidth; j++) {
			// Calculate vertex colors
			rgb[0] = plasma[i][j][0];
			rgb[1] = plasma[i][j][1];
			rgb[2] = plasma[i][j][2];
			plasma[i][j][0] = 0.7f * (c[0] * position[i][j][0] + c[1] * position[i][j][1]
						  + c[2] * (position[i][j][0] * position[i][j][0] + 1.0f)
						  + c[3] * position[i][j][0] * position[i][j][1]
						  + c[4] * rgb[1] + c[5] * rgb[2]);
			plasma[i][j][1] = 0.7f * (c[6] * position[i][j][0] + c[7] * position[i][j][1]
						  + c[8] * position[i][j][0] * position[i][j][0]
						  + c[9] * (position[i][j][1] * position[i][j][1] - 1.0f)
						  + c[10] * rgb[0] + c[11] * rgb[2]);
			plasma[i][j][2] = 0.7f * (c[12] * position[i][j][0] + c[13] * position[i][j][1]
						  + c[14] * (1.0f - position[i][j][0] * position[i][j][1])
						  + c[15] * position[i][j][1] * position[i][j][1]
						  + c[16] * rgb[0] + c[17] * rgb[1]);

			// Don't let the colors change too much
			temp = plasma[i][j][0] - rgb[0];
			if (temp > maxdiff)
				plasma[i][j][0] = rgb[0] + maxdiff;
			if (temp < -maxdiff)
				plasma[i][j][0] = rgb[0] - maxdiff;
			temp = plasma[i][j][1] - rgb[1];
			if (temp > maxdiff)
				plasma[i][j][1] = rgb[1] + maxdiff;
			if (temp < -maxdiff)
				plasma[i][j][1] = rgb[1] - maxdiff;
			temp = plasma[i][j][2] - rgb[2];
			if (temp > maxdiff)
				plasma[i][j][2] = rgb[2] + maxdiff;
			if (temp < -maxdiff)
				plasma[i][j][2] = rgb[2] - maxdiff;

			// Put colors into texture
			index = (i * texsize + j) * 3;
			plasmamap[index] = fabstrunc (plasma[i][j][0]);
			plasmamap[index + 1] = fabstrunc (plasma[i][j][1]);
			plasmamap[index + 2] = fabstrunc (plasma[i][j][2]);
		}
	}

	// Update texture
	glPixelStorei(GL_UNPACK_ROW_LENGTH, texsize);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, plasmaWidth, plasmaHeight, GL_RGB, GL_FLOAT, plasmamap);

	// Draw it
	glBegin (GL_TRIANGLE_STRIP);
	glTexCoord2f (0.0f, 0.0f);
	glVertex2f (0.0f, 0.0f);
	glTexCoord2f (0.0f, texright);
	glVertex2f (1.0f, 0.0f);
	glTexCoord2f (textop, 0.0f);
	glVertex2f (0.0f, 1.0f);
	glTexCoord2f (textop, texright);
	glVertex2f (1.0f, 1.0f);
	glEnd ();
}

void hack_reshape (xstuff_t * XStuff)
{
	// Window initialization
	glViewport (0, 0, XStuff->windowWidth, XStuff->windowHeight);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	gluOrtho2D (0.0f, 1.0f, 0.0f, 1.0f);
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
}

void hack_init (xstuff_t * XStuff)
{
	int i, j;

	hack_reshape (XStuff);

	aspectRatio = float (XStuff->windowWidth) / float (XStuff->windowHeight);

	if (aspectRatio >= 1.0f) {
		wide = 30.0f / float (dZoom);

		high = wide / aspectRatio;
	} else {
		high = 30.0f / float (dZoom);

		wide = high * aspectRatio;
	}

	// Set resolution of plasma
	if (aspectRatio >= 1.0f)
		plasmaHeight = (dResolution * MAXTEXSIZE) / 100;
	else
		plasmaHeight = int (float (dResolution * MAXTEXSIZE) * aspectRatio * 0.01f);

	plasmaWidth = int (float (plasmaHeight) / aspectRatio);

	// Set resolution of texture
	texsize = 16;
	if (aspectRatio >= 1.0f)
		while (plasmaHeight > texsize)
			texsize *= 2;
	else
		while (plasmaWidth > texsize)
			texsize *= 2;

	// The "- 1" cuts off right and top edges to get rid of blending to black
	texright = float (plasmaHeight - 1) / float (texsize);
	textop = texright / aspectRatio;

	// Initialize memory and positions
	plasmamap = new float[texsize * texsize * 3];

	for (i = 0; i < texsize * texsize * 3; i++)
		plasmamap[i] = 0.0f;
	plasma = new float **[plasmaHeight];
	position = new float **[plasmaHeight];

	for (i = 0; i < plasmaHeight; i++) {
		plasma[i] = new float *[plasmaWidth];
		position[i] = new float *[plasmaWidth];

		for (j = 0; j < plasmaWidth; j++) {
			plasma[i][j] = new float[3];
			position[i][j] = new float[2];

			plasma[i][j][0] = 0.0f;
			plasma[i][j][1] = 0.0f;
			plasma[i][j][2] = 0.0f;
			position[i][j][0] = float (i * wide) / float (plasmaHeight - 1) - (wide * 0.5f);
			position[i][j][1] = float (j * high) / (float(plasmaHeight) / aspectRatio - 1.0f) - (high * 0.5f);
		}
	}
	// Initialize constants
	for (i = 0; i < NUMCONSTS; i++) {
		ct[i] = rsRandf (PIx2);
		cv[i] = rsRandf (0.005f * dSpeed) + 0.0001f;
	}

	// Make texture
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D (GL_TEXTURE_2D, 0, 3, texsize, texsize, 0, GL_RGB, GL_FLOAT, plasmamap);
	glEnable (GL_TEXTURE_2D);
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
}

void hack_cleanup (xstuff_t * XStuff)
{
}

void hack_handle_opts (int argc, char **argv)
{
	dZoom = 10;
	dFocus = 30;
	dSpeed = 20;
	dResolution = 25;

	while (1) {
		int c;

#ifdef HAVE_GETOPT_H
		static struct option long_options[] = {
			{"help", 0, 0, 'h'},
			DRIVER_OPTIONS_LONG 
			{"zoom", 1, 0, 'z'},
			{"focus", 1, 0, 'f'},
			{"speed", 1, 0, 's'},
			{"resolution", 1, 0, 'R'},
			{0, 0, 0, 0}
		};

		c = getopt_long (argc, argv, DRIVER_OPTIONS_SHORT "hz:f:s:R:", long_options, NULL);
#else
		c = getopt (argc, argv, DRIVER_OPTIONS_SHORT "hz:f:s:R:");
#endif
		if (c == -1)
			break;

		switch (c) {
			DRIVER_OPTIONS_CASES case 'h':printf ("%s:"
#ifndef HAVE_GETOPT_H
							      " Not built with GNU getopt.h, long options *NOT* enabled."
#endif
							      "\n" DRIVER_OPTIONS_HELP "\t--zoom/-z <arg>\n" "\t--focus/-f <arg>\n" "\t--speed/-s <arg>\n"
							      "\t--resolution/-R <arg>\n", argv[0]);
			exit (1);
		case 'z':
			dZoom = strtol_minmaxdef (optarg, 10, 1, 100, 1, 10, "--zoom: ");
			break;
		case 'f':
			dFocus = strtol_minmaxdef (optarg, 10, 1, 100, 1, 30, "--focus: ");
			break;
		case 's':
			dSpeed = strtol_minmaxdef (optarg, 10, 1, 100, 1, 20, "--speed: ");
			break;
		case 'R':
			dResolution = strtol_minmaxdef (optarg, 10, 1, 100, 1, 25, "--resolution: ");
			break;
		}
	}
}
