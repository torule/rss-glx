/*
 * Copyright (C) 2002  Terence M. Welsh
 * Ported to Linux by Tugrul Galatali <tugrul@galatali.com>
 *
 * Skyrocket is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 *
 * Skyrocket is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <GL/gl.h>
#include <GL/glu.h>

#include "skyrocket_flare.h"

#define FLARESIZE 128

#define max(x, y) ((x > y) ? x : y)

unsigned int flarelist[4];

extern int xsize, ysize;
extern float aspectRatio;

// Generate textures for lens flares
// then applies textures to geometry in display lists
void initFlares ()
{
	int i, j;
	float x, y;
	float temp;
	unsigned char flare[FLARESIZE][FLARESIZE][4];
	unsigned int flaretex[4];

	glGenTextures (4, flaretex);

	// First flare:  basic sphere
	for (i = 0; i < FLARESIZE; i++) {
		for (j = 0; j < FLARESIZE; j++) {
			flare[i][j][0] = 255;
			flare[i][j][1] = 255;
			flare[i][j][2] = 255;
			x = float (i - FLARESIZE / 2) / float (FLARESIZE / 2);
			y = float (j - FLARESIZE / 2) / float (FLARESIZE / 2);

			temp = 1.0f - ((x * x) + (y * y));
			if (temp > 1.0f)
				temp = 1.0f;
			if (temp < 0.0f)
				temp = 0.0f;
			flare[i][j][3] = char (255.0f * temp * temp);
		}
	}
	glBindTexture (GL_TEXTURE_2D, flaretex[0]);
	//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D (GL_TEXTURE_2D, 0, 4, FLARESIZE, FLARESIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, flare);

	// Second flare:  flattened sphere
	for (i = 0; i < FLARESIZE; i++) {
		for (j = 0; j < FLARESIZE; j++) {
			flare[i][j][0] = 255;
			flare[i][j][1] = 255;
			flare[i][j][2] = 255;
			x = float (i - FLARESIZE / 2) / float (FLARESIZE / 2);
			y = float (j - FLARESIZE / 2) / float (FLARESIZE / 2);

			temp = 2.5f * (1.0f - ((x * x) + (y * y)));
			if (temp > 1.0f)
				temp = 1.0f;
			if (temp < 0.0f)
				temp = 0.0f;
			//temp = temp * temp * temp * temp;
			flare[i][j][3] = char (255.0f * temp);
		}
	}
	glBindTexture (GL_TEXTURE_2D, flaretex[1]);
	//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D (GL_TEXTURE_2D, 0, 4, FLARESIZE, FLARESIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, flare);

	// Third flare:  torus
	for (i = 0; i < FLARESIZE; i++) {
		for (j = 0; j < FLARESIZE; j++) {
			flare[i][j][0] = 255;
			flare[i][j][1] = 255;
			flare[i][j][2] = 255;
			x = float (i - FLARESIZE / 2) / float (FLARESIZE / 2);
			y = float (j - FLARESIZE / 2) / float (FLARESIZE / 2);

			temp = 4.0f * ((x * x) + (y * y)) * (1.0f - ((x * x) + (y * y)));
			if (temp > 1.0f)
				temp = 1.0f;
			if (temp < 0.0f)
				temp = 0.0f;
			temp = temp * temp * temp * temp;
			flare[i][j][3] = char (255.0f * temp);
		}
	}
	glBindTexture (GL_TEXTURE_2D, flaretex[2]);
	//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D (GL_TEXTURE_2D, 0, 4, FLARESIZE, FLARESIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, flare);

	// Fourth flare:  kick-ass flare
	for (i = 0; i < FLARESIZE; i++) {
		for (j = 0; j < FLARESIZE; j++) {
			x = float (i - FLARESIZE / 2) / float (FLARESIZE / 2);

			if (x < 0.0f)
				x = -x;
			y = float (j - FLARESIZE / 2) / float (FLARESIZE / 2);

			if (y < 0.0f)
				y = -y;
			flare[i][j][0] = 255;
			flare[i][j][1] = 255;
			temp = 0.14f * (1.0f - max (x, y)) / max ((x * y), 0.05f);
			if (temp > 1.0f)
				temp = 1.0f;
			if (temp < 0.0f)
				temp = 0.0f;
			flare[i][j][2] = char (255.0f * temp);

			temp = 0.1f * (1.0f - max (x, y)) / max ((x * y), 0.1f);
			if (temp > 1.0f)
				temp = 1.0f;
			if (temp < 0.0f)
				temp = 0.0f;
			flare[i][j][3] = char (255.0f * temp);
		}
	}
	glBindTexture (GL_TEXTURE_2D, flaretex[3]);
	//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D (GL_TEXTURE_2D, 0, 4, FLARESIZE, FLARESIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, flare);

	// Build display lists
	flarelist[0] = glGenLists (4);
	flarelist[1] = flarelist[0] + 1;
	flarelist[2] = flarelist[0] + 2;
	flarelist[3] = flarelist[0] + 3;
	for (i = 0; i < 4; i++) {
		glNewList (flarelist[i], GL_COMPILE);
		glBindTexture (GL_TEXTURE_2D, flaretex[i]);
		glBegin (GL_TRIANGLE_STRIP);
		glTexCoord2f (0.0f, 0.0f);
		glVertex3f (-0.5f, -0.5f, 0.0f);
		glTexCoord2f (1.0f, 0.0f);
		glVertex3f (0.5f, -0.5f, 0.0f);
		glTexCoord2f (0.0f, 1.0f);
		glVertex3f (-0.5f, 0.5f, 0.0f);
		glTexCoord2f (1.0f, 1.0f);
		glVertex3f (0.5f, 0.5f, 0.0f);
		glEnd ();
		glEndList ();
	}
}

void cleanupFlares ()
{
	glDeleteLists (flarelist[0], 4);
}

// Draw a flare at a specified (x,y) location on the screen
// Screen corners are at (0,0) and (1,1)
// alpha = 0.0 for lowest intensity; alpha = 1.0 for highest intensity
void flare (float x, float y, float red, float green, float blue, float alpha)
{
	float dx, dy;
	float fadewidth, temp;

	glBlendFunc (GL_SRC_ALPHA, GL_ONE);
	glEnable (GL_BLEND);

	// Fade alpha if source is off edge of screen
	fadewidth = float (xsize) / 10.0f;

	if (y < 0) {
		temp = fadewidth + y;
		if (temp < 0.0f)
			return;
		alpha *= temp / fadewidth;
	}
	if (y > ysize) {
		temp = fadewidth - y + ysize;
		if (temp < 0.0f)
			return;
		alpha *= temp / fadewidth;
	}
	if (x < 0) {
		temp = fadewidth + x;
		if (temp < 0.0f)
			return;
		alpha *= temp / fadewidth;
	}
	if (x > xsize) {
		temp = fadewidth - x + xsize;
		if (temp < 0.0f)
			return;
		alpha *= temp / fadewidth;
	}
	// Find lens flare vector
	// This vector runs from the light source through the screen's center
	dx = 0.5f * aspectRatio - x;
	dy = 0.5f - y;

	// Setup projection matrix
	glMatrixMode (GL_PROJECTION);
	glPushMatrix ();
	glLoadIdentity ();
	gluOrtho2D (0, aspectRatio, 0, 1.0f);

	// Draw stuff
	glMatrixMode (GL_MODELVIEW);
	glPushMatrix ();

	glLoadIdentity ();
	glTranslatef (x + dx * 0.05f, y + dy * 0.05f, 0.0f);
	glScalef (0.065f, 0.065f, 0.065f);
	glColor4f (red, green, blue, alpha * 0.4f);
	glCallList (flarelist[2]);

	glLoadIdentity ();
	glTranslatef (x + dx * 0.15f, y + dy * 0.15f, 0.0f);
	glScalef (0.04f, 0.04f, 0.04f);
	glColor4f (red * 0.9f, green * 0.9f, blue, alpha * 0.9f);
	glCallList (flarelist[1]);

	glLoadIdentity ();
	glTranslatef (x + dx * 0.25f, y + dy * 0.25f, 0.0f);
	glScalef (0.06f, 0.06f, 0.06f);
	glColor4f (red * 0.8f, green * 0.8f, blue, alpha * 0.9f);
	glCallList (flarelist[1]);

	glLoadIdentity ();
	glTranslatef (x + dx * 0.35f, y + dy * 0.35f, 0.0f);
	glScalef (0.08f, 0.08f, 0.08f);
	glColor4f (red * 0.7f, green * 0.7f, blue, alpha * 0.9f);
	glCallList (flarelist[1]);

	glLoadIdentity ();
	glTranslatef (x + dx * 1.25f, y + dy * 1.25f, 0.0f);
	glScalef (0.05f, 0.05f, 0.05f);
	glColor4f (red, green * 0.6f, blue * 0.6f, alpha * 0.9f);
	glCallList (flarelist[1]);

	glLoadIdentity ();
	glTranslatef (x + dx * 1.65f, y + dy * 1.65f, 0.0f);
	glRotatef (x, 0, 0, 1);
	glScalef (0.3f, 0.3f, 0.3f);
	glColor4f (red, green, blue, alpha);
	glCallList (flarelist[3]);

	glLoadIdentity ();
	glTranslatef (x + dx * 1.85f, y + dy * 1.85f, 0.0f);
	glScalef (0.04f, 0.04f, 0.04f);
	glColor4f (red, green * 0.6f, blue * 0.6f, alpha * 0.9f);
	glCallList (flarelist[1]);

	glLoadIdentity ();
	glTranslatef (x + dx * 2.2f, y + dy * 2.2f, 0.0f);
	glScalef (0.3f, 0.3f, 0.3f);
	glColor4f (red, green, blue, alpha * 0.7f);
	glCallList (flarelist[1]);

	glLoadIdentity ();
	glTranslatef (x + dx * 2.5f, y + dy * 2.5f, 0.0f);
	glScalef (0.6f, 0.6f, 0.6f);
	glColor4f (red, green, blue, alpha * 0.8f);
	glCallList (flarelist[3]);

	glPopMatrix ();

	// Unsetup projection matrix
	glMatrixMode (GL_PROJECTION);
	glPopMatrix ();
	glMatrixMode (GL_MODELVIEW);
}

// super bright elongated glow for sucker, shockwave, stretcher, and bigmama
void superFlare (float x, float y, float red, float green, float blue, float alpha)
{
	float fadewidth, temp;

	glBlendFunc (GL_SRC_ALPHA, GL_ONE);
	glEnable (GL_BLEND);

	// Fade alpha if source is off edge of screen
	fadewidth = float (xsize) / 10.0f;

	if (y < 0) {
		temp = fadewidth + y;
		if (temp < 0.0f)
			return;
		alpha *= temp / fadewidth;
	}
	if (y > ysize) {
		temp = fadewidth - y + ysize;
		if (temp < 0.0f)
			return;
		alpha *= temp / fadewidth;
	}
	if (x < 0) {
		temp = fadewidth + x;
		if (temp < 0.0f)
			return;
		alpha *= temp / fadewidth;
	}
	if (x > xsize) {
		temp = fadewidth - x + xsize;
		if (temp < 0.0f)
			return;
		alpha *= temp / fadewidth;
	}
	// Setup projection matrix
	glMatrixMode (GL_PROJECTION);
	glPushMatrix ();
	glLoadIdentity ();
	gluOrtho2D (0, aspectRatio, 0, 1.0f);

	// Draw stuff
	glMatrixMode (GL_MODELVIEW);
	glPushMatrix ();

	glLoadIdentity ();
	glTranslatef (x, y, 0.0f);
	glScalef (2.0f * alpha, 0.08f, 0.0f);
	glColor4f (red, green, blue, alpha);
	glCallList (flarelist[0]);

	glLoadIdentity ();
	glTranslatef (x, y, 0.0f);
	glScalef (0.4f, 0.35f * alpha + 0.05f, 1.0f);
	glColor4f (red, green, blue, alpha * 0.4f);
	glCallList (flarelist[2]);

	glPopMatrix ();

	// Unsetup projection matrix
	glMatrixMode (GL_PROJECTION);
	glPopMatrix ();
	glMatrixMode (GL_MODELVIEW);
}
