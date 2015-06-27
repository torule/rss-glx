/*
 * Copyright (C) 2003 Holmes Futrell <holmes@neatosoftware.com>
 * Ported to Linux by Tugrul Galatali <tugrul@galatali.com>
 *
 * SpirographX is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 *
 * SpirographX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// SpirographX screen saver

#include "driver.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "rsDefines.h"
#include "rsRand.h"
#include "rsMath/rsTrigonometry.h"

const char *hack_name = "SpirographX";

#define MAXSUBLOOPS 5
#define MAXDETAIL 100

float equationBase;
float speed;
int graphTo;
int SUBLOOPS;
int DETAIL;
int blurWidth;
float blurAlpha;
float lineAlpha;
int animated;
int timeInterval;

float points[2 * 360 * MAXSUBLOOPS * MAXDETAIL];

int colorBits, depthBits;
int runningFullScreen;

float lineColor[3];
float blurColor[3];

void changeSettings ()
{
	do {
		equationBase = rsRandf (10) - 5;
	} while (equationBase <= 2 && equationBase >= -2); // we don't want between 1 and -1

	blurColor[0] = rsRandi (100) / 50.0;
	blurColor[1] = rsRandi (100) / 50.0;
	blurColor[2] = rsRandi (100) / 50.0;

	lineColor[0] = rsRandi (100) / 50.0;
	lineColor[1] = rsRandi (100) / 50.0;
	lineColor[2] = rsRandi (100) / 50.0;

	SUBLOOPS = rsRandi (3) + 2;
	graphTo = rsRandi (16) + 15;
	speed = (rsRandi (225) + 75) / 1000000.0;

	if (rsRandi (2) == 1) {
		speed *= -1;
	}
}

void getAll ()
{
	int m, n;

	float poweranswer[SUBLOOPS];
	poweranswer[0] = 1;
	for (n = 1; n < SUBLOOPS; n++) {
		poweranswer[n] = poweranswer[n - 1] * equationBase;
	}

	const int numberOfPoints = 2 * PI * graphTo * DETAIL;

	memset(points, 0, numberOfPoints * 2 * sizeof(float));

	for (m = 0; m < numberOfPoints; m++) {
		const float moverd = (float)m / DETAIL;
		for (n = 0; n < SUBLOOPS; n++) {
			const float pointpoweroverdetail = moverd * poweranswer[n];
			float sinppod, cosppod;
			rsSinCosf(fmod(pointpoweroverdetail, RS_SINCOS_MAX), &sinppod, &cosppod);

			points[2 * m + 0] += cosppod / poweranswer[n];
			points[2 * m + 1] += sinppod / poweranswer[n];
		}
	}
}

void drawAll ()
{
	const int numberOfPoints = 2 * PI * graphTo * DETAIL;

	glDrawArrays(GL_LINE_STRIP, 0, numberOfPoints);
}

void hack_reshape (xstuff_t * XStuff)
{
	glViewport (0, 0, (GLsizei) XStuff->windowWidth, (GLsizei) XStuff->windowHeight);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	gluPerspective (45.0f, (GLfloat) XStuff->windowWidth / (GLfloat) XStuff->windowHeight, 0.1f, 100.0f);
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
}

void hack_init (xstuff_t * XStuff)	// Called right after the window is created, and OpenGL is initialized.
{
	rsSinCosInit();

	blurWidth = 5;

	glVertexPointer(2, GL_FLOAT, 0, points);
	glEnableClientState(GL_VERTEX_ARRAY);

	glClearColor (0.0f, 0.0f, 0.0f, 0.5f);
	glClearDepth (1.0f);
	glEnable (GL_BLEND);
	glEnable (GL_LINE_SMOOTH);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE);
	glLoadIdentity ();

	changeSettings ();

	hack_reshape (XStuff);
}

void hack_cleanup (xstuff_t * XStuff)
{
}

void hack_draw (xstuff_t * XStuff, double currentTime, float frameTime)
{
	static double lastSettingsChange = -1;

	if (lastSettingsChange == -1)
		lastSettingsChange = currentTime;

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity ();
	getAll ();

	glTranslatef(0.0, 0.0, -3.0f);

	glLineWidth (blurWidth * sqrt((GLfloat) (XStuff->windowWidth * XStuff->windowHeight) / (500 * 400)));
	glColor4f (blurColor[0], blurColor[1], blurColor[2], 0.2);
	drawAll ();

	glLineWidth(1);
	glColor4f (lineColor[0], lineColor[1], lineColor[2], 0.4);
	drawAll ();

	if (currentTime - lastSettingsChange > timeInterval) {
		lastSettingsChange = currentTime;

		changeSettings ();
	}

	equationBase += speed * (frameTime / (1.0 / 30.0));

	glFlush ();
}

void hack_handle_opts (int argc, char **argv)
{
	timeInterval = 15;
	DETAIL = 45;

	while (1) {
		int c;

#ifdef HAVE_GETOPT_H
		static struct option long_options[] = {
			{"help", 0, 0, 'h'},
			DRIVER_OPTIONS_LONG {"detail", 1, 0, 'd'},
			{"interval", 1, 0, 'i'},
			{0, 0, 0, 0}
		};

		c = getopt_long (argc, argv, DRIVER_OPTIONS_SHORT "hd:i:", long_options, NULL);
#else
		c = getopt (argc, argv, DRIVER_OPTIONS_SHORT "hd:i:");
#endif
		if (c == -1)
			break;

		switch (c) {
			DRIVER_OPTIONS_CASES case 'h':printf ("%s:"
#ifndef HAVE_GETOPT_H
							      " Not built with GNU getopt.h, long options *NOT* enabled."
#endif
							      "\n" DRIVER_OPTIONS_HELP "\t--detail/-d <arg>\n" "\t--interval/-i <arg>\n", argv[0]);
				exit (1);
			case 'd':
				DETAIL = strtol_minmaxdef (optarg, 10, 10, MAXDETAIL, 1, 45, "--detail: ");
				break;
			case 'i':
				timeInterval = strtol_minmaxdef (optarg, 10, 5, 120, 1, 15, "--interval: ");
		}
	}
}
