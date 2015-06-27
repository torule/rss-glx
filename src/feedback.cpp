/*
 * Copyright (C) 2009 Tugrul Galatali <tugrul@galatali.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// feedback screensaver

const char *hack_name = "feedback";

#include "driver.h"
#include <algorithm>
#include <GL/gl.h>
#include <GL/glu.h>
#include <cstdio>
#include <cassert>
#include <cmath>

#include "rgbhsl.h"
#include "rsRand.h"
#include <rsMath/rsVec.h>

using namespace std;

GLuint tex;
unsigned int width = 256, height = 256;
unsigned int cwidth = 8, cheight = 8;
rsVec *displacements, *velocities, *accelerations;
rsVec totalV;

bool dGrey = false;
float dSaturation = 1.0;
float dLightness = 1.0;
bool dGrid = false;
int dPeriod = 5;
int dTexSize = 8;
float dSpeed = 1;

void hack_draw (xstuff_t * XStuff, double currentTime, float frameTime)
{
// ################################################################################
// Frame texture with a rotating color

	glViewport (0, 0, width, height);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	gluOrtho2D (-0.125f, 1.125f, -0.125f, 1.125f);
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();

	{
		float h, s, l;
		if (dGrey) {
			h = 0.0;
			s = 0.0;
			l = currentTime / dPeriod - trunc(currentTime / dPeriod);

			l = l * 2.0;

			if (l > 1.0) {
				l = 2.0 - l;
			}
		} else {
			h = currentTime / dPeriod - trunc(currentTime / dPeriod);
			s = dSaturation;
			l = dLightness;
		}

		float r, g, b;

		hsl2rgb(h, s, l, r, g, b);

		glClearColor (r, g, b, 1.0);
	}

	glClear (GL_COLOR_BUFFER_BIT);

	glColor3f (1.0f, 1.0f, 1.0f);

	glBegin (GL_QUADS);
	glTexCoord2f (0.0, 1.0); glVertex2d (0.0, 1.0);
	glTexCoord2f (1.0, 1.0); glVertex2d (1.0, 1.0);
	glTexCoord2f (1.0, 0.0); glVertex2d (1.0, 0.0);
	glTexCoord2f (0.0, 0.0); glVertex2d (0.0, 0.0);
	glEnd ();

	glCopyTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, 0, 0, width, height, 0);

// ################################################################################
// Warp framed texture

	glViewport (0, 0, width, height);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	gluOrtho2D (0.0f, 1.0f, 0.0f, 1.0f);
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();

	glColor3f (1.0, 1.0, 1.0);
	glBegin (GL_QUADS);
	for (unsigned int dh = 0; dh < cheight; ++dh) {
		for (unsigned int dw = 0; dw < cwidth; ++dw) {
			const rsVec da = rsVec(dw / float(cwidth),       dh / float(cheight),       0.0);
			const rsVec db = rsVec(dw / float(cwidth),       (dh + 1) / float(cheight), 0.0);
			const rsVec dc = rsVec((dw + 1) / float(cwidth), (dh + 1) / float(cheight), 0.0);
			const rsVec dd = rsVec((dw + 1) / float(cwidth), dh / float(cheight),       0.0);

			const unsigned nh = (dh + 1) & (cheight - 1);
			const unsigned nw = (dw + 1) & (cwidth - 1);
			const rsVec sa = displacements[dh * cwidth + dw] + rsVec(dw,     dh, 0.0);
			const rsVec sb = displacements[nh * cwidth + dw] + rsVec(dw,     dh + 1, 0.0);
			const rsVec sc = displacements[nh * cwidth + nw] + rsVec(dw + 1, dh + 1, 0.0);
			const rsVec sd = displacements[dh * cwidth + nw] + rsVec(dw + 1, dh, 0.0);

			glTexCoord2f (sa[0] / cwidth * 0.8 + 0.1, sa[1] / cheight * 0.8 + 0.1);
			glVertex2f (da[0], da[1]);
			glTexCoord2f (sb[0] / cwidth * 0.8 + 0.1, sb[1] / cheight * 0.8 + 0.1);
			glVertex2f (db[0], db[1]);
			glTexCoord2f (sc[0] / cwidth * 0.8 + 0.1, sc[1] / cheight * 0.8 + 0.1);
			glVertex2f (dc[0], dc[1]);
			glTexCoord2f (sd[0] / cwidth * 0.8 + 0.1, sd[1] / cheight * 0.8 + 0.1);
			glVertex2f (dd[0], dd[1]);
		}
	}
	glEnd ();

	glCopyTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, 0, 0, width, height, 0);

// ################################################################################
// Render warped texture to screen

	glViewport (0, 0, XStuff->windowWidth, XStuff->windowHeight);

	glClearColor (0.0, 0.0, 0.0, 1.0);
	glClear (GL_COLOR_BUFFER_BIT);

	glColor3f (1.0, 1.0, 1.0);

	glBegin (GL_QUADS);
	glTexCoord2f (0.0, 1.0); glVertex2d (0.0, 1.0);
	glTexCoord2f (1.0, 1.0); glVertex2d (1.0, 1.0);
	glTexCoord2f (1.0, 0.0); glVertex2d (1.0, 0.0);
	glTexCoord2f (0.0, 0.0); glVertex2d (0.0, 0.0);
	glEnd ();

// ################################################################################
// Optionally display warping grid

	if (dGrid) {
		glDisable (GL_TEXTURE_2D);

		glBegin (GL_LINES);
		for (unsigned int dh = 0; dh < cheight; ++dh) {
			for (unsigned int dw = 0; dw < cwidth; ++dw) {
				const unsigned nh = (dh + 1) & (cheight - 1);
				const unsigned nw = (dw + 1) & (cwidth - 1);
				const rsVec a = displacements[dh * cwidth + dw] + rsVec(dw,     dh, 0.0);
				const rsVec b = displacements[nh * cwidth + dw] + rsVec(dw,     dh + 1, 0.0);
				const rsVec c = displacements[nh * cwidth + nw] + rsVec(dw + 1, dh + 1, 0.0);
				const rsVec d = displacements[dh * cwidth + nw] + rsVec(dw + 1, dh, 0.0);

				glColor3f (0.0, 1.0, 0.0);

				glVertex2f (float(dw) / cwidth, float(dh) / cheight);
				glVertex2f (a[0] / cwidth, a[1] / cheight);

				glColor3f (1.0, 0.0, 0.0);

				glVertex2f (a[0] / cwidth, a[1] / cheight);
				glVertex2f (b[0] / cwidth, b[1] / cheight);

				glVertex2f (b[0] / cwidth, b[1] / cheight);
				glVertex2f (c[0] / cwidth, c[1] / cheight);

				glVertex2f (c[0] / cwidth, c[1] / cheight);
				glVertex2f (d[0] / cwidth, d[1] / cheight);

				glVertex2f (d[0] / cwidth, d[1] / cheight);
				glVertex2f (a[0] / cwidth, a[1] / cheight);
			}
		}
		glEnd ();

		glEnable (GL_TEXTURE_2D);
	}

// ################################################################################
// Jiggle grid

	// Only compute forces along leading edge to save duplication since the grid wraps...
	// xxL
	// xSL
	// xLL
	for (unsigned int dh = 0, ii = 0; dh < cheight; ++dh) {
		for (unsigned int dw = 0; dw < cwidth; ++dw) {
			const int offsets[4][2] = { { 1, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 } };

			accelerations[ii] += displacements[ii] * -displacements[ii].length();

			for (int jj = 0; jj < 4; ++jj) {
				const int nh = (int)dh + offsets[jj][0];
				const int nw = (int)dw + offsets[jj][1];
				const int nii = (nh & (cheight - 1)) * cwidth + (nw & (cwidth - 1));

				const rsVec nn = displacements[nii] - displacements[ii] + rsVec(nh - (int)dh, nw - (int)dw, 0.0);

				const float nominalDisplacements[3] = { 0.0f, 1.0f, M_SQRT2 };
				const float nominalDisplacement = nominalDisplacements[abs(offsets[jj][0]) + abs(offsets[jj][1])];

				const rsVec ff = nn * (nn.length() - nominalDisplacement);
				accelerations[ii] += ff;
				accelerations[nii] -= ff;
			}

			++ii;
		}
	}

	rsVec newTotalV(0, 0, 0);
	const float totalVScalar = totalV.length();
	const float stepSize = min(frameTime, 0.05f);
	for (unsigned int dh = 0, ii = 0; dh < cheight; ++dh) {
		for (unsigned int dw = 0; dw < cwidth; ++dw) {
			velocities[ii] += accelerations[ii] * stepSize;
			accelerations[ii] = rsVec(0.0f, 0.0f, 0.0f);

			// Don't let things get too fast
			if (totalVScalar > 20.0f) {
				velocities[ii] -= velocities[ii] / exp(totalVScalar - 20.0);
			}

			newTotalV += rsVec(abs(velocities[ii][0]), abs(velocities[ii][1]), 0);

			displacements[ii] += velocities[ii] * stepSize * dSpeed;

			// or displacements too large
			if (displacements[ii].length() > 1.0) {
				displacements[ii] = displacements[ii] / displacements[ii].length();
			}

			++ii;
		}
	}
	totalV = newTotalV;
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
	{
		width = height = 1 << dTexSize;
		int newTexSize = dTexSize;
		while ((width > XStuff->windowWidth) || (height > XStuff->windowHeight)) {
			--newTexSize;
			width = width >> 1;
			height = height >> 1;
		}

		if (newTexSize != dTexSize) {
			printf("--texsize reduced to %d from %d to fit display\n", newTexSize, dTexSize);
			dTexSize = newTexSize;
		}

		unsigned char *pixels = new unsigned char[width * height * 3];
		for (unsigned int hh = 0, ii = 0; hh < height; ++hh) {
			for (unsigned int ww = 0; ww < width; ++ww) {
				float r, g, b;

				if (dGrey) {
					hsl2rgb(0.0, 0.0, hh * ww / float(height * width), r, g, b);
				} else {
					hsl2rgb(hh / float(height), dSaturation, dLightness, r, g, b);
				}

				pixels[ii++] = r * 255;
				pixels[ii++] = g * 255;
				pixels[ii++] = b * 255;
			}
		}

		glGenTextures (1, &tex);

		glBindTexture (GL_TEXTURE_2D, tex);

		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

		delete [] pixels;
	}

	displacements = new rsVec[cwidth * cheight];
	velocities = new rsVec[cwidth * cheight];
	accelerations = new rsVec[cwidth * cheight];

	for (unsigned int hh = 0, ii = 0; hh < cheight; ++hh) {
		for (unsigned int ww = 0; ww < cwidth; ++ww) {
			displacements[ii][0] = rsRandf(0.5) - 0.25;
			displacements[ii][1] = rsRandf(0.5) - 0.25;
			displacements[ii][2] = 0.0;
			velocities[ii] = rsVec(0.0f, 0.0f, 0.0f);
			accelerations[ii] = rsVec(0.0f, 0.0f, 0.0f);
			++ii;
		}
	}

	glEnable (GL_TEXTURE_2D);

	hack_reshape(XStuff);
}

void hack_cleanup (xstuff_t * XStuff)
{
	delete [] displacements;
	delete [] velocities;
	delete [] accelerations;
}

void hack_handle_opts (int argc, char **argv)
{
#ifdef DEBUG
	printf("handle_hack_opts\n");
#endif
	while (1) {
		int c;

#ifdef HAVE_GETOPT_H
		static struct option long_options[] = {
			{"help", 0, 0, 'h'},
			{"grey", 0, 0, 'g'},
			{"saturation", 0, 0, 'S'},
			{"lightness", 0, 0, 'L'},
			{"period", 1, 0, 'p'},
			{"speed", 1, 0, 's'},
			{"cells", 1, 0, 'c'},
			{"texsize", 1, 0, 't'},
			{"grid", 0, 0, 'i'},
			DRIVER_OPTIONS_LONG
		};

		c = getopt_long (argc, argv, DRIVER_OPTIONS_SHORT "hgS:L:p:s:c:t:i", long_options, NULL);
#else
		c = getopt (argc, argv, DRIVER_OPTIONS_SHORT "hgS:L:p:s:c:t:i");
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
				"\t--grey/-g\n"
				"\t--saturation/-S <arg>\n"
				"\t--lightness/-L <arg>\n"
				"\t--period/-p <arg>\n"
				"\t--speed/-s <arg>\n"
				"\t--cells/-c <arg>\n"
				"\t--texsize/-t <arg>\n"
				"\t--grid/-i\n", argv[0]);
			exit (1);
		case 'g':
			dGrey = true;
			break;
		case 'S':
			dSaturation = strtol_minmaxdef (optarg, 10, 1, 255, 1, 255, "--saturation: ") / 255.0;
			break;
		case 'L':
			dLightness = strtol_minmaxdef (optarg, 10, 1, 255, 1, 255, "--lightness: ") / 255.0;
			break;
		case 'p':
			dPeriod = strtol_minmaxdef (optarg, 10, 1, 100, 1, 5, "--period: ");
			break;
		case 's':
			dSpeed = strtol_minmaxdef (optarg, 10, 1, 100, 1, 10, "--speed: ") / 10.0;
			break;
		case 'c':
			cwidth = cheight = 1 << strtol_minmaxdef (optarg, 10, 0, 6, 1, 3, "--cells: ");
			break;
		case 't':
			dTexSize = strtol_minmaxdef (optarg, 10, 6, 12, 1, 8, "--texsize: ");
			break;
		case 'i':
			dGrid = true;
			break;
		}
	}
}
