/*
 * Copyright (C) 1999  Andreas Gustafsson
 * Ported to Linux by Tugrul Galatali <tugrul@galatali.com>
 *
 * Colorfire is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 *
 * Colorfire is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Colorfire screen saver

#include <math.h>
#include <stdio.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "colorfire_textures.h"
#include "driver.h"
#include "loadTexture.h"
#include "rsRand.h"

const char *hack_name = "Colorfire by Andreas Gustafsson (Shadow/Noice) (C)1999";

int dTexture;

#define NR_WAVES 24
#define TEXSIZE 256

float wrot[NR_WAVES], wtime[NR_WAVES], wr[NR_WAVES], wg[NR_WAVES], wb[NR_WAVES], wspd[NR_WAVES], wmax[NR_WAVES];
float v1 = 0, v2 = 0, v3 = 0;

void initWave (int nr)
{
	wtime[nr] = 0;
	wrot[nr] = rsRandf (360);
	wr[nr] = rsRandf (1.0);
	wg[nr] = rsRandf (1.0);
	wb[nr] = rsRandf (1.0);
	wspd[nr] = 0.5f + rsRandf (0.3f);
	wmax[nr] = 1.0f + rsRandf (1.0f);
}

void hack_reshape (xstuff_t * XStuff)
{
	// Window initialization
	glViewport (0, 0, (GLsizei) XStuff->windowWidth, (GLsizei) XStuff->windowHeight);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	gluPerspective (55.0, (GLfloat) XStuff->windowWidth / (GLfloat) XStuff->windowHeight, 1.0, 20.0);
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
	glTranslatef (0.0, 0.0, -3.0);
	glClearColor (0.0, 0.0, 0.0, 0.0);
}

void hack_init (xstuff_t * XStuff)	// Called right after the window is created, and OpenGL is initialized.
{
	int i;

	hack_reshape (XStuff);

	for (i = 0; i < NR_WAVES; i++) {
		initWave (i);
		wtime[i] = 3.0f + rsRandf (1.0f);
	}

	if (dTexture) {
		unsigned char *l_tex;
		unsigned int tex;

		glGenTextures (1, &tex);

		glBindTexture (GL_TEXTURE_2D, tex);

		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);

		switch (dTexture) {
		case 1:
			LOAD_TEXTURE (l_tex, smokemap, smokemap_compressedsize, smokemap_size);
			gluBuild2DMipmaps (GL_TEXTURE_2D, 1, TEXSIZE, TEXSIZE, GL_RGB, GL_UNSIGNED_BYTE, l_tex);
			FREE_TEXTURE (l_tex)

			break;
	
		case 2:
			LOAD_TEXTURE (l_tex, ripplemap, ripplemap_compressedsize, ripplemap_size);
			gluBuild2DMipmaps (GL_TEXTURE_2D, 1, TEXSIZE, TEXSIZE, GL_RGB, GL_UNSIGNED_BYTE, l_tex);
			FREE_TEXTURE (l_tex)

			break;

		case 3:
			int i, j;
			double x, y, r, d;
			unsigned char texbuf[64][64];

			memset ((void *)&texbuf, 0, 4096);

			r = 32;
			for (i = 0; i < 64; i++) {
				for (j = 0; j < 64; j++) {
					x = abs (i - 32);
					y = abs (j - 32);
					d = sqrt (x * x + y * y);

					if (d < r) { 
						d = 1 - (d / r);
						texbuf[i][j] = char (255 * d * d);
					}
				} 
			}

			gluBuild2DMipmaps (GL_TEXTURE_2D, 1, 64, 64, GL_LUMINANCE, GL_UNSIGNED_BYTE, texbuf);

			glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			break;
		}
	} else {
		unsigned char img[] = { 0, 255, 255, 0 };

		gluBuild2DMipmaps (GL_TEXTURE_2D, 1, 2, 2, GL_LUMINANCE, GL_UNSIGNED_BYTE, img);
		glTexImage2D (GL_TEXTURE_2D, 0, 1, 2, 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, img);
	}

	glEnable (GL_TEXTURE_2D);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE);
	glEnable (GL_BLEND);
}

void hack_cleanup (xstuff_t * XStuff)
{
}

void drawWave (int nr, float fDeltaTime)
{
	float colMod = 1.0;

	glPushMatrix ();
	glScalef (wtime[nr], wtime[nr], wtime[nr]);
	if (wtime[nr] > (wmax[nr] - 1.0))
		colMod = wmax[nr] - wtime[nr];
	glRotatef (wrot[nr], 1.f, 0, 0);
	glRotatef (wrot[nr], 0, 1.f, 0);
	glRotatef (wrot[nr], 0, 0, 1.f);
	glColor3f (colMod * wr[nr], colMod * wg[nr], colMod * wb[nr]);
	glBegin (GL_QUADS);
	glTexCoord2f (0, 0);
	glVertex3f (-1, -1, 0);
	glTexCoord2f (1, 0);
	glVertex3f (1, -1, 0);
	glTexCoord2f (1, 1);
	glVertex3f (1, 1, 0);
	glTexCoord2f (0, 1);
	glVertex3f (-1, 1, 0);
	glEnd ();
	wtime[nr] += fDeltaTime * wspd[nr];
	if (wtime[nr] > wmax[nr])
		initWave (nr);
	glPopMatrix ();
}

void hack_draw (xstuff_t * XStuff, double currentTime, float frameTime)
{
	glClear (GL_COLOR_BUFFER_BIT);
	glPushMatrix ();
	glRotatef (v1, 1.f, 0, 0);
	glRotatef (v2, 0, 1.f, 0);
	glRotatef (v3, 0, 0, 1.f);
	v1 += frameTime * 5;
	if (v1 > 360)
		v1 -= 360;
	v2 += frameTime * 10;
	if (v2 > 360)
		v2 -= 360;
	v3 += frameTime * 7;
	if (v3 > 360)
		v3 -= 360;
	for (int i = 0; i < NR_WAVES; i++)
		drawWave (i, frameTime);
	glPopMatrix ();
}

void hack_handle_opts (int argc, char **argv)
{
	int change_flag = 0;

	dTexture = 1;

	while (1) {
		int c;

#ifdef HAVE_GETOPT_H
		static struct option long_options[] = {
			{"help", 0, 0, 'h'},
			DRIVER_OPTIONS_LONG
			{"texture", 1, 0, 't'},
			{"smoke", 0, 0, 1},
			{"ripples", 0, 0, 2},
			{"smooth", 0, 0, 3},
			{0, 0, 0, 0}
		};

		c = getopt_long (argc, argv, DRIVER_OPTIONS_SHORT "ht:", long_options, NULL);
#else
		c = getopt (argc, argv, DRIVER_OPTIONS_SHORT "ht:");
#endif
		if (c == -1)
			break;

		switch (c) {
			DRIVER_OPTIONS_CASES 
		case 'h':printf ("%s:"
#ifndef HAVE_GETOPT_H
							      " Not built with GNU getopt.h, long options *NOT* enabled."
#endif
							      "\n" DRIVER_OPTIONS_HELP "\t--texture/-t <arg>\n" "\t--smoke\n" "\t--ripples\n" "\t--smooth\n", argv[0]);

			exit (1);
		case 't':
			change_flag = 1;
			dTexture = strtol_minmaxdef (optarg, 10, 0, 3, 0, 1, "--texture: ");
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
		dTexture = rsRandi (3) + 1;
	}
}
