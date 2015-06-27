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

#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "cloudtex.h"
#include "earthtex.h"
#include "moontex.h"

#include "loadTexture.h"
#include "rsDefines.h"
#include "rsRand.h"
#include "skyrocket_world.h"

extern int dWind;
extern int dAmbient;
extern int dStardensity;
extern int dMoonglow;
extern int dMoon;
extern int dClouds;
extern int dEarth;
int doSunset;

extern float elapsedTime;

extern unsigned int flarelist[4];

float clouds[CLOUDMESH + 1][CLOUDMESH + 1][9];	// 9 = x,y,z,u,v,std bright,r,g,b

unsigned int cloudtex;
unsigned int earthneartex;
unsigned int earthfartex;
unsigned int earthlighttex;

unsigned int starlist;
unsigned int moonlist;
unsigned int moonglowlist;
unsigned int sunsetlist;
unsigned int earthlist;
unsigned int earthnearlist;
unsigned int earthfarlist;

// For building mountain sillohettes in sunset
void makeHeights (int first, int last, int *h)
{
	int middle;
	int diff;

	diff = last - first;
	if (diff <= 1)
		return;
	middle = (first + last) / 2;
	h[middle] = (h[first] + h[last]) / 2;
	h[middle] += rsRandi (diff / 2) - (diff / 4);
	if (h[middle] < 1)
		h[middle] = 1;

	makeHeights (first, middle, h);
	makeHeights (middle, last, h);
}

void initWorld2 ()
{
	int i;

	i = 0;
}

void initWorld ()
{
	int i, j;
	float x, y, z;
	unsigned int startex;
	unsigned int moontex;
	unsigned int moonglowtex;
	unsigned int sunsettex;
	unsigned char *tex;

	// Initialize cloud texture object even if clouds are not turned on.
	// Sunsets and shockwaves can also use cloud texture.
	glGenTextures (1, &cloudtex);
	glBindTexture (GL_TEXTURE_2D, cloudtex);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	LOAD_TEXTURE (tex, cloudmap, cloudmap_compressedsize, cloudmap_size)
		gluBuild2DMipmaps (GL_TEXTURE_2D, 2, CLOUDTEXSIZE, CLOUDTEXSIZE, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, tex);
	FREE_TEXTURE (tex)
		// initialize star texture
		if (dStardensity) {
		unsigned char starmap[STARTEXSIZE][STARTEXSIZE][3];

		for (i = 0; i < STARTEXSIZE; i++) {
			for (j = 0; j < STARTEXSIZE; j++) {
				starmap[i][j][0] = starmap[i][j][1] = starmap[i][j][2] = 0;
			}
		}
		int u, v;
		unsigned int rgb[3];

		for (i = 0; i < (dStardensity * 20); i++) {
			u = rsRandi (STARTEXSIZE - 4) + 2;
			v = rsRandi (STARTEXSIZE - 4) + 2;
			rgb[0] = 220 + rsRandi (36);
			rgb[1] = 220 + rsRandi (36);
			rgb[2] = 220 + rsRandi (36);
			rgb[rsRandi (3)] = 255;
			starmap[u][v][0] = rgb[0];
			starmap[u][v][1] = rgb[1];
			starmap[u][v][2] = rgb[2];
			switch (rsRandi (6)) {	// different stars
			case 0:	// small
			case 1:
			case 2:
				starmap[u][v][0] /= 2;
				starmap[u][v][1] /= 2;
				starmap[u][v][2] /= 2;
				starmap[u + 1][v][0] = starmap[u - 1][v][0] = starmap[u][v + 1][0] = starmap[u][v - 1][0] = rgb[0] / (3 + rsRandi (6));
				starmap[u + 1][v][1] = starmap[u - 1][v][1] = starmap[u][v + 1][1] = starmap[u][v - 1][1] = rgb[1] / (3 + rsRandi (6));
				starmap[u + 1][v][2] = starmap[u - 1][v][2] = starmap[u][v + 1][2] = starmap[u][v - 1][2] = rgb[2] / (3 + rsRandi (6));
				break;
			case 3:	// medium
			case 4:
				starmap[u + 1][v][0] = starmap[u - 1][v][0] = starmap[u][v + 1][0] = starmap[u][v - 1][0] = rgb[0] / 2;
				starmap[u + 1][v][1] = starmap[u - 1][v][1] = starmap[u][v + 1][1] = starmap[u][v - 1][1] = rgb[1] / 2;
				starmap[u + 1][v][2] = starmap[u - 1][v][2] = starmap[u][v + 1][2] = starmap[u][v - 1][2] = rgb[2] / 2;
				break;
			case 5:	// large
				starmap[u + 1][v][0] = starmap[u - 1][v][0] = starmap[u][v + 1][0] = starmap[u][v - 1][0] = char (float (rgb[0]) * 0.75f);
				starmap[u + 1][v][1] = starmap[u - 1][v][1] = starmap[u][v + 1][1] = starmap[u][v - 1][1] = char (float (rgb[1]) * 0.75f);
				starmap[u + 1][v][2] = starmap[u - 1][v][2] = starmap[u][v + 1][2] = starmap[u][v - 1][2] = char (float (rgb[2]) * 0.75f);

				starmap[u + 1][v + 1][0] = starmap[u + 1][v - 1][0] = starmap[u - 1][v + 1][0] = starmap[u - 1][v - 1][0] = rgb[0] / 4;
				starmap[u + 1][v + 1][1] = starmap[u + 1][v - 1][1] = starmap[u - 1][v + 1][1] = starmap[u - 1][v - 1][1] = rgb[1] / 4;
				starmap[u + 1][v + 1][2] = starmap[u + 1][v - 1][2] = starmap[u - 1][v + 1][2] = starmap[u - 1][v - 1][2] = rgb[2] / 4;
			}
		}
		glGenTextures (1, &startex);
		glBindTexture (GL_TEXTURE_2D, startex);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		gluBuild2DMipmaps (GL_TEXTURE_2D, 3, STARTEXSIZE, STARTEXSIZE, GL_RGB, GL_UNSIGNED_BYTE, starmap);
	}
	//initialize moon texture
	if (dMoon) {
		unsigned char moonmap[MOONTEXSIZE][MOONTEXSIZE][4];
		unsigned char *mtint;
		unsigned char *malpha;

		LOAD_TEXTURE (mtint, moontint, moontint_compressedsize, moontint_size)
			LOAD_TEXTURE (malpha, moonalpha, moonalpha_compressedsize, moonalpha_size)

			for (i = 0; i < MOONTEXSIZE; i++) {
			for (j = 0; j < MOONTEXSIZE; j++) {
				moonmap[i][j][0] = moonmap[i][j][1] = moonmap[i][j][2] = mtint[i * MOONTEXSIZE + j];
				moonmap[i][j][3] = malpha[i * MOONTEXSIZE + j];
			}
		}

		FREE_TEXTURE (mtint)
			FREE_TEXTURE (malpha)

			glGenTextures (1, &moontex);
		glBindTexture (GL_TEXTURE_2D, moontex);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		gluBuild2DMipmaps (GL_TEXTURE_2D, 4, MOONTEXSIZE, MOONTEXSIZE, GL_RGBA, GL_UNSIGNED_BYTE, moonmap);
	}
	//initialize moon glow texture
	if (dMoonglow) {
		unsigned char moonglowmap[MOONGLOWTEXSIZE][MOONGLOWTEXSIZE][4];

		float temp1, temp2, temp3, u, v;

		for (i = 0; i < MOONGLOWTEXSIZE; i++) {
			for (j = 0; j < MOONGLOWTEXSIZE; j++) {
				u = float (i - MOONGLOWTEXSIZE / 2) / float (MOONGLOWTEXSIZE / 2);
				v = float (j - MOONGLOWTEXSIZE / 2) / float (MOONGLOWTEXSIZE / 2);

				temp1 = 4.0f * ((u * u) + (v * v)) * (1.0f - ((u * u) + (v * v)));
				if (temp1 > 1.0f)
					temp1 = 1.0f;
				if (temp1 < 0.0f)
					temp1 = 0.0f;
				temp1 = temp1 * temp1 * temp1 * temp1;
				u *= 1.2f;
				v *= 1.2f;
				temp2 = 4.0f * ((u * u) + (v * v)) * (1.0f - ((u * u) + (v * v)));
				if (temp2 > 1.0f)
					temp2 = 1.0f;
				if (temp2 < 0.0f)
					temp2 = 0.0f;
				temp2 = temp2 * temp2 * temp2 * temp2;
				u *= 1.25f;
				v *= 1.25f;
				temp3 = 4.0f * ((u * u) + (v * v)) * (1.0f - ((u * u) + (v * v)));
				if (temp3 > 1.0f)
					temp3 = 1.0f;
				if (temp3 < 0.0f)
					temp3 = 0.0f;
				temp3 = temp3 * temp3 * temp3 * temp3;
				moonglowmap[i][j][0] = char (255.0f * (temp1 * 0.4f + temp2 * 0.4f + temp3 * 0.48f));
				moonglowmap[i][j][1] = char (255.0f * (temp1 * 0.4f + temp2 * 0.48f + temp3 * 0.38f));
				moonglowmap[i][j][2] = char (255.0f * (temp1 * 0.48f + temp2 * 0.4f + temp3 * 0.38f));
				moonglowmap[i][j][3] = char (255.0f * (temp1 * 0.48f + temp2 * 0.48f + temp3 * 0.48f));
			}
		}
		glGenTextures (1, &moonglowtex);
		glBindTexture (GL_TEXTURE_2D, moonglowtex);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		gluBuild2DMipmaps (GL_TEXTURE_2D, 4, MOONGLOWTEXSIZE, MOONGLOWTEXSIZE, GL_RGBA, GL_UNSIGNED_BYTE, moonglowmap);

	}
	// do a sunset?
	doSunset = 1;
	if (!rsRandi (4))
		doSunset = 0;

	// initialize sunset texture
	if (doSunset) {
		unsigned char *sunsetmap;
		unsigned char rgb[3];
		float temp;

		sunsetmap = (unsigned char *)malloc (CLOUDTEXSIZE * CLOUDTEXSIZE * 3);

		if (rsRandi (3))
			rgb[0] = 60 + rsRandi (42);
		else
			rgb[0] = rsRandi (102);
		rgb[1] = rsRandi (rgb[0]);
		rgb[2] = 0;
		if (rgb[1] < 50)
			rgb[2] = 100 - rsRandi (rgb[0]);
		for (j = 0; j < CLOUDTEXSIZE; j++) {
			for (i = 0; i < CLOUDTEXSIZE; i++) {
				sunsetmap[i * CLOUDTEXSIZE * 3 + j * 3 + 0] = rgb[0];
				sunsetmap[i * CLOUDTEXSIZE * 3 + j * 3 + 1] = rgb[1];
				sunsetmap[i * CLOUDTEXSIZE * 3 + j * 3 + 2] = rgb[2];
			}
		}
		// clouds in sunset
		if (rsRandi (3)) {
			float cloudinf;	// influence of clouds
			int xoffset = rsRandi (CLOUDTEXSIZE);
			int yoffset = rsRandi (CLOUDTEXSIZE);
			int x, y;

			for (i = 0; i < CLOUDTEXSIZE; i++) {
				for (j = 0; j < CLOUDTEXSIZE; j++) {
					x = (i + xoffset) % CLOUDTEXSIZE;
					y = (j + yoffset) % CLOUDTEXSIZE;
					cloudinf = float (cloudmap[x * CLOUDTEXSIZE * 2 + y * 2 + 1]) / 256.0f;
					temp = float (sunsetmap[i * CLOUDTEXSIZE * 3 + j * 3 + 0]) / 256.0f;

					temp *= cloudinf;
					sunsetmap[i * CLOUDTEXSIZE * 3 + j * 3 + 0] = char (temp * 256.0f);
					cloudinf *= float (cloudmap[x * CLOUDTEXSIZE * 2 + y * 2]) / 256.0f;
					temp = float (sunsetmap[i * CLOUDTEXSIZE * 3 + j * 3 + 1]) / 256.0f;

					temp *= cloudinf;
					sunsetmap[i * CLOUDTEXSIZE * 3 + j * 3 + 1] = char (temp * 256.0f);
				}
			}
		}
		// Fractal mountain generation
		int mountains[CLOUDTEXSIZE + 1];

		mountains[0] = mountains[CLOUDTEXSIZE] = rsRandi (10) + 5;
		makeHeights (0, CLOUDTEXSIZE, mountains);
		for (i = 0; i < CLOUDTEXSIZE; i++) {
			for (j = 0; j <= mountains[i]; j++) {
				sunsetmap[i * CLOUDTEXSIZE * 3 + j * 3 + 0] = 0;
				sunsetmap[i * CLOUDTEXSIZE * 3 + j * 3 + 1] = 0;
				sunsetmap[i * CLOUDTEXSIZE * 3 + j * 3 + 2] = 0;
			}
			sunsetmap[i * CLOUDTEXSIZE * 3 + (mountains[i] + 1) * 3 + 0] /= 4;
			sunsetmap[i * CLOUDTEXSIZE * 3 + (mountains[i] + 1) * 3 + 1] /= 4;
			sunsetmap[i * CLOUDTEXSIZE * 3 + (mountains[i] + 1) * 3 + 2] /= 4;
			sunsetmap[i * CLOUDTEXSIZE * 3 + (mountains[i] + 2) * 3 + 0] /= 2;
			sunsetmap[i * CLOUDTEXSIZE * 3 + (mountains[i] + 2) * 3 + 1] /= 2;
			sunsetmap[i * CLOUDTEXSIZE * 3 + (mountains[i] + 2) * 3 + 2] /= 2;
			sunsetmap[i * CLOUDTEXSIZE * 3 + (mountains[i] + 3) * 3 + 0] = char (float (sunsetmap[i * CLOUDTEXSIZE * 3 + (mountains[i] + 3) * 3 + 0]) * 0.75f);
			sunsetmap[i * CLOUDTEXSIZE * 3 + (mountains[i] + 3) * 3 + 1] = char (float (sunsetmap[i * CLOUDTEXSIZE * 3 + (mountains[i] + 3) * 3 + 1]) * 0.75f);
			sunsetmap[i * CLOUDTEXSIZE * 3 + (mountains[i] + 3) * 3 + 2] = char (float (sunsetmap[i * CLOUDTEXSIZE * 3 + (mountains[i] + 3) * 3 + 2]) * 0.75f);
		}
		// build texture object
		glGenTextures (1, &sunsettex);
		glBindTexture (GL_TEXTURE_2D, sunsettex);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		gluBuild2DMipmaps (GL_TEXTURE_2D, 3, CLOUDTEXSIZE, CLOUDTEXSIZE, GL_RGB, GL_UNSIGNED_BYTE, sunsetmap);

		free (sunsetmap);
	}
	//initialize earth texture
	if (dEarth) {
		glGenTextures (1, &earthneartex);
		glBindTexture (GL_TEXTURE_2D, earthneartex);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		LOAD_TEXTURE (tex, earthnearmap, earthnearmap_compressedsize, earthnearmap_size)
			gluBuild2DMipmaps (GL_TEXTURE_2D, 3, EARTHNEARSIZE, EARTHNEARSIZE, GL_RGB, GL_UNSIGNED_BYTE, tex);
		FREE_TEXTURE (tex)
			glGenTextures (1, &earthfartex);
		glBindTexture (GL_TEXTURE_2D, earthfartex);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		LOAD_TEXTURE (tex, earthfarmap, earthfarmap_compressedsize, earthfarmap_size)
			gluBuild2DMipmaps (GL_TEXTURE_2D, 3, EARTHFARSIZE, EARTHFARSIZE, GL_RGB, GL_UNSIGNED_BYTE, tex);
		FREE_TEXTURE (tex)
			glGenTextures (1, &earthlighttex);
		glBindTexture (GL_TEXTURE_2D, earthlighttex);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		LOAD_TEXTURE (tex, earthlightmap, earthlightmap_compressedsize, earthlightmap_size)
			gluBuild2DMipmaps (GL_TEXTURE_2D, 3, EARTHFARSIZE, EARTHFARSIZE, GL_RGB, GL_UNSIGNED_BYTE, tex);
		FREE_TEXTURE (tex)
	}
	// initialize star geometry
	if (dStardensity) {
		float stars[STARMESH + 1][STARMESH / 2][6];	// 6 = x,y,z,u,v,bright

		for (j = 0; j < STARMESH / 2; j++) {
			y = sin (RS_PIo2 * float (j) / float (STARMESH / 2));

			for (i = 0; i <= STARMESH; i++) {
				x = cos (RS_PIx2 * float (i) / float (STARMESH)) * cos (RS_PIo2 * float (j) / float (STARMESH / 2));
				z = sin (RS_PIx2 * float (i) / float (STARMESH)) * cos (RS_PIo2 * float (j) / float (STARMESH / 2));

				// positions
				stars[i][j][0] = x * 20000.0f;
				stars[i][j][1] = 1500.0f + 18500.0f * y;
				stars[i][j][2] = z * 20000.0f;
				// tex coords
				stars[i][j][3] = 1.2f * x * (2.5f - y);
				stars[i][j][4] = 1.2f * z * (2.5f - y);
				// brightness
				if (stars[i][j][1] < 1501.0f)
					stars[i][j][5] = 0.0f;
				else
					stars[i][j][5] = 1.0f;
			}
		}
		starlist = glGenLists (1);
		glNewList (starlist, GL_COMPILE);
		glBindTexture (GL_TEXTURE_2D, startex);
		for (j = 0; j < (STARMESH / 2 - 1); j++) {
			glBegin (GL_TRIANGLE_STRIP);
			for (i = 0; i <= STARMESH; i++) {
				glColor3f (stars[i][j + 1][5], stars[i][j + 1][5], stars[i][j + 1][5]);
				glTexCoord2f (stars[i][j + 1][3], stars[i][j + 1][4]);
				glVertex3fv (stars[i][j + 1]);
				glColor3f (stars[i][j][5], stars[i][j][5], stars[i][j][5]);
				glTexCoord2f (stars[i][j][3], stars[i][j][4]);
				glVertex3fv (stars[i][j]);
			}
			glEnd ();
		}
		j = STARMESH / 2 - 1;
		glBegin (GL_TRIANGLE_FAN);
		glColor3f (1.0f, 1.0f, 1.0f);
		glTexCoord2f (0.0f, 0.0f);
		glVertex3f (0.0f, 20000.0f, 0.0f);
		for (i = 0; i <= STARMESH; i++) {
			glColor3f (stars[i][j][5], stars[i][j][5], stars[i][j][5]);
			glTexCoord2f (stars[i][j][3], stars[i][j][4]);
			glVertex3fv (stars[i][j]);
		}
		glEnd ();
		glEndList ();
	}
	// initialize moon geometry
	if (dMoon) {
		moonlist = glGenLists (1);
		glNewList (moonlist, GL_COMPILE);
		glColor4f (1.0f, 1.0f, 1.0f, 1.0f);
		glBindTexture (GL_TEXTURE_2D, moontex);
		glBegin (GL_TRIANGLE_STRIP);
		glTexCoord2f (0.0f, 0.0f);
		glVertex3f (-800.0f, -800.0f, 0.0f);
		glTexCoord2f (1.0f, 0.0f);
		glVertex3f (800.0f, -800.0f, 0.0f);
		glTexCoord2f (0.0f, 1.0f);
		glVertex3f (-800.0f, 800.0f, 0.0f);
		glTexCoord2f (1.0f, 1.0f);
		glVertex3f (800.0f, 800.0f, 0.0f);
		glEnd ();
		glEndList ();
	}
	// initialize moon glow geometry
	if (dMoonglow) {
		moonglowlist = glGenLists (1);
		glNewList (moonglowlist, GL_COMPILE);
		glBindTexture (GL_TEXTURE_2D, moonglowtex);
		glBegin (GL_TRIANGLE_STRIP);
		glTexCoord2f (0.0f, 0.0f);
		glVertex3f (-7000.0f, -7000.0f, 0.0f);
		glTexCoord2f (1.0f, 0.0f);
		glVertex3f (7000.0f, -7000.0f, 0.0f);
		glTexCoord2f (0.0f, 1.0f);
		glVertex3f (-7000.0f, 7000.0f, 0.0f);
		glTexCoord2f (1.0f, 1.0f);
		glVertex3f (7000.0f, 7000.0f, 0.0f);
		glEnd ();
		glEndList ();
	}
	// initialize cloud geometry
	if (dClouds) {
		for (j = 0; j <= CLOUDMESH; j++) {
			for (i = 0; i <= CLOUDMESH; i++) {
				x = float (i - (CLOUDMESH / 2));
				z = float (j - (CLOUDMESH / 2));

				clouds[i][j][0] = x * (40000.0f / float (CLOUDMESH));
				clouds[i][j][2] = z * (40000.0f / float (CLOUDMESH));
				x = float (fabs (x / float (CLOUDMESH / 2)));
				z = float (fabs (z / float (CLOUDMESH / 2)));
				clouds[i][j][1] = 2000.0f - 1000.0f * float (x * x + z * z);
				clouds[i][j][3] = float (-i) / float (CLOUDMESH / 6);	// tex coords
				clouds[i][j][4] = float (-j) / float (CLOUDMESH / 6);
				clouds[i][j][5] = (clouds[i][j][1] - 1000.0f) * 0.00001f * float (dAmbient);	// brightness

				if (clouds[i][j][5] < 0.0f)
					clouds[i][j][5] = 0.0f;
			}
		}
	}
	// initialize sunset geometry
	if (doSunset) {
		sunsetlist = glGenLists (1);
		float vert[6] = { 0.0f, 7654.0f, 8000.0f, 14142.0f, 18448.0f, 20000.0f };

		glNewList (sunsetlist, GL_COMPILE);
		glBindTexture (GL_TEXTURE_2D, sunsettex);
		glBegin (GL_TRIANGLE_STRIP);
		glColor3f (0.0f, 0.0f, 0.0f);
		glTexCoord2f (1.0f, 0.0f);
		glVertex3f (vert[0], vert[2], vert[5]);
		glColor3f (0.0f, 0.0f, 0.0f);
		glTexCoord2f (0.0f, 0.0f);
		glVertex3f (vert[0], vert[0], vert[5]);
		glColor3f (0.0f, 0.0f, 0.0f);
		glTexCoord2f (1.0f, 0.125f);
		glVertex3f (-vert[1], vert[2], vert[4]);
		glColor3f (0.25f, 0.25f, 0.25f);
		glTexCoord2f (0.0f, 0.125f);
		glVertex3f (-vert[1], vert[0], vert[4]);
		glColor3f (0.0f, 0.0f, 0.0f);
		glTexCoord2f (1.0f, 0.25f);
		glVertex3f (-vert[3], vert[2], vert[3]);
		glColor3f (0.5f, 0.5f, 0.5f);
		glTexCoord2f (0.0f, 0.25f);
		glVertex3f (-vert[3], vert[0], vert[3]);
		glColor3f (0.0f, 0.0f, 0.0f);
		glTexCoord2f (1.0f, 0.375f);
		glVertex3f (-vert[4], vert[2], vert[1]);
		glColor3f (0.75f, 0.75f, 0.75f);
		glTexCoord2f (0.0f, 0.375f);
		glVertex3f (-vert[4], vert[0], vert[1]);
		glColor3f (0.0f, 0.0f, 0.0f);
		glTexCoord2f (1.0f, 0.5f);
		glVertex3f (-vert[5], vert[2], vert[0]);
		glColor3f (1.0f, 1.0f, 1.0f);
		glTexCoord2f (0.0f, 0.5f);
		glVertex3f (-vert[5], vert[0], vert[0]);
		glColor3f (0.0f, 0.0f, 0.0f);
		glTexCoord2f (1.0f, 0.625f);
		glVertex3f (-vert[4], vert[2], -vert[1]);
		glColor3f (0.75f, 0.75f, 0.75f);
		glTexCoord2f (0.0f, 0.625f);
		glVertex3f (-vert[4], vert[0], -vert[1]);
		glColor3f (0.0f, 0.0f, 0.0f);
		glTexCoord2f (1.0f, 0.75f);
		glVertex3f (-vert[3], vert[2], -vert[3]);
		glColor3f (0.5f, 0.5f, 0.5f);
		glTexCoord2f (0.0f, 0.75f);
		glVertex3f (-vert[3], vert[0], -vert[3]);
		glColor3f (0.0f, 0.0f, 0.0f);
		glTexCoord2f (1.0f, 0.875f);
		glVertex3f (-vert[1], vert[2], -vert[4]);
		glColor3f (0.25f, 0.25f, 0.25f);
		glTexCoord2f (0.0f, 0.875f);
		glVertex3f (-vert[1], vert[0], -vert[4]);
		glColor3f (0.0f, 0.0f, 0.0f);
		glTexCoord2f (1.0f, 1.0f);
		glVertex3f (vert[0], vert[2], -vert[5]);
		glColor3f (0.0f, 0.0f, 0.0f);
		glTexCoord2f (0.0f, 1.0f);
		glVertex3f (vert[0], vert[0], -vert[5]);
		glEnd ();
		glEndList ();
	}
	// initialize earth geometry
	if (dEarth) {
		earthlist = glGenLists (1);
		earthnearlist = glGenLists (1);
		earthfarlist = glGenLists (1);
		float lit[] = { float (dAmbient) * 0.01f, float (dAmbient) * 0.01f, float (dAmbient) * 0.01f };
		float unlit[] = { 0.0f, 0.0f, 0.0f };
		float vert[2] = { 839.68f, 8396.8f };
		float tex[4] = { 0.0f, 0.45f, 0.55f, 1.0f };

		glNewList (earthnearlist, GL_COMPILE);
		glColor3fv (lit);
		glBegin (GL_TRIANGLE_STRIP);
		glTexCoord2f (tex[0], tex[0]);
		glVertex3f (-vert[0], 0.0f, -vert[0]);
		glTexCoord2f (tex[0], tex[3]);
		glVertex3f (-vert[0], 0.0f, vert[0]);
		glTexCoord2f (tex[3], tex[0]);
		glVertex3f (vert[0], 0.0f, -vert[0]);
		glTexCoord2f (tex[3], tex[3]);
		glVertex3f (vert[0], 0.0f, vert[0]);
		glEnd ();
		glEndList ();
		glNewList (earthfarlist, GL_COMPILE);
		glBegin (GL_TRIANGLE_STRIP);
		glColor3fv (lit);
		glTexCoord2f (tex[1], tex[1]);
		glVertex3f (-vert[0], 0.0f, -vert[0]);
		glTexCoord2f (tex[2], tex[1]);
		glVertex3f (vert[0], 0.0f, -vert[0]);
		glColor3fv (unlit);
		glTexCoord2f (tex[0], tex[0]);
		glVertex3f (-vert[1], 0.0f, -vert[1]);
		glTexCoord2f (tex[3], tex[0]);
		glVertex3f (vert[1], 0.0f, -vert[1]);
		glEnd ();
		glBegin (GL_TRIANGLE_STRIP);
		glColor3fv (lit);
		glTexCoord2f (tex[1], tex[2]);
		glVertex3f (-vert[0], 0.0f, vert[0]);
		glTexCoord2f (tex[1], tex[1]);
		glVertex3f (-vert[0], 0.0f, -vert[0]);
		glColor3fv (unlit);
		glTexCoord2f (tex[0], tex[3]);
		glVertex3f (-vert[1], 0.0f, vert[1]);
		glTexCoord2f (tex[0], tex[0]);
		glVertex3f (-vert[1], 0.0f, -vert[1]);
		glEnd ();
		glBegin (GL_TRIANGLE_STRIP);
		glColor3fv (lit);
		glTexCoord2f (tex[2], tex[2]);
		glVertex3f (vert[0], 0.0f, vert[0]);
		glTexCoord2f (tex[1], tex[2]);
		glVertex3f (-vert[0], 0.0f, vert[0]);
		glColor3fv (unlit);
		glTexCoord2f (tex[3], tex[3]);
		glVertex3f (vert[1], 0.0f, vert[1]);
		glTexCoord2f (tex[0], tex[3]);
		glVertex3f (-vert[1], 0.0f, vert[1]);
		glEnd ();
		glBegin (GL_TRIANGLE_STRIP);
		glColor3fv (lit);
		glTexCoord2f (tex[2], tex[1]);
		glVertex3f (vert[0], 0.0f, -vert[0]);
		glTexCoord2f (tex[2], tex[2]);
		glVertex3f (vert[0], 0.0f, vert[0]);
		glColor3fv (unlit);
		glTexCoord2f (tex[3], tex[0]);
		glVertex3f (vert[1], 0.0f, -vert[1]);
		glTexCoord2f (tex[3], tex[3]);
		glVertex3f (vert[1], 0.0f, vert[1]);
		glEnd ();
		glEndList ();
		glNewList (earthlist, GL_COMPILE);
		lit[0] = lit[1] = lit[2] = 0.4f;
		glColor3fv (lit);
		glBegin (GL_TRIANGLE_STRIP);
		glTexCoord2f (tex[1], tex[1]);
		glVertex3f (-vert[0], 0.0f, -vert[0]);
		glTexCoord2f (tex[1], tex[2]);
		glVertex3f (-vert[0], 0.0f, vert[0]);
		glTexCoord2f (tex[2], tex[1]);
		glVertex3f (vert[0], 0.0f, -vert[0]);
		glTexCoord2f (tex[2], tex[2]);
		glVertex3f (vert[0], 0.0f, vert[0]);
		glEnd ();
		glBegin (GL_TRIANGLE_STRIP);
		glColor3fv (lit);
		glTexCoord2f (tex[1], tex[1]);
		glVertex3f (-vert[0], 0.0f, -vert[0]);
		glTexCoord2f (tex[2], tex[1]);
		glVertex3f (vert[0], 0.0f, -vert[0]);
		glColor3fv (unlit);
		glTexCoord2f (tex[0], tex[0]);
		glVertex3f (-vert[1], 0.0f, -vert[1]);
		glTexCoord2f (tex[3], tex[0]);
		glVertex3f (vert[1], 0.0f, -vert[1]);
		glEnd ();
		glBegin (GL_TRIANGLE_STRIP);
		glColor3fv (lit);
		glTexCoord2f (tex[1], tex[2]);
		glVertex3f (-vert[0], 0.0f, vert[0]);
		glTexCoord2f (tex[1], tex[1]);
		glVertex3f (-vert[0], 0.0f, -vert[0]);
		glColor3fv (unlit);
		glTexCoord2f (tex[0], tex[3]);
		glVertex3f (-vert[1], 0.0f, vert[1]);
		glTexCoord2f (tex[0], tex[0]);
		glVertex3f (-vert[1], 0.0f, -vert[1]);
		glEnd ();
		glBegin (GL_TRIANGLE_STRIP);
		glColor3fv (lit);
		glTexCoord2f (tex[2], tex[2]);
		glVertex3f (vert[0], 0.0f, vert[0]);
		glTexCoord2f (tex[1], tex[2]);
		glVertex3f (-vert[0], 0.0f, vert[0]);
		glColor3fv (unlit);
		glTexCoord2f (tex[3], tex[3]);
		glVertex3f (vert[1], 0.0f, vert[1]);
		glTexCoord2f (tex[0], tex[3]);
		glVertex3f (-vert[1], 0.0f, vert[1]);
		glEnd ();
		glBegin (GL_TRIANGLE_STRIP);
		glColor3fv (lit);
		glTexCoord2f (tex[2], tex[1]);
		glVertex3f (vert[0], 0.0f, -vert[0]);
		glTexCoord2f (tex[2], tex[2]);
		glVertex3f (vert[0], 0.0f, vert[0]);
		glColor3fv (unlit);
		glTexCoord2f (tex[3], tex[0]);
		glVertex3f (vert[1], 0.0f, -vert[1]);
		glTexCoord2f (tex[3], tex[3]);
		glVertex3f (vert[1], 0.0f, vert[1]);
		glEnd ();
		glEndList ();
	}
}

void cleanupWorld ()
{
	glDeleteLists (starlist, 1);
	glDeleteLists (moonlist, 1);
	glDeleteLists (moonglowlist, 1);
	glDeleteLists (sunsetlist, 1);
	glDeleteLists (earthnearlist, 1);
	glDeleteLists (earthfarlist, 1);
	glDeleteLists (earthlist, 1);
}

void updateWorld ()
{
	static float cloudShift = 0.0f;
	static float recipHalfCloud = 1.0f / float (CLOUDMESH / 6);

	if (dClouds) {
		// Blow clouds (particles should get the same drift at 2000 feet)
		// Maximum dWind is 100 and texture repeats every 6666.67 feet
		// so 100 * 0.00015 * 6666.67 = 100 ft/sec maximum windspeed.
		cloudShift += 0.00015f * float (dWind) * elapsedTime;

		if (cloudShift > 1.0f)
			cloudShift -= 1.0f;
		for (int j = 0; j <= CLOUDMESH; j++) {
			for (int i = 0; i <= CLOUDMESH; i++) {
				clouds[i][j][6] = clouds[i][j][7] = clouds[i][j][8] = clouds[i][j][5];	// darken clouds
				clouds[i][j][3] = float (-i) * recipHalfCloud + cloudShift;
			}
		}
	}
}

void drawWorld ()
{
	int i, j;
	static float moonRotation = rsRandf (360.0f);
	static float moonHeight = rsRandf (40.0f) + 20.0f;

	glMatrixMode (GL_MODELVIEW);;
	glDisable (GL_DEPTH_TEST);

	// draw stars
	if (dStardensity) {
		glDisable (GL_BLEND);
		glCallList (starlist);
	}
	// draw moon
	if (dMoon) {
		glPushMatrix ();
		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glRotatef (moonRotation, 0, 1, 0);
		glRotatef (moonHeight, 1, 0, 0);
		glTranslatef (0.0f, 0.0f, -20000.0f);
		glCallList (moonlist);
		glPopMatrix ();
	}
	// draw clouds
	if (dClouds) {
		glPushMatrix ();
		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBindTexture (GL_TEXTURE_2D, cloudtex);
		glTranslatef (0.0f, 0.0f, 0.0f);
		for (j = 0; j < CLOUDMESH; j++) {
			glBegin (GL_TRIANGLE_STRIP);
			for (i = 0; i <= CLOUDMESH; i++) {
				glColor3f (clouds[i][j + 1][6], clouds[i][j + 1][7], clouds[i][j + 1][8]);
				glTexCoord2f (clouds[i][j + 1][3], clouds[i][j + 1][4]);
				glVertex3fv (clouds[i][j + 1]);
				glColor3f (clouds[i][j][6], clouds[i][j][7], clouds[i][j][8]);
				glTexCoord2f (clouds[i][j][3], clouds[i][j][4]);
				glVertex3fv (clouds[i][j]);
			}
			glEnd ();
		}
		glPopMatrix ();
	}
	// draw sunset
	if (doSunset) {
		glEnable (GL_BLEND);
		glBlendFunc (GL_ONE, GL_ONE);
		glCallList (sunsetlist);
	}
	// draw moon's halo
	if (dMoonglow && dMoon) {
		glPushMatrix ();
		float glow = float (dMoonglow) * 0.005f;	// half of max possible value

		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE);
		glRotatef (moonRotation, 0, 1, 0);
		glRotatef (moonHeight, 1, 0, 0);
		glTranslatef (0.0f, 0.0f, -20000.0f);
		glColor4f (1.0f, 1.0f, 1.0f, glow);
		glCallList (moonglowlist);	// halo
		glScalef (6000.0f, 6000.0f, 6000.0f);
		glColor4f (1.0f, 1.0f, 1.0f, glow * 0.7f);
		glCallList (flarelist[0]);	// spot
		glPopMatrix ();
	}
	// draw earth
	if (dEarth) {
		glPushMatrix ();
		glDisable (GL_BLEND);
		glBindTexture (GL_TEXTURE_2D, earthneartex);
		glCallList (earthnearlist);
		glBindTexture (GL_TEXTURE_2D, earthfartex);
		glCallList (earthfarlist);
		if (dAmbient <= 30) {
			glEnable (GL_BLEND);
			glBlendFunc (GL_ONE, GL_ONE);
			glBindTexture (GL_TEXTURE_2D, earthlighttex);
			glCallList (earthlist);
		}
		glPopMatrix ();
	}
}
