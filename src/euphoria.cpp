/*
 * Copyright (C) 2002  Terence M. Welsh
 * Ported to Linux by Tugrul Galatali <tugrul@galatali.com>
 *
 * Euphoria is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Euphoria is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Euphoria screensaver
 */

const char *hack_name = "euphoria";

#include <stdio.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "driver.h"
#include "euphoria_textures.h"
#include "loadTexture.h"
#include "rgbhsl.h"
#include "rsDefines.h"
#include "rsRand.h"
#include <rsMath/rsVec.h>

#define PRESET_REGULAR 1
#define PRESET_GRID 2
#define PRESET_CUBISM 3
#define PRESET_BADMATH 4
#define PRESET_MTHEORY 5
#define PRESET_UHFTEM 6
#define PRESET_NOWHERE 7
#define PRESET_ECHO 8
#define PRESET_KALEIDOSCOPE 9

#define NUMCONSTS 9
#define TEXSIZE 256

typedef struct {
	float ***vertices;
	float c[NUMCONSTS];	/* constants */
	float cr[NUMCONSTS];	/* constants radial position */
	float cv[NUMCONSTS];	/* constants change velocities */
	float hsl[3];
	float rgb[3];
	float hueSpeed;
	float saturationSpeed;
} wisp;

wisp *backwisps;
wisp *wisps;
unsigned int tex;
float aspectRatio;
int viewport[4];
float elapsedTime;

/*
 * Parameters edited in the dialog box
 */
int dWisps;
int dBgWisps;
int dDensity;
int dVisibility;
int dSpeed;
int dFeedback;
int dFeedbackspeed;
int dFeedbacksize;
int dWireframe;
int dTexture;
int dPriority;

/*
 * feedback texture object
 */
unsigned int feedbacktex;
unsigned int feedbacktexsize;

/*
 * feedback variables
 */
float fr[4];
float fv[4];
float f[4];

/*
 * feedback limiters
 */
float lr[3];
float lv[3];
float l[3];

void setDefaults (int which)
{
	switch (which) {
	case PRESET_REGULAR:	/* Regular */
		dWisps = 5;
		dBgWisps = 0;
		dDensity = 35;
		dVisibility = 35;
		dSpeed = 15;
		dFeedback = 0;
		dFeedbackspeed = 1;
		dFeedbacksize = 10;
		dWireframe = 0;
		dTexture = 2;
		break;
	case PRESET_GRID:	/* Grid */
		dWisps = 4;
		dBgWisps = 1;
		dDensity = 30;
		dVisibility = 70;
		dSpeed = 15;
		dFeedback = 0;
		dFeedbackspeed = 1;
		dFeedbacksize = 10;
		dWireframe = 1;
		dTexture = 0;
		break;
	case PRESET_CUBISM:	/* Cubism */
		dWisps = 15;
		dBgWisps = 0;
		dDensity = 4;
		dVisibility = 15;
		dSpeed = 10;
		dFeedback = 0;
		dFeedbackspeed = 1;
		dFeedbacksize = 10;
		dWireframe = 0;
		dTexture = 0;
		break;
	case PRESET_BADMATH:	/* Bad math */
		dWisps = 2;
		dBgWisps = 2;
		dDensity = 20;
		dVisibility = 40;
		dSpeed = 30;
		dFeedback = 40;
		dFeedbackspeed = 5;
		dFeedbacksize = 10;
		dWireframe = 1;
		dTexture = 2;
		break;
	case PRESET_MTHEORY:	/* M-Theory */
		dWisps = 3;
		dBgWisps = 0;
		dDensity = 35;
		dVisibility = 15;
		dSpeed = 20;
		dFeedback = 40;
		dFeedbackspeed = 20;
		dFeedbacksize = 10;
		dWireframe = 0;
		dTexture = 0;
		break;
	case PRESET_UHFTEM:	/* ultra high frequency tunneling electron microscope */
		dWisps = 0;
		dBgWisps = 3;
		dDensity = 35;
		dVisibility = 5;
		dSpeed = 50;
		dFeedback = 0;
		dFeedbackspeed = 1;
		dFeedbacksize = 10;
		dWireframe = 0;
		dTexture = 0;
		break;
	case PRESET_NOWHERE:	/* Nowhere */
		dWisps = 0;
		dBgWisps = 3;
		dDensity = 30;
		dVisibility = 40;
		dSpeed = 20;
		dFeedback = 80;
		dFeedbackspeed = 10;
		dFeedbacksize = 10;
		dWireframe = 1;
		dTexture = 3;
		break;
	case PRESET_ECHO:	/* Echo */
		dWisps = 3;
		dBgWisps = 0;
		dDensity = 35;
		dVisibility = 30;
		dSpeed = 20;
		dFeedback = 85;
		dFeedbackspeed = 30;
		dFeedbacksize = 10;
		dWireframe = 0;
		dTexture = 1;
		break;
	case PRESET_KALEIDOSCOPE:	/* Kaleidoscope */
		dWisps = 3;
		dBgWisps = 0;
		dDensity = 35;
		dVisibility = 40;
		dSpeed = 15;
		dFeedback = 90;
		dFeedbackspeed = 3;
		dFeedbacksize = 10;
		dWireframe = 0;
		dTexture = 0;
		break;
	}
}

void wisp_new (wisp * w)
{
	int i, j;
	float recHalfDens = 1.0f / (dDensity * 0.5f);

	w->vertices = (float ***)malloc (sizeof (float **) * (dDensity + 1));
	for (i = 0; i <= dDensity; i++) {
		w->vertices[i] = (float **)malloc (sizeof (float *) * (dDensity + 1));
		for (j = 0; j <= dDensity; j++) {
			w->vertices[i][j] = (float *)malloc (sizeof (float) * 7);
			w->vertices[i][j][3] = i * recHalfDens - 1.0f;	/* x position on grid */
			w->vertices[i][j][4] = j * recHalfDens - 1.0f;	/* y position on grid */

			/*
			 * distance squared from the center
			 */
			w->vertices[i][j][5] = w->vertices[i][j][3] * w->vertices[i][j][3] + w->vertices[i][j][4] * w->vertices[i][j][4];

			w->vertices[i][j][6] = 0.0f;	/* intensity */
		}
	}

	/*
	 * initialize constants
	 */
	for (i = 0; i < NUMCONSTS; i++) {
		w->c[i] = rsRandf (2.0f) - 1.0f;
		w->cr[i] = rsRandf (PIx2);
		w->cv[i] = rsRandf (dSpeed * 0.03f) + (dSpeed * 0.001f);
	}

	/*
	 * pick color
	 */
	w->hsl[0] = rsRandf (1.0f);
	w->hsl[1] = 0.1f + rsRandf (0.9f);
	w->hsl[2] = 1.0f;
	w->hueSpeed = rsRandf (0.1f) - 0.05f;
	w->saturationSpeed = rsRandf (0.04f) + 0.001f;
}

void delete_wisp (wisp * w)
{
	int i, j;

	for (i = 0; i <= dDensity; i++) {
		for (j = 0; j <= dDensity; j++) {
			free (w->vertices[i][j]);
		}
		free (w->vertices[i]);
	}
	free (w->vertices);
}

void wisp_update (wisp * w)
{
	int i, j;
	rsVec up, right, crossvec;

	/*
	 * visibility constants
	 */
	const float viscon1 = dVisibility * 0.01f;
	const float viscon2 = 1.0f / viscon1;

	/*
	 * update constants
	 */
	for (i = 0; i < NUMCONSTS; i++) {
		w->cr[i] += w->cv[i] * elapsedTime;
		if (w->cr[i] > PIx2)
			w->cr[i] -= PIx2;
		w->c[i] = cos (w->cr[i]);
	}

	/*
	 * update vertex positions
	 */
	for (i = 0; i <= dDensity; i++) {
		for (j = 0; j <= dDensity; j++) {
			w->vertices[i][j][0] = w->vertices[i][j][3] * w->vertices[i][j][3] * w->vertices[i][j][4] * w->c[0] + w->vertices[i][j][5] * w->c[1] + 0.5f * w->c[2];
			w->vertices[i][j][1] = w->vertices[i][j][4] * w->vertices[i][j][4] * w->vertices[i][j][5] * w->c[3] + w->vertices[i][j][3] * w->c[4] + 0.5f * w->c[5];
			w->vertices[i][j][2] = w->vertices[i][j][5] * w->vertices[i][j][5] * w->vertices[i][j][3] * w->c[6] + w->vertices[i][j][4] * w->c[7] + w->c[8];
		}
	}

	/*
	 * update vertex normals for most of mesh
	 */
	for (i = 1; i < dDensity; i++) {
		for (j = 1; j < dDensity; j++) {
			up[0] = w->vertices[i][j + 1][0] - w->vertices[i][j - 1][0];
			up[1] = w->vertices[i][j + 1][1] - w->vertices[i][j - 1][1];
			up[2] = w->vertices[i][j + 1][2] - w->vertices[i][j - 1][2];

			right[0] = w->vertices[i + 1][j][0] - w->vertices[i - 1][j][0];
			right[1] = w->vertices[i + 1][j][1] - w->vertices[i - 1][j][1];
			right[2] = w->vertices[i + 1][j][2] - w->vertices[i - 1][j][2];

			up.normalize();
			right.normalize();
			crossvec.cross(right, up);

			/*
			 * Use depth component of normal to compute intensity
			 */
			/*
			 * This way only edges of wisp are bright
			 */
			if (crossvec[2] < 0.0f)
				crossvec[2] *= -1.0f;
			w->vertices[i][j][6] = viscon2 * (viscon1 - crossvec[2]);
			if (w->vertices[i][j][6] > 1.0f)
				w->vertices[i][j][6] = 1.0f;
			if (w->vertices[i][j][6] < 0.0f)
				w->vertices[i][j][6] = 0.0f;
		}
	}

	/*
	 * update color
	 */
	w->hsl[0] += w->hueSpeed * elapsedTime;
	if (w->hsl[0] < 0.0f)
		w->hsl[0] += 1.0f;
	if (w->hsl[0] > 1.0f)
		w->hsl[0] -= 1.0f;
	w->hsl[1] += w->saturationSpeed * elapsedTime;
	if (w->hsl[1] <= 0.1f) {
		w->hsl[1] = 0.1f;
		w->saturationSpeed = -w->saturationSpeed;
	}
	if (w->hsl[1] >= 1.0f) {
		w->hsl[1] = 1.0f;
		w->saturationSpeed = -w->saturationSpeed;
	}
	hsl2rgb (w->hsl[0], w->hsl[1], w->hsl[2], w->rgb[0], w->rgb[1], w->rgb[2]);
}

void wisp_draw (wisp * w)
{
	int i, j;

	glPushMatrix ();

	if (dWireframe) {
		for (i = 1; i <= dDensity; i++) {
			glBegin (GL_LINE_STRIP);
			for (j = 0; j <= dDensity; j++) {
				glColor3f (w->rgb[0] + w->vertices[i][j][6] - 1.0f, w->rgb[1] + w->vertices[i][j][6] - 1.0f, w->rgb[2] + w->vertices[i][j][6] - 1.0f);
				glTexCoord2d (w->vertices[i][j][3] - w->vertices[i][j][0], w->vertices[i][j][4] - w->vertices[i][j][1]);
				glVertex3fv (w->vertices[i][j]);
			}
			glEnd ();
		}
		for (j = 1; j <= dDensity; j++) {
			glBegin (GL_LINE_STRIP);
			for (i = 0; i <= dDensity; i++) {
				glColor3f (w->rgb[0] + w->vertices[i][j][6] - 1.0f, w->rgb[1] + w->vertices[i][j][6] - 1.0f, w->rgb[2] + w->vertices[i][j][6] - 1.0f);
				glTexCoord2d (w->vertices[i][j][3] - w->vertices[i][j][0], w->vertices[i][j][4] - w->vertices[i][j][1]);
				glVertex3fv (w->vertices[i][j]);
			}
			glEnd ();
		}
	} else {
		for (i = 0; i < dDensity; i++) {
			glBegin (GL_TRIANGLE_STRIP);
			for (j = 0; j <= dDensity; j++) {
				glColor3f (w->rgb[0] + w->vertices[i + 1][j][6] - 1.0f, w->rgb[1] + w->vertices[i + 1][j][6] - 1.0f, w->rgb[2] + w->vertices[i + 1][j][6] - 1.0f);
				glTexCoord2d (w->vertices[i + 1][j][3] - w->vertices[i + 1][j][0], w->vertices[i + 1][j][4] - w->vertices[i + 1][j][1]);
				glVertex3fv (w->vertices[i + 1][j]);
				glColor3f (w->rgb[0] + w->vertices[i][j][6] - 1.0f, w->rgb[1] + w->vertices[i][j][6] - 1.0f, w->rgb[2] + w->vertices[i][j][6] - 1.0f);
				glTexCoord2d (w->vertices[i][j][3] - w->vertices[i][j][0], w->vertices[i][j][4] - w->vertices[i][j][1]);
				glVertex3fv (w->vertices[i][j]);
			}
			glEnd ();
		}
	}

	glPopMatrix ();
}

void wisp_drawAsBackground (wisp * w)
{
	int i, j;

	glPushMatrix ();
	glTranslatef (w->c[0] * 0.2f, w->c[1] * 0.2f, 1.6f);

	if (dWireframe) {
		for (i = 0; i <= dDensity; i++) {
			glBegin (GL_LINE_STRIP);
			for (j = 0; j <= dDensity; j++) {
				glColor3f (w->rgb[0] + w->vertices[i][j][6] - 1.0f, w->rgb[1] + w->vertices[i][j][6] - 1.0f, w->rgb[2] + w->vertices[i][j][6] - 1.0f);
				glTexCoord2d (w->vertices[i][j][3] - w->vertices[i][j][0], w->vertices[i][j][4] - w->vertices[i][j][1]);
				glVertex3f (w->vertices[i][j][3], w->vertices[i][j][4], w->vertices[i][j][6]);
			}
			glEnd ();
		}
		for (j = 0; j <= dDensity; j++) {
			glBegin (GL_LINE_STRIP);
			for (i = 0; i <= dDensity; i++) {
				glColor3f (w->rgb[0] + w->vertices[i][j][6] - 1.0f, w->rgb[1] + w->vertices[i][j][6] - 1.0f, w->rgb[2] + w->vertices[i][j][6] - 1.0f);
				glTexCoord2d (w->vertices[i][j][3] - w->vertices[i][j][0], w->vertices[i][j][4] - w->vertices[i][j][1]);
				glVertex3f (w->vertices[i][j][3], w->vertices[i][j][4], w->vertices[i][j][6]);
			}
			glEnd ();
		}
	} else {
		for (i = 0; i < dDensity; i++) {
			glBegin (GL_TRIANGLE_STRIP);
			for (j = 0; j <= dDensity; j++) {
				glColor3f (w->rgb[0] + w->vertices[i + 1][j][6] - 1.0f, w->rgb[1] + w->vertices[i + 1][j][6] - 1.0f, w->rgb[2] + w->vertices[i + 1][j][6] - 1.0f);
				glTexCoord2d (w->vertices[i + 1][j][3] - w->vertices[i + 1][j][0], w->vertices[i + 1][j][4] - w->vertices[i + 1][j][1]);
				glVertex3f (w->vertices[i + 1][j][3], w->vertices[i + 1][j][4], w->vertices[i + 1][j][6]);
				glColor3f (w->rgb[0] + w->vertices[i][j][6] - 1.0f, w->rgb[1] + w->vertices[i][j][6] - 1.0f, w->rgb[2] + w->vertices[i][j][6] - 1.0f);
				glTexCoord2d (w->vertices[i][j][3] - w->vertices[i][j][0], w->vertices[i][j][4] - w->vertices[i][j][1]);
				glVertex3f (w->vertices[i][j][3], w->vertices[i][j][4], w->vertices[i][j][6]);
			}
			glEnd ();
		}
	}

	glPopMatrix ();
}

void hack_draw (xstuff_t * XStuff, double currentTime, float frameTime)
{
	int i;
#ifdef BENCHMARK
	static int a = 1;
#endif

	if (frameTime > 0)
		elapsedTime = frameTime;

#ifdef BENCHMARK
	elapsedTime = 0.001;
#endif

	/*
	 * Update wisps
	 */
	for (i = 0; i < dWisps; i++)
		wisp_update (&wisps[i]);
	for (i = 0; i < dBgWisps; i++)
		wisp_update (&backwisps[i]);

	if (dFeedback) {
		float feedbackIntensity = dFeedback / 101.0f;

		/*
		 * update feedback variables
		 */
		for (i = 0; i < 4; i++) {
			fr[i] += elapsedTime * fv[i];
			if (fr[i] > PIx2)
				fr[i] -= PIx2;
		}
		f[0] = 30.0f * cos (fr[0]);
		f[1] = 0.2f * cos (fr[1]);
		f[2] = 0.2f * cos (fr[2]);
		f[3] = 0.8f * cos (fr[3]);
		for (i = 0; i < 3; i++) {
			lr[i] += elapsedTime * lv[i];
			if (lr[i] > PIx2)
				lr[i] -= PIx2;
			l[i] = cos (lr[i]);
			l[i] = l[i] * l[i];
		}

		/*
		 * Create drawing area for feedback texture
		 */
		glViewport (0, 0, feedbacktexsize, feedbacktexsize);
		glMatrixMode (GL_PROJECTION);
		glLoadIdentity ();
		gluPerspective (30.0, aspectRatio, 0.01f, 20.0f);
		glMatrixMode (GL_MODELVIEW);

		/*
		 * Draw
		 */
		glClear (GL_COLOR_BUFFER_BIT);
		glColor3f (feedbackIntensity, feedbackIntensity, feedbackIntensity);
		glBindTexture (GL_TEXTURE_2D, feedbacktex);
		glPushMatrix ();
		glTranslatef (f[1] * l[1], f[2] * l[1], f[3] * l[2]);
		glRotatef (f[0] * l[0], 0, 0, 1);
		glBegin (GL_TRIANGLE_STRIP);
		glTexCoord2f (-0.5f, -0.5f);
		glVertex3f (-aspectRatio * 2.0f, -2.0f, 1.25f);
		glTexCoord2f (1.5f, -0.5f);
		glVertex3f (aspectRatio * 2.0f, -2.0f, 1.25f);
		glTexCoord2f (-0.5f, 1.5f);
		glVertex3f (-aspectRatio * 2.0f, 2.0f, 1.25f);
		glTexCoord2f (1.5f, 1.5f);
		glVertex3f (aspectRatio * 2.0f, 2.0f, 1.25f);
		glEnd ();
		glPopMatrix ();
		glBindTexture (GL_TEXTURE_2D, tex);
		for (i = 0; i < dBgWisps; i++)
			wisp_drawAsBackground (&backwisps[i]);
		for (i = 0; i < dWisps; i++)
			wisp_draw (&wisps[i]);

		/*
		 * readback feedback texture
		 */
		glReadBuffer (GL_BACK);
		glPixelStorei (GL_UNPACK_ROW_LENGTH, feedbacktexsize);
		glBindTexture (GL_TEXTURE_2D, feedbacktex);
		glCopyTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 0, 0, feedbacktexsize, feedbacktexsize);

		/*
		 * create regular drawing area
		 */
		glViewport (0, 0, XStuff->windowWidth, XStuff->windowHeight);
		glMatrixMode (GL_PROJECTION);
		glLoadIdentity ();
		gluPerspective (20.0, aspectRatio, 0.01f, 20.0f);
		glMatrixMode (GL_MODELVIEW);

		/*
		 * Draw again
		 */
		glClear (GL_COLOR_BUFFER_BIT);
		glColor3f (feedbackIntensity, feedbackIntensity, feedbackIntensity);
		glPushMatrix ();
		glTranslatef (f[1] * l[1], f[2] * l[1], f[3] * l[2]);
		glRotatef (f[0] * l[0], 0, 0, 1);
		glBegin (GL_TRIANGLE_STRIP);
		glTexCoord2f (-0.5f, -0.5f);
		glVertex3f (-aspectRatio * 2.0f, -2.0f, 1.25f);
		glTexCoord2f (1.5f, -0.5f);
		glVertex3f (aspectRatio * 2.0f, -2.0f, 1.25f);
		glTexCoord2f (-0.5f, 1.5f);
		glVertex3f (-aspectRatio * 2.0f, 2.0f, 1.25f);
		glTexCoord2f (1.5f, 1.5f);
		glVertex3f (aspectRatio * 2.0f, 2.0f, 1.25f);
		glEnd ();
		glPopMatrix ();

		glBindTexture (GL_TEXTURE_2D, tex);
	} else
		glClear (GL_COLOR_BUFFER_BIT);

	for (i = 0; i < dBgWisps; i++)
		wisp_drawAsBackground (&backwisps[i]);
	for (i = 0; i < dWisps; i++)
		wisp_draw (&wisps[i]);

	glFlush ();

#ifdef BENCHMARK
	if (a++ == 10000)
		exit(0);
#endif
}

void hack_reshape (xstuff_t * XStuff)
{
	aspectRatio = (GLfloat) XStuff->windowWidth / (GLfloat) XStuff->windowHeight;

	glViewport (0, 0, (GLint) XStuff->windowWidth, (GLint) XStuff->windowHeight);

	/*
	 * setup regular drawing area just in case feedback isnt used
	 */
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	gluPerspective (20.0, aspectRatio, 0.01, 20);
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
	glTranslatef (0.0, 0.0, -5.0);

	glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
	glClear (GL_COLOR_BUFFER_BIT);
	glEnable (GL_BLEND);
	glBlendFunc (GL_ONE, GL_ONE);
	glLineWidth (2.0f);

	/*
	 * Commented out because smooth lines and textures dont mix on my TNT.
	 * Its like it rendering in software mode
	 */

	/*
	 * glEnable(GL_LINE_SMOOTH); glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	 */
}

void hack_init (xstuff_t * XStuff)
{
	int i;

	hack_reshape (XStuff);

	if (dTexture) {
		int whichtex = dTexture;
		unsigned char *l_tex = NULL;

		if (whichtex == 4)	/* random texture */
			whichtex = rsRandi (3) + 1;
		glEnable (GL_TEXTURE_2D);
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		/*
		 * Initialize texture
		 */
		glGenTextures (1, &tex);
		glBindTexture (GL_TEXTURE_2D, tex);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		switch (whichtex) {
		case 1:
			LOAD_TEXTURE (l_tex, plasmamap, plasmamap_compressedsize, plasmamap_size)
				break;
		case 2:
			LOAD_TEXTURE (l_tex, stringymap, stringymap_compressedsize, stringymap_size)
				break;
		case 3:
			LOAD_TEXTURE (l_tex, linesmap, linesmap_compressedsize, linesmap_size)
		}

		gluBuild2DMipmaps (GL_TEXTURE_2D, 1, TEXSIZE, TEXSIZE, GL_LUMINANCE, GL_UNSIGNED_BYTE, l_tex);
		FREE_TEXTURE (l_tex)
	}

	if (dFeedback) {
		feedbacktexsize = 1 << dFeedbacksize;
		while (feedbacktexsize > XStuff->windowWidth || feedbacktexsize > XStuff->windowHeight) {
			dFeedbacksize -= 1;
			feedbacktexsize = 1 << dFeedbacksize;
		}

		/*
		 * feedback texture setup
		 */
		glEnable (GL_TEXTURE_2D);
		glGenTextures (1, &feedbacktex);
		glBindTexture (GL_TEXTURE_2D, feedbacktex);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexImage2D (GL_TEXTURE_2D, 0, 3, feedbacktexsize, feedbacktexsize, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

		/*
		 * feedback velocity variable setup
		 */
		fv[0] = (float)dFeedbackspeed * (rsRandf (0.025f) + 0.025f);
		fv[1] = (float)dFeedbackspeed * (rsRandf (0.05f) + 0.05f);
		fv[2] = (float)dFeedbackspeed * (rsRandf (0.05f) + 0.05f);
		fv[3] = (float)dFeedbackspeed * (rsRandf (0.1f) + 0.1f);
		lv[0] = (float)dFeedbackspeed * (rsRandf (0.0025f) + 0.0025f);
		lv[1] = (float)dFeedbackspeed * (rsRandf (0.0025f) + 0.0025f);
		lv[2] = (float)dFeedbackspeed * (rsRandf (0.0025f) + 0.0025f);
	}

	/*
	 * Initialize wisps
	 */
	if (dWisps > 0) {
		wisps = (wisp *) malloc (sizeof (wisp) * dWisps);
		for (i = 0; i < dWisps; i++) {
			wisp_new (&wisps[i]);
		}
	}

	if (dBgWisps > 0) {
		backwisps = (wisp *) malloc (sizeof (wisp) * dBgWisps);
		for (i = 0; i < dBgWisps; i++) {
			wisp_new (&backwisps[i]);
		}
	}
}

void hack_cleanup (xstuff_t * XStuff)
{
}

void hack_handle_opts (int argc, char **argv)
{
	int change_flag = 0;

	setDefaults (PRESET_REGULAR);

	while (1) {
		int c;

#ifdef HAVE_GETOPT_H
		static struct option long_options[] = {
			{"help", 0, 0, 'h'},
			DRIVER_OPTIONS_LONG
			{"preset", 1, 0, 'p'},
			{"regular", 0, 0, 10},
			{"grid", 0, 0, 11},
			{"cubism", 0, 0, 12},
			{"badmath", 0, 0, 13},
			{"mtheory", 0, 0, 14},
			{"uhftem", 0, 0, 15},
			{"nowhere", 0, 0, 16},
			{"echo", 0, 0, 17},
			{"kaleidoscope", 0, 0, 18},
			{"wisps", 1, 0, 'i'},
			{"bgwisps", 1, 0, 'b'},
			{"density", 1, 0, 'd'},
			{"visibility", 1, 0, 'v'},
			{"speed", 1, 0, 's'},
			{"feedback", 1, 0, 'f'},
			{"feedbackspeed", 1, 0, 'e'},
			{"feedbacksize", 1, 0, 'c'},
			{"wireframe", 0, 0, 'w'},
			{"no-wireframe", 0, 0, 'W'},
			{"texture", 1, 0, 't'},
			{"plasma", 0, 0, 1},
			{"stringy", 0, 0, 2},
			{"linear", 0, 0, 3},
			{0, 0, 0, 0}
		};

		c = getopt_long (argc, argv, DRIVER_OPTIONS_SHORT "hp:i:b:d:v:s:f:e:c:wWt:", long_options, NULL);
#else
		c = getopt (argc, argv, DRIVER_OPTIONS_SHORT "hp:i:b:d:v:s:f:e:c:wWt:");
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
				"\t--preset/-p <arg>\n" "\t--regular\n" "\t--grid\n" "\t--cubism\n" "\t--badmath\n" "\t--mtheory\n" "\t--uhftem\n" "\t--nowhere\n" "\t--echo\n" "\t--kaleidoscope\n"
				"\t--wisps/-i <arg>\n" "\t--bgwisps/-b <arg>\n"
				"\t--density/-d <arg>\n" "\t--visibility/-v <arg>\n" "\t--speed/-s <arg>\n"
				"\t--feedback/-f <arg>\n" "\t--feedbackspeed/-e <arg>\n" "\t--feedbacksize/-c <arg>\n"
				"\t--wireframe/-w\n" "\t--no-wireframe/-W\n"
				"\t--texture/-t <arg>\n" "\t--plasma\n" "\t--stringy\n" "\t--linear\n", argv[0]);
			exit (1);
		case 'p':
			change_flag = 1;
			setDefaults (strtol_minmaxdef (optarg, 10, 1, 9, 0, 1, "--preset: "));
			break;
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
		case 17:
		case 18:
			change_flag = 1;
			setDefaults (c - 9);
			break;
		case 'i':
			change_flag = 1;
			dWisps = strtol_minmaxdef (optarg, 10, 0, 128, 1, 1, "--wisps: ");
			break;
		case 'b':
			change_flag = 1;
			dBgWisps = strtol_minmaxdef (optarg, 10, 0, 128, 1, 1, "--bgwisps: ");
			break;
		case 'd':
			change_flag = 1;
			dDensity = strtol_minmaxdef (optarg, 10, 2, 1000, 1, 1, "--density: ");
			break;
		case 'v':
			change_flag = 1;
			dVisibility = strtol_minmaxdef (optarg, 10, 1, 100, 1, 1, "--visibility: ");
			break;
		case 's':
			change_flag = 1;
			dSpeed = strtol_minmaxdef (optarg, 10, 1, 100, 1, 1, "--speed: ");
			break;
		case 'f':
			change_flag = 1;
			dFeedback = strtol_minmaxdef (optarg, 10, 0, 100, 1, 1, "--feedback: ");
			break;
		case 'e':
			change_flag = 1;
			dFeedbackspeed = strtol_minmaxdef (optarg, 10, 1, 10, 1, 1, "--feedbackspeed: ");
			break;
		case 'c':
			change_flag = 1;
			dFeedbacksize = 1 << strtol_minmaxdef (optarg, 10, 1, 10, 1, 1, "--feedbacksize: ");
			break;
		case 'w':
			change_flag = 1;
			dWireframe = 1;
			break;
		case 'W':
			change_flag = 1;
			dWireframe = 0;
			break;
		case 't':
			change_flag = 1;
			dTexture = strtol_minmaxdef (optarg, 10, 0, 4, 0, 1, "--texture: ");
			break;
		case 1:
		case 2:
		case 3:
			change_flag = 1;
			dTexture = c;
			break;
		}
	}

	if (!change_flag) {
		setDefaults (rsRandi (9) + 1);
	}
}
