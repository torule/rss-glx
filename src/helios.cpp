/*
 * Copyright (C) 2002  Terence M. Welsh
 * Ported to Linux by Tugrul Galatali <tugrul@galatali.com>
 *
 * Helios is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 *
 * Helios is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Helios screensaver

#include <math.h>
#include <stdio.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

#include "driver.h"
#include "Implicit/impCubeVolume.h"
#include "Implicit/impCrawlPoint.h"
#include "Implicit/impSphere.h"
#include "loadTexture.h"
#include "rgbhsl.h"
#include "rsDefines.h"
#include "rsRand.h"
#include "rsMath/rsMatrix.h"
#include "rsMath/rsQuat.h"
#include "rsMath/rsVec.h"

const char *hack_name = "helios";

#define TEXSIZE 256

extern unsigned char *spheremap;
extern unsigned int spheremap_size;
extern unsigned int spheremap_compressedsize;

#define LIGHTSIZE 64

class particle;
class emitter;
class attracter;
class ion;

// Global variables
int readyToDraw = 0;
unsigned char lightTexture[LIGHTSIZE][LIGHTSIZE];
float elapsedTime = 0.0f;
emitter *elist;
attracter *alist;
ion *ilist;
rsVec newRgb;
float billboardMat[16];
int pfd_swap_exchange;

// Parameters edited in the dialog box
int dIons;
int dSize;
int dEmitters;
int dAttracters;
int dSpeed;
int dCameraspeed;
int dSurface;
int dWireframe;
int dBlur;

impCubeVolume *volume;
impSurface *surface;
impSphere *spheres;

class particle {
      public:
	rsVec pos;
	rsVec rgb;
	float size;
};

class emitter:public particle {
      public:
	rsVec oldpos;
	rsVec targetpos;

	  emitter ();
	 ~emitter () {
	};
	void settargetpos (rsVec target) {
		oldpos = pos;
		targetpos = target;
	};
	void interppos (float n) {
		pos = oldpos * (1.0f - n) + targetpos * n;
	};
	void update () {
	};
};

emitter::emitter ()
{
	pos = rsVec (rsRandf (1000.0f) - 500.0f, rsRandf (1000.0f) - 500.0f, rsRandf (1000.0f) - 500.0f);
}

class attracter:public particle {
      public:
	rsVec oldpos;
	rsVec targetpos;

	  attracter ();
	 ~attracter () {
	};
	void settargetpos (rsVec target) {
		oldpos = pos;
		targetpos = target;
	};
	void interppos (float n) {
		pos = oldpos * (1.0f - n) + targetpos * n;
	};
	void update () {
	};
};

attracter::attracter ()
{
	pos = rsVec (rsRandf (1000.0f) - 500.0f, rsRandf (1000.0f) - 500.0f, rsRandf (1000.0f) - 500.0f);
}

class ion:public particle {
      public:
	float speed;

	  ion ();
	 ~ion () {
	};
	void start ();
	void update ();
	void draw ();
};

ion::ion ()
{
	float temp;

	pos = rsVec (0.0f, 0.0f, 0.0f);
	rgb = rsVec (0.0f, 0.0f, 0.0f);
	temp = rsRandf (2.0f) + 0.4f;
	size = float (dSize) * temp;
	speed = float (dSpeed) * 12.0f / temp;
}

void
  ion::start ()
{
	int i = rsRandi (dEmitters);
	float offset = elapsedTime * speed;

	pos = elist[i].pos;

	switch (rsRandi (14)) {
	case 0:
		pos[0] += offset;
		break;
	case 1:
		pos[0] -= offset;
		break;
	case 2:
		pos[1] += offset;
		break;
	case 3:
		pos[1] -= offset;
		break;
	case 4:
		pos[2] += offset;
		break;
	case 5:
		pos[2] -= offset;
		break;
	case 6:
		pos[0] += offset;
		pos[1] += offset;
		pos[2] += offset;
		break;
	case 7:
		pos[0] -= offset;
		pos[1] += offset;
		pos[2] += offset;
		break;
	case 8:
		pos[0] += offset;
		pos[1] -= offset;
		pos[2] += offset;
		break;
	case 9:
		pos[0] -= offset;
		pos[1] -= offset;
		pos[2] += offset;
		break;
	case 10:
		pos[0] += offset;
		pos[1] += offset;
		pos[2] -= offset;
		break;
	case 11:
		pos[0] -= offset;
		pos[1] += offset;
		pos[2] -= offset;
		break;
	case 12:
		pos[0] += offset;
		pos[1] -= offset;
		pos[2] -= offset;
		break;
	case 13:
		pos[0] -= offset;
		pos[1] -= offset;
		pos[2] -= offset;
	}

	rgb = newRgb;
}

void ion::update ()
{
	int i;
	int startOver = 0;
	static float startOverDistance;
	static rsVec force, tempvec;
	static float length, temp;

	force = rsVec (0.0f, 0.0f, 0.0f);
	for (i = 0; i < dEmitters; i++) {
		tempvec = pos - elist[i].pos;
		length = tempvec.normalize ();
		if (length > 11000.0f)
			startOver = 1;
		if (length <= 1.0f)
			temp = 1.0f;
		else
			temp = 1.0f / length;
		tempvec *= temp;
		force += tempvec;
	}
	startOverDistance = speed * elapsedTime;
	for (i = 0; i < dAttracters; i++) {
		tempvec = alist[i].pos - pos;
		length = tempvec.normalize ();
		if (length < startOverDistance)
			startOver = 1;
		if (length <= 1.0f)
			temp = 1.0f;
		else
			temp = 1.0f / length;
		tempvec *= temp;
		force += tempvec;
	}

	// Start this ion at an emitter if it gets too close to an attracter
	// or too far from an emitter
	if (startOver)
		start ();
	else {
		force.normalize ();
		pos += (force * elapsedTime * speed);
	}
}

void ion::draw ()
{
	glColor3f (rgb[0], rgb[1], rgb[2]);
	glPushMatrix ();
	glTranslatef (pos[0] * billboardMat[0] + pos[1] * billboardMat[4] + pos[2] * billboardMat[8],
		      pos[0] * billboardMat[1] + pos[1] * billboardMat[5] + pos[2] * billboardMat[9],
		      pos[0] * billboardMat[2] + pos[1] * billboardMat[6] + pos[2] * billboardMat[10]);
	glScalef (size, size, size);
	glCallList (1);
	glPopMatrix ();
}

void setTargets (int whichTarget)
{
	int i;

	switch (whichTarget) {
	case 0:		// random
		for (i = 0; i < dEmitters; i++)
			elist[i].settargetpos (rsVec (rsVec (rsRandf (1000.0f) - 500.0f, rsRandf (1000.0f) - 500.0f, rsRandf (1000.0f) - 500.0f)));
		for (i = 0; i < dAttracters; i++)
			alist[i].settargetpos (rsVec (rsVec (rsRandf (1000.0f) - 500.0f, rsRandf (1000.0f) - 500.0f, rsRandf (1000.0f) - 500.0f)));
		break;
	case 1:
		{		// line (all emitters on one side, all attracters on the other)
			float position = -500.0f, change = 1000.0f / float (dEmitters + dAttracters - 1);

			for (i = 0; i < dEmitters; i++) {
				elist[i].settargetpos (rsVec (rsVec (position, position * 0.5f, 0.0f)));
				position += change;
			}
			for (i = 0; i < dAttracters; i++) {
				alist[i].settargetpos (rsVec (rsVec (position, position * 0.5f, 0.0f)));
				position += change;
			}
			break;
		}
	case 2:
		{		// line (emitters and attracters staggered)
			float change;

			if (dEmitters > dAttracters) {
				change = 1000.0f / float (dEmitters * 2 - 1);
			} else {
				change = 1000.0f / float (dAttracters * 2 - 1);
			}
			float position = -500.0f;

			for (i = 0; i < dEmitters; i++) {
				elist[i].settargetpos (rsVec (rsVec (position, position * 0.5f, 0.0f)));
				position += change * 2.0f;
			}
			position = -500.0f + change;
			for (i = 0; i < dAttracters; i++) {
				alist[i].settargetpos (rsVec (rsVec (position, position * 0.5f, 0.0f)));
				position += change * 2.0f;
			}
			break;
		}
	case 3:
		{		// 2 lines (parallel)
			float change = 1000.0f / float (dEmitters * 2 - 1);
			float position = -500.0f;
			float height = -525.0f + float (dEmitters * 25);

			for (i = 0; i < dEmitters; i++) {
				elist[i].settargetpos (rsVec (rsVec (position, height, -50.0f)));
				position += change * 2.0f;
			}
			change = 1000.0f / float (dAttracters * 2 - 1);

			position = -500.0f;
			height = 525.0f - float (dAttracters * 25);

			for (i = 0; i < dAttracters; i++) {
				alist[i].settargetpos (rsVec (rsVec (position, height, 50.0f)));
				position += change * 2.0f;
			}
			break;
		}
	case 4:
		{		// 2 lines (skewed)
			float change = 1000.0f / float (dEmitters * 2 - 1);
			float position = -500.0f;
			float height = -525.0f + float (dEmitters * 25);

			for (i = 0; i < dEmitters; i++) {
				elist[i].settargetpos (rsVec (rsVec (position, height, 0.0f)));
				position += change * 2.0f;
			}
			change = 1000.0f / float (dAttracters * 2 - 1);

			position = -500.0f;
			height = 525.0f - float (dAttracters * 25);

			for (i = 0; i < dAttracters; i++) {
				alist[i].settargetpos (rsVec (rsVec (10.0f, height, position)));
				position += change * 2.0f;
			}
			break;
		}
	case 5:		// random distribution across a plane
		for (i = 0; i < dEmitters; i++)
			elist[i].settargetpos (rsVec (rsVec (rsRandf (1000.0f) - 500.0f, 0.0f, rsRandf (1000.0f) - 500.0f)));
		for (i = 0; i < dAttracters; i++)
			alist[i].settargetpos (rsVec (rsVec (rsRandf (1000.0f) - 500.0f, 0.0f, rsRandf (1000.0f) - 500.0f)));
		break;
	case 6:
		{		// random distribution across 2 planes
			float height = -525.0f + float (dEmitters * 25);

			for (i = 0; i < dEmitters; i++)
				elist[i].settargetpos (rsVec (rsVec (rsRandf (1000.0f) - 500.0f, height, rsRandf (1000.0f) - 500.0f)));

			height = 525.0f - float (dAttracters * 25);

			for (i = 0; i < dAttracters; i++)
				alist[i].settargetpos (rsVec (rsVec (rsRandf (1000.0f) - 500.0f, height, rsRandf (1000.0f) - 500.0f)));

			break;
		}
	case 7:
		{		// 2 rings (1 inside and 1 outside)
			float angle = 0.5f, cosangle, sinangle;
			float change = PIx2 / float (dEmitters);

			for (i = 0; i < dEmitters; i++) {
				angle += change;
				cosangle = cos (angle) * 200.0f;
				sinangle = sin (angle) * 200.0f;
				elist[i].settargetpos (rsVec (rsVec (cosangle, sinangle, 0.0f)));
			}
			angle = 1.5f;
			change = PIx2 / float (dAttracters);

			for (i = 0; i < dAttracters; i++) {
				angle += change;
				cosangle = cos (angle) * 500.0f;
				sinangle = sin (angle) * 500.0f;
				alist[i].settargetpos (rsVec (rsVec (cosangle, sinangle, 0.0f)));
			}
			break;
		}
	case 8:
		{		// ring (all emitters on one side, all attracters on the other)
			float angle = 0.5f, cosangle, sinangle;
			float change = PIx2 / float (dEmitters + dAttracters);

			for (i = 0; i < dEmitters; i++) {
				angle += change;
				cosangle = cos (angle) * 500.0f;
				sinangle = sin (angle) * 500.0f;
				elist[i].settargetpos (rsVec (rsVec (cosangle, sinangle, 0.0f)));
			}
			for (i = 0; i < dAttracters; i++) {
				angle += change;
				cosangle = cos (angle) * 500.0f;
				sinangle = sin (angle) * 500.0f;
				alist[i].settargetpos (rsVec (rsVec (cosangle, sinangle, 0.0f)));
			}
			break;
		}
	case 9:
		{		// ring (emitters and attracters staggered)
			float change;

			if (dEmitters > dAttracters) {
				change = PIx2 / float (dEmitters * 2);
			} else {
				change = PIx2 / float (dAttracters * 2);
			}
			float angle = 0.5f, cosangle, sinangle;

			for (i = 0; i < dEmitters; i++) {
				cosangle = cos (angle) * 500.0f;
				sinangle = sin (angle) * 500.0f;
				elist[i].settargetpos (rsVec (rsVec (cosangle, sinangle, 0.0f)));
				angle += change * 2.0f;
			}
			angle = 0.5f + change;
			for (i = 0; i < dAttracters; i++) {
				cosangle = cos (angle) * 500.0f;
				sinangle = sin (angle) * 500.0f;
				alist[i].settargetpos (rsVec (rsVec (cosangle, sinangle, 0.0f)));
				angle += change * 2.0f;
			}
			break;
		}
	case 10:		// 2 points
		for (i = 0; i < dEmitters; i++)
			elist[i].settargetpos (rsVec (rsVec (500.0f, 100.0f, 50.0f)));
		for (i = 0; i < dAttracters; i++)
			alist[i].settargetpos (rsVec (rsVec (-500.0f, -100.0f, -50.0f)));
		break;
	}
}

float surfaceFunction (float *position)
{
	static int i;
	static float value;
	static int points = dEmitters + dAttracters;

	value = 0.0f;
	for (i = 0; i < points; i++)
		value += spheres[i].value (position);

	return (value);
}

void hack_draw (xstuff_t * XStuff, double currentTime, float frameTime)
{
	int i;
	static int ionsReleased = 0;
	static float releaseTime = 0.0f;
	Display *dpy = XStuff->display;
#ifdef BENCHMARK
	static int a = 1;
#endif

	Window window = XStuff->window;

	elapsedTime = frameTime;

#ifdef BENCHMARK
	if (a++ == 1000)
		exit(0);
	elapsedTime = 0.1f;
#endif

	// Camera movements
	// first do translation (distance from center)
	static float oldCameraDistance;
	static float cameraDistance;
	static float targetCameraDistance = -1000.0f;
	static float preCameraInterp = PI;
	float cameraInterp;

	preCameraInterp += float (dCameraspeed) * elapsedTime * 0.01f;

	cameraInterp = 0.5f - (0.5f * cos (preCameraInterp));
	cameraDistance = (1.0f - cameraInterp) * oldCameraDistance + cameraInterp * targetCameraDistance;

	if (preCameraInterp >= PI) {
		oldCameraDistance = targetCameraDistance;
		targetCameraDistance = -rsRandf (1300.0f) - 200.0f;
		preCameraInterp = 0.0f;
	}

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
	glTranslatef (0.0, 0.0, cameraDistance);

	// then do rotation
	static rsVec radialVel = rsVec (0.0f, 0.0f, 0.0f);
	static rsVec targetRadialVel = radialVel;
	static rsQuat rotQuat = rsQuat (0.0f, 0.0f, 0.0f, 1.0f);

	rsVec radialVelDiff = targetRadialVel - radialVel;
	float changeRemaining = radialVelDiff.normalize ();
	float change = float (dCameraspeed) * 0.0002f * elapsedTime;

	if (changeRemaining > change) {
		radialVelDiff *= change;
		radialVel += radialVelDiff;
	} else {
		radialVel = targetRadialVel;
		if (rsRandi (2)) {
			targetRadialVel = rsVec (rsRandf (1.0f), rsRandf (1.0f), rsRandf (1.0f));
			targetRadialVel.normalize ();
			targetRadialVel *= float (dCameraspeed) * rsRandf (0.002f);
		} else
			targetRadialVel = rsVec (0.0f, 0.0f, 0.0f);
	}

	rsVec tempRadialVel = radialVel;
	float angle = tempRadialVel.normalize ();

	rsQuat radialQuat;

	radialQuat.make (angle, tempRadialVel[0], tempRadialVel[1], tempRadialVel[2]);
	rotQuat.preMult (radialQuat);
	rsMatrix rotMat;

	rotMat.fromQuat (rotQuat);

	// make billboard matrix for rotating particles when they are drawn
	rotMat.get (billboardMat);

	// Calculate new color
	static rsVec oldHsl, newHsl = rsVec (rsRandf (1.0f), 1.0f, 1.0f), targetHsl;
	static float colorInterp = 1.0f, colorChange;

	colorInterp += elapsedTime * colorChange;
	if (colorInterp >= 1.0f) {
		if (!rsRandi (3) && dIons >= 100)	// change color suddenly
			newHsl = rsVec (rsRandf (1.0f), 1.0f - (rsRandf (1.0f) * rsRandf (1.0f)), 1.0f);
		oldHsl = newHsl;
		targetHsl = rsVec (rsRandf (1.0f), 1.0f - (rsRandf (1.0f) * rsRandf (1.0f)), 1.0f);
		colorInterp = 0.0f;
		// amount by which to change colorInterp each second
		colorChange = rsRandf (0.005f * float (dSpeed)) + (0.002f * float (dSpeed));
	} else {
		float diff = targetHsl[0] - oldHsl[0];

		if (diff < -0.5f || (diff > 0.0f && diff < 0.5f))
			newHsl[0] = oldHsl[0] + colorInterp * diff;
		else
			newHsl[0] = oldHsl[0] - colorInterp * diff;
		diff = targetHsl[1] - oldHsl[1];
		newHsl[1] = oldHsl[1] + colorInterp * diff;
		if (newHsl[0] < 0.0f)
			newHsl[0] += 1.0f;
		if (newHsl[0] > 1.0f)
			newHsl[0] -= 1.0f;
		hsl2rgb (newHsl[0], newHsl[1], 1.0f, newRgb[0], newRgb[1], newRgb[2]);
	}

	// Release ions
	if (ionsReleased < dIons) {
		releaseTime -= elapsedTime;
		while (ionsReleased < dIons && releaseTime <= 0.0f) {
			ilist[ionsReleased].start ();
			ionsReleased++;
			// all ions released after 2 minutes
			releaseTime += 120.0f / float (dIons);
		}
	}
	// Set interpolation value for emitters and attracters
	static float wait = 0.0f;
	static float preinterp = PI, interp;
	static float interpconst = 0.001f;

	wait -= elapsedTime;
	if (wait <= 0.0f) {
		preinterp += elapsedTime * float (dSpeed) * interpconst;

		interp = 0.5f - (0.5f * cos (preinterp));
	}
	if (preinterp >= PI) {
		// select new taget points (not the same pattern twice in a row)
		static int newTarget = 0, lastTarget;

		lastTarget = newTarget;
		newTarget = rsRandi (10);
		if (newTarget == lastTarget)
			newTarget++;
		setTargets (newTarget);
		preinterp = 0.0f;
		interp = 0.0f;
		wait = 10.0f;	// pause after forming each new pattern
		interpconst = 0.001f;
		if (!rsRandi (4))	// interpolate really fast sometimes
			interpconst = 0.1f;
	}
	// Update particles
	for (i = 0; i < dEmitters; i++) {
		elist[i].interppos (interp);
		elist[i].update ();
	}
	for (i = 0; i < dAttracters; i++) {
		alist[i].interppos (interp);
		alist[i].update ();
	}
	for (i = 0; i < ionsReleased; i++)
		ilist[i].update ();

	// Calculate surface
	if (dSurface) {
		for (i = 0; i < dEmitters; i++)
			spheres[i].setPosition (elist[i].pos[0], elist[i].pos[1], elist[i].pos[2]);
		for (i = 0; i < dAttracters; i++)
			spheres[dEmitters + i].setPosition (alist[i].pos[0], alist[i].pos[1], alist[i].pos[2]);

		impCrawlPointVector cpv;
		for(i=0; i<dEmitters+dAttracters; i++)
			spheres[i].addCrawlPoint(cpv);

		surface->reset ();

		static float valuetrig = 0.0f;
		valuetrig += elapsedTime;

		volume->setSurfaceValue(0.45f + 0.05f * cosf(valuetrig));
		volume->makeSurface(cpv);
	}
	// Draw
	// clear the screen
	if (dBlur) {		// partially
		glMatrixMode (GL_PROJECTION);
		glPushMatrix ();
			glLoadIdentity();
			glOrtho(0.0, 1.0, 0.0, 1.0, 1.0, -1.0);
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
				glLoadIdentity();
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glColor4f(0.0f, 0.0f, 0.0f, 0.5f - (float(sqrtf(sqrtf(float(dBlur)))) * 0.15495f));
				glBegin(GL_TRIANGLE_STRIP);
					glVertex3f(0.0f, 0.0f, 0.0f);
					glVertex3f(1.0f, 0.0f, 0.0f);
					glVertex3f(0.0f, 1.0f, 0.0f);
					glVertex3f(1.0f, 1.0f, 0.0f);
			glEnd();
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	} else			// completely
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw ions
	glMatrixMode(GL_MODELVIEW);
	glBlendFunc (GL_ONE, GL_ONE);
	glBindTexture (GL_TEXTURE_2D, 1);
	for (i = 0; i < ionsReleased; i++)
		ilist[i].draw ();

	// Draw surfaces
	float brightFactor = 0;
	float surfaceColor[3] = {
		0.0f,
		0.0f,
		0.0f
	};

	if (dSurface) {
		glBindTexture (GL_TEXTURE_2D, 2);
		glEnable (GL_TEXTURE_GEN_S);
		glEnable (GL_TEXTURE_GEN_T);
		// find color for surfaces
		if (dIons >= 100) {
			if (dWireframe)
				brightFactor = 2.0f / (float (dBlur + 30) * float (dBlur + 30));
			else
				brightFactor = 4.0f / (float (dBlur + 30) * float (dBlur + 30));
			for (i = 0; i < 100; i++) {
				surfaceColor[0] += ilist[i].rgb[0] * brightFactor;
				surfaceColor[1] += ilist[i].rgb[1] * brightFactor;
				surfaceColor[2] += ilist[i].rgb[2] * brightFactor;
			}
			glColor3fv (surfaceColor);
		} else {
			if (dWireframe)
				brightFactor = 200.0f / (float (dBlur + 30) * float (dBlur + 30));
			else
				brightFactor = 400.0f / (float (dBlur + 30) * float (dBlur + 30));
			glColor3f (newRgb[0] * brightFactor, newRgb[1] * brightFactor, newRgb[2] * brightFactor);
		}
		// draw the surface
		glPushMatrix ();
		glMultMatrixf (billboardMat);
		if (dWireframe) {
			glDisable (GL_TEXTURE_2D);
			surface->draw_wireframe ();
			glEnable (GL_TEXTURE_2D);
		} else
			surface->draw ();
		glPopMatrix ();
		glDisable (GL_TEXTURE_GEN_S);
		glDisable (GL_TEXTURE_GEN_T);
	}
	// If graphics card does a true buffer swap instead of a copy swap
	// then everything must get drawn on both buffers
	if (dBlur & pfd_swap_exchange) {
		glXSwapBuffers (dpy, window);
		// wglSwapLayerBuffers(hdc, WGL_SWAP_MAIN_PLANE);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f (0.0f, 0.0f, 0.0f, 0.5f - (float (sqrt (sqrt (double (dBlur)))) * 0.15495f));

		glPushMatrix ();
		glLoadIdentity ();
		glBegin (GL_TRIANGLE_STRIP);
		glVertex3f (-5.0f, -4.0f, -3.0f);
		glVertex3f (5.0f, -4.0f, -3.0f);
		glVertex3f (-5.0f, 4.0f, -3.0f);
		glVertex3f (5.0f, 4.0f, -3.0f);
		glEnd ();
		glPopMatrix ();

		// Draw ions
		glBlendFunc (GL_ONE, GL_ONE);
		glBindTexture (GL_TEXTURE_2D, 1);
		for (i = 0; i < ionsReleased; i++)
			ilist[i].draw ();

		// Draw surfaces
		if (dSurface) {
			glBindTexture (GL_TEXTURE_2D, 2);
			glEnable (GL_TEXTURE_GEN_S);
			glEnable (GL_TEXTURE_GEN_T);
			if (dIons >= 100)
				glColor3fv (surfaceColor);
			else
				glColor3f (newRgb[0] * brightFactor, newRgb[1] * brightFactor, newRgb[2] * brightFactor);
			glPushMatrix ();
			glMultMatrixf (billboardMat);
			if (dWireframe) {
				glDisable (GL_TEXTURE_2D);
				surface->draw_wireframe ();
				glEnable (GL_TEXTURE_2D);
			} else
				surface->draw ();
			glPopMatrix ();
			glDisable (GL_TEXTURE_GEN_S);
			glDisable (GL_TEXTURE_GEN_T);
		}
	}
}

void hack_reshape (xstuff_t * XStuff)
{
	glViewport (0, 0, XStuff->windowWidth, XStuff->windowHeight);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	gluPerspective (60.0, float (XStuff->windowWidth) / float (XStuff->windowHeight), 0.1, 10000.0f);
}

void hack_init (xstuff_t * XStuff)
{
	int i, j;
	float x, y, temp;
	unsigned char *tex;

	// Window initialization
	hack_reshape (XStuff);

	glDisable (GL_DEPTH_TEST);
	glEnable (GL_BLEND);
	glLightModeli (GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

	// Clear the buffers and test for type of buffer swapping
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glXSwapBuffers (XStuff->display, XStuff->window);	// wglSwapLayerBuffers(hdc, WGL_SWAP_MAIN_PLANE);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	unsigned char pixel[1] = { 255 };

	glRasterPos2i (0, 0);
	glDrawPixels (1, 1, GL_RED, GL_UNSIGNED_BYTE, pixel);
	glXSwapBuffers (XStuff->display, XStuff->window);	// wglSwapLayerBuffers(hdc, WGL_SWAP_MAIN_PLANE);
	glReadPixels (0, 0, 1, 1, GL_RED, GL_UNSIGNED_BYTE, pixel);

	if (pixel[0] == 0) {	// Color was swapped out of the back buffer
		pfd_swap_exchange = 1;
	} else {		// Color remains in back buffer
		pfd_swap_exchange = 0;
	}

	// Init light texture
	for (i = 0; i < LIGHTSIZE; i++) {
		for (j = 0; j < LIGHTSIZE; j++) {
			x = float (i - LIGHTSIZE / 2) / float (LIGHTSIZE / 2);
			y = float (j - LIGHTSIZE / 2) / float (LIGHTSIZE / 2);
			temp = 1.0f - float (sqrt ((x * x) + (y * y)));

			if (temp > 1.0f)
				temp = 1.0f;
			if (temp < 0.0f)
				temp = 0.0f;
			lightTexture[i][j] = char (255.0f * temp * temp);
		}
	}

	glBindTexture (GL_TEXTURE_2D, 1);
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	gluBuild2DMipmaps (GL_TEXTURE_2D, 1, LIGHTSIZE, LIGHTSIZE, GL_LUMINANCE, GL_UNSIGNED_BYTE, lightTexture);
	glBindTexture (GL_TEXTURE_2D, 2);
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	LOAD_TEXTURE (tex, spheremap, spheremap_compressedsize, spheremap_size)
		gluBuild2DMipmaps (GL_TEXTURE_2D, 3, TEXSIZE, TEXSIZE, GL_RGB, GL_UNSIGNED_BYTE, tex);
	FREE_TEXTURE (tex)

		glTexGeni (GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	glTexGeni (GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	glEnable (GL_TEXTURE_2D);

	// Initialize light display list
	glNewList (1, GL_COMPILE);
	glBindTexture (GL_TEXTURE_2D, 1);
	glBegin (GL_TRIANGLES);
	glTexCoord2f (0.0f, 0.0f);
	glVertex3f (-0.5f, -0.5f, 0.0f);
	glTexCoord2f (1.0f, 0.0f);
	glVertex3f (0.5f, -0.5f, 0.0f);
	glTexCoord2f (1.0f, 1.0f);
	glVertex3f (0.5f, 0.5f, 0.0f);
	glTexCoord2f (0.0f, 0.0f);
	glVertex3f (-0.5f, -0.5f, 0.0f);
	glTexCoord2f (1.0f, 1.0f);
	glVertex3f (0.5f, 0.5f, 0.0f);
	glTexCoord2f (0.0f, 1.0f);
	glVertex3f (-0.5f, 0.5f, 0.0f);
	glEnd ();
	glEndList ();

	// Initialize particles
	elist = new emitter[dEmitters];
	alist = new attracter[dAttracters];
	ilist = new ion[dIons];

	// Initialize surface
	if (dSurface) {
		volume = new impCubeVolume;
		volume->init (50, 50, 50, 35.0f);
		volume->function = surfaceFunction;
		surface = volume->getSurface();

		spheres = new impSphere[dEmitters + dAttracters];

		float sphereScaleFactor = 1.0f / sqrtf (double (2 * dEmitters + dAttracters));
		for (i = 0; i < dEmitters; i++)
			spheres[i].setThickness (400.0f * sphereScaleFactor);
		for (i = 0; i < dAttracters; i++)
			spheres[i + dEmitters].setThickness (200.0f * sphereScaleFactor);
	}
}

void hack_cleanup (xstuff_t * XStuff)
{
	glDeleteLists (1, 1);
}

void hack_handle_opts (int argc, char **argv)
{
	dIons = 1500;
	dSize = 10;
	dEmitters = 3;
	dAttracters = 3;
	dSpeed = 10;
	dCameraspeed = 10;
	dSurface = 1;
	dWireframe = 0;
	dBlur = 10;

	while (1) {
		int c;

#ifdef HAVE_GETOPT_H
		static struct option long_options[] = {
			{"help", 0, 0, 'h'},
			DRIVER_OPTIONS_LONG {"ions", 1, 0, 'i'},
			{"size", 1, 0, 's'},
			{"emitters", 1, 0, 'e'},
			{"attracters", 1, 0, 'a'},
			{"speed", 1, 0, 'S'},
			{"cameraspeed", 1, 0, 'c'},
			{"surface", 0, 0, 'u'},
			{"no-surface", 0, 0, 'U'},
			{"blur", 1, 0, 'b'},
			{"wireframe", 0, 0, 'w'},
			{"no-wireframe", 0, 0, 'W'},
			{0, 0, 0, 0}
		};

		c = getopt_long (argc, argv, DRIVER_OPTIONS_SHORT "hi:s:e:a:S:c:uUb:wW", long_options, NULL);
#else
		c = getopt (argc, argv, DRIVER_OPTIONS_SHORT "hi:s:e:a:S:c:uUb:wW");
#endif
		if (c == -1)
			break;

		switch (c) {
			DRIVER_OPTIONS_CASES case 'h':printf ("%s:"
#ifndef HAVE_GETOPT_H
							      " Not built with GNU getopt.h, long options *NOT* enabled."
#endif
							      "\n" DRIVER_OPTIONS_HELP "\t--ions/-i <arg>\n" "\t--size/-s <arg>\n" "\t--emitters/-e <arg>\n"
							      "\t--attracters/-a <arg>\n" "\t--speed/-S <arg>\n" "\t--cameraspeed/-c <arg>\n" "\t--surface/-u\n"
							      "\t--no-surface/-U\n" "\t--blur/-b <arg>\n" "\t--wireframe/-w\n" "\t--no-wireframe/-W\n", argv[0]);
			exit (1);
		case 'i':
			dIons = strtol_minmaxdef (optarg, 10, 0, 30000, 1, 1500, "--ions: ");
			break;
		case 's':
			dSize = strtol_minmaxdef (optarg, 10, 1, 100, 1, 10, "--size: ");
			break;
		case 'e':
			dEmitters = strtol_minmaxdef (optarg, 10, 1, 10, 1, 3, "--emitters: ");
			break;
		case 'a':
			dAttracters = strtol_minmaxdef (optarg, 10, 1, 10, 1, 3, "--attracters: ");
			break;
		case 'S':
			dSpeed = strtol_minmaxdef (optarg, 10, 1, 100, 1, 10, "--speed: ");
			break;
		case 'c':
			dCameraspeed = strtol_minmaxdef (optarg, 10, 0, 100, 1, 10, "--cameraspeed: ");
			break;
		case 'u':
			dSurface = 1;
			break;
		case 'U':
			dSurface = 0;
			break;
		case 'b':
			dBlur = strtol_minmaxdef (optarg, 10, 0, 100, 1, 10, "--blur: ");
			break;
		case 'w':
			dWireframe = 1;
			break;
		case 'W':
			dWireframe = 0;
			break;
		}
	}
}
