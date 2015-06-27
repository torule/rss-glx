/*
 * Copyright (C) 2002  Terence M. Welsh
 * Ported to Linux by Tugrul Galatali <tugrul@galatali.com>
 *
 * Flocks is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 *
 * Flocks is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Flocks screensaver

#include <math.h>
#include <stdio.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "driver.h"
#include "rgbhsl.h"
#include "rsDefines.h"
#include "rsRand.h"

const char *hack_name = "flocks";

class bug;

// Globals
float elapsedTime = 0.0f;
float aspectRatio;
int wide;
int high;
int deep;
bug *lBugs;
bug *fBugs;
float colorFade;

// Parameters edited in the dialog box
int dLeaders;
int dFollowers;
int dGeometry;
int dSize;
int dComplexity;
int dSpeed;
int dStretch;
int dColorfadespeed;
int dPriority;
int dChromatek;
int dConnections;
int dBlur;
int dTrail;
int dClear;
int dCircles;
int dRandomColors;

class bug {
      public:
	int type;		// 0 = leader 1 = follower
	float h, s, l;
	float r, g, b;
	float halfr, halfg, halfb;
	float x, y, z;
	float xSpeed, ySpeed, zSpeed, maxSpeed;
	float accel;
	int right, up, forward;
	int leader;
	float craziness;	// How prone to switching direction is this leader
	float nextChange;	// Time until this leader's next direction change
	int hcount;

	int skipTrail;
	int trailEndPtr;

	float *xtrail;
	float *ytrail;
	float *ztrail;

	float *rtrail;
	float *gtrail;
	float *btrail;

	float xdrift;
	float ydrift;
	float zdrift;

	  bug ();
	  virtual ~ bug ();
	void initTrail ();
	void initLeader ();
	void initFollower ();
	void update (bug * bugs);
	void render (bug * bugs) const;
};

bug::bug ()
{
	hcount = random();
}

bug::~bug ()
{
}

void bug::initTrail ()
{
	int i;

	trailEndPtr = 0;
	skipTrail = 0;

	xtrail = new float[dTrail];
	ytrail = new float[dTrail];
	ztrail = new float[dTrail];
	rtrail = new float[dTrail];
	gtrail = new float[dTrail];
	btrail = new float[dTrail];

	for (i = 0; i < dTrail; i++) {
		xtrail[i] = x;
		ytrail[i] = y;
		ztrail[i] = z;

		rtrail[i] = 0;
		gtrail[i] = 0;
		btrail[i] = 0;
	}
}

void bug::initLeader ()
{
	type = 0;
	h = rsRandf (1.0);
	s = 1.0f;
	l = 1.0f;
	x = rsRandf (float (wide * 2)) - float (wide);
	y = rsRandf (float (high * 2)) - float (high);
	z = rsRandf (float (wide * 2)) + float (wide * 2);

	if (dTrail) {
		bug::initTrail ();

		xdrift = (rsRandf (2) - 1);
		ydrift = (rsRandf (2) - 1);
		zdrift = (rsRandf (2) - 1);
	}

	right = up = forward = 1;
	xSpeed = ySpeed = zSpeed = 0.0f;
	maxSpeed = 8.0f * float (dSpeed);
	accel = 13.0f * float (dSpeed);

	craziness = rsRandf (4.0f) + 0.05f;
	nextChange = 1.0f;

	leader = -1;
}

void bug::initFollower ()
{
	type = 1;
	h = rsRandf (1.0);
	s = 1.0f;
	l = 1.0f;
	x = rsRandf (float (wide * 2)) - float (wide);
	y = rsRandf (float (high * 2)) - float (high);
	z = rsRandf (float (wide * 5)) + float (wide * 2);

	if (dTrail) {
		bug::initTrail ();
	}

	right = up = forward = 0;
	xSpeed = ySpeed = zSpeed = 0.0f;
	maxSpeed = (rsRandf (6.0f) + 4.0f) * float (dSpeed);
	accel = (rsRandf (4.0f) + 9.0f) * float (dSpeed);

	leader = 0;
}

void bug::update (bug * bugs)
{
	int i;

	if (!type) {		// leader
		nextChange -= elapsedTime;

		if (nextChange <= 0.0f) {
			if (rsRandi (2))
				right++;
			if (rsRandi (2))
				up++;
			if (rsRandi (2))
				forward++;

			if (right >= 2)
				right = 0;
			if (up >= 2)
				up = 0;
			if (forward >= 2)
				forward = 0;

			nextChange = rsRandf (craziness);
		}

		if (right)
			xSpeed += accel * elapsedTime;
		else
			xSpeed -= accel * elapsedTime;
		if (up)
			ySpeed += accel * elapsedTime;
		else
			ySpeed -= accel * elapsedTime;
		if (forward)
			zSpeed -= accel * elapsedTime;
		else
			zSpeed += accel * elapsedTime;

		if (x < float (-wide))
			right = 1;
		else if (x > float (wide))
			right = 0;
		if (y < float (-high))
			up = 1;
		else if (y > float (high))
			up = 0;
		if (z < float (-deep))
			forward = 0;
		else if (z > float (deep))
			forward = 1;

		// Even leaders change color from Chromatek 3D
		if (dChromatek) {
			h = 0.666667f * ((float (wide) - z)/float (wide + wide));
			if (h > 0.666667f)
				h = 0.666667f;
			if (h < 0.0f)
				h = 0.0f;
		}
	} else {		// follower
		if (!rsRandi (10)) {
			float oldDistance = 10000000.0f, newDistance;

			for (i = 0; i < dLeaders; i++) {
				newDistance = ((bugs[i].x - x) * (bugs[i].x - x)
					       + (bugs[i].y - y) * (bugs[i].y - y)
					       + (bugs[i].z - z) * (bugs[i].z - z));
				if (newDistance < oldDistance) {
					oldDistance = newDistance;
					leader = i;
				}
			}
		}

		if ((bugs[leader].x - x) > 0.0f)
			xSpeed += accel * elapsedTime;
		else
			xSpeed -= accel * elapsedTime;
		if ((bugs[leader].y - y) > 0.0f)
			ySpeed += accel * elapsedTime;
		else
			ySpeed -= accel * elapsedTime;
		if ((bugs[leader].z - z) > 0.0f)
			zSpeed += accel * elapsedTime;
		else
			zSpeed -= accel * elapsedTime;

		if (dChromatek) {
			h = 0.666667f * ((float (wide) - z)/float (wide + wide));
			if (h > 0.666667f)
				h = 0.666667f;
			if (h < 0.0f)
				h = 0.0f;
		} else {
			if (fabs (h - bugs[leader].h) < (colorFade * elapsedTime))
				h = bugs[leader].h;
			else {
				if (fabs (h - bugs[leader].h) < 0.5f) {
					if (h > bugs[leader].h)
						h -= colorFade * elapsedTime;
					else
						h += colorFade * elapsedTime;
				} else {
					if (h > bugs[leader].h)
						h += colorFade * elapsedTime;
					else
						h -= colorFade * elapsedTime;
					if (h > 1.0f)
						h -= 1.0f;
					if (h < 0.0f)
						h += 1.0f;
				}
			}
		}
	}

	if (xSpeed > maxSpeed)
		xSpeed = maxSpeed;
	else if (xSpeed < -maxSpeed)
		xSpeed = -maxSpeed;
	if (ySpeed > maxSpeed)
		ySpeed = maxSpeed;
	else if (ySpeed < -maxSpeed)
		ySpeed = -maxSpeed;
	if (zSpeed > maxSpeed)
		zSpeed = maxSpeed;
	else if (zSpeed < -maxSpeed)
		zSpeed = -maxSpeed;

	x += xSpeed * elapsedTime;
	y += ySpeed * elapsedTime;
	z += zSpeed * elapsedTime;

	++hcount;
	hcount = hcount % 360;

	hsl2rgb (h, s, l, r, g, b);
	halfr = r * 0.5f;
	halfg = g * 0.5f;
	halfb = b * 0.5f;

	if (dTrail) {
		float tr, tg, tb;
		hsl2rgb (h, s, l, tr, tg, tb);

		skipTrail++;

		xtrail[trailEndPtr] = x;
		ytrail[trailEndPtr] = y;
		ztrail[trailEndPtr] = z;
		rtrail[trailEndPtr] = tr;
		gtrail[trailEndPtr] = tg;
		btrail[trailEndPtr] = tb;

		trailEndPtr = (trailEndPtr + 1) % dTrail;
	}
}

void bug::render (bug * bugs) const
{
	int i;
	float scale[4] = { 0.0 };

	glColor3f (r, g, b);

	if (dGeometry) {	// Draw blobs
		glPushMatrix ();
		glTranslatef (x, y, z);

		if (dStretch) {
			scale[0] = xSpeed * 0.04f;
			scale[1] = ySpeed * 0.04f;
			scale[2] = zSpeed * 0.04f;
			scale[3] = scale[0] * scale[0] + scale[1] * scale[1] + scale[2] * scale[2];

			if (scale[3] > 0.0f) {
				scale[3] = float (sqrt (scale[3]));

				scale[0] /= scale[3];
				scale[1] /= scale[3];
				scale[2] /= scale[3];
			}

			scale[3] *= float (dStretch) * 0.05f;

			if (scale[3] < 1.0f)
				scale[3] = 1.0f;
			glRotatef (float (atan2 (-scale[0], -scale[2])) * RAD2DEG, 0.0f, 1.0f, 0.0f);
			glRotatef (float (asin (scale[1])) * RAD2DEG, 1.0f, 0.0f, 0.0f);

			glScalef (1.0f, 1.0f, scale[3]);
		}

		glCallList (1);
		glPopMatrix ();
	} else if (dCircles) {	// Draw circle
		if ((z > 100.0) && (z < 1000.0)) {
			float rr = r, gg = g, bb = b;

			glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable (GL_BLEND);
			glDisable (GL_DEPTH_TEST);

			if (dRandomColors) hsl2rgb (hcount / 360.0, 1.0, 1.0, rr, gg, bb);

			glColor4f (rr, gg, bb, 0.1);
			glBegin (GL_TRIANGLE_FAN);
			glVertex3f (x, y, 0.0);
			for (int ii = 0; ii <= 30; ++ii) {
				glVertex3f (x + cos(ii / 30.0 * 2 * M_PI) * z / 10.0, y + sin(ii / 30.0 * 2 * M_PI) * z / 10.0, 0.0);
			}
			glEnd ();

			if (dRandomColors) {
				hsl2rgb (fmod(hcount / 360.0 + 0.5, 1.0), 1.0, 1.0, rr, gg, bb);
			} else {
				hsl2rgb (fmod(h + 0.5, 1.0), 1.0, 1.0, rr, gg, bb);
			}

			glColor4f (rr, gg, bb, 0.5);
			glBegin (GL_LINE_STRIP);
			for (int ii = 0; ii <= 30; ++ii) {
				glVertex3f (x + cos(ii / 30.0 * 2 * M_PI) * z / 10.0, y + sin(ii / 30.0 * 2 * M_PI) * z / 10.0, 0.0);
			}
			glEnd ();

			glEnable (GL_DEPTH_TEST);
			glDisable (GL_BLEND);
		}
	} else {		// Draw dots
		if (dStretch) {
			glLineWidth (float (dSize) * float (700 - z) * 0.0002f);

			scale[0] *= float (dStretch);
			scale[1] *= float (dStretch);
			scale[2] *= float (dStretch);

			glBegin (GL_LINES);
			glVertex3f (x - scale[0], y - scale[1], z - scale[2]);
			glVertex3f (x + scale[0], y + scale[1], z + scale[2]);
			glEnd ();
		} else {
			glPointSize (float (dSize) * float (700 - z) * 0.001f);

			glBegin (GL_POINTS);
			glVertex3f (x, y, z);
			glEnd ();
		}
	}

	if (dConnections && type) {	// draw connections
		glLineWidth (1.0f);
		glBegin (GL_LINES);
		glColor3f (halfr, halfg, halfb);
		glVertex3f (x, y, z);
		glColor3f (bugs[leader].halfr, bugs[leader].halfg, bugs[leader].halfb);
		glVertex3f (bugs[leader].x, bugs[leader].y, bugs[leader].z);
		glEnd ();
	}

	if (dTrail) {
		glLineWidth (2.0f);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE);
		glEnable (GL_BLEND);
		glDisable (GL_DEPTH_TEST);

#define ELEMENT(x) x[(trailEndPtr + i) % dTrail]
		glBegin (GL_LINE_STRIP);
		for (i = 0; i < dTrail; i++) {
			glColor4f (ELEMENT (rtrail), ELEMENT (gtrail), ELEMENT (btrail), (float)i / dTrail);

			glVertex3f (ELEMENT (xtrail), ELEMENT (ytrail), ELEMENT (ztrail));
		}
		glEnd ();

		for (i = 0; i < dTrail; i++) {
			xtrail[i] += bugs[leader].xdrift;
			ytrail[i] += bugs[leader].ydrift;
			ztrail[i] += bugs[leader].zdrift;
		}

		glDisable (GL_BLEND);
		glEnable (GL_DEPTH_TEST);
	}
}

void copy_buffer()
{
  static GLint viewport[4];
  static GLfloat raster_pos[4];

  glGetIntegerv(GL_VIEWPORT, viewport);

  /* set source buffer */
  glReadBuffer(GL_FRONT);

  /* set projection matrix */
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity() ;
  gluOrtho2D(0, viewport[2], 0, viewport[3]);

  /* set modelview matrix */
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  /* save old raster position */
  glGetFloatv(GL_CURRENT_RASTER_POSITION, raster_pos);

  /* set raster position */
  glRasterPos4f(0.0, 0.0, 0.0, 1.0);

  /* copy buffer */
  glCopyPixels(0, 0, viewport[2], viewport[3], GL_COLOR);

  /* restore old raster position */
  glRasterPos4fv(raster_pos);

  /* restore old matrices */
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);

  /* restore source buffer */
  glReadBuffer(GL_BACK); 
} 


void hack_draw (xstuff_t * XStuff, double currentTime, float frameTime)
{
	int i;
	static float times[10] = { 0.03f, 0.03f, 0.03f, 0.03f, 0.03f,
		0.03f, 0.03f, 0.03f, 0.03f, 0.03f
	};
	static int timeindex = 0;

	if (frameTime > 0)
		times[timeindex] = frameTime;
	else
		times[timeindex] = elapsedTime;

	elapsedTime = 0.1f * (times[0] + times[1] + times[2] + times[3] + times[4] + times[5] + times[6] + times[7] + times[8] + times[9]);

	if (dBlur) {		// partially
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable (GL_BLEND);
		glDisable (GL_DEPTH_TEST);
		glColor4f (0.0f, 0.0f, 0.0f, 0.5f - (float (sqrt (sqrt (double (dBlur * 0.75)))) * 0.15495f));

		glBegin (GL_TRIANGLE_STRIP);
		glVertex3f (-1000.0f, -1000.0f, 0.0f);
		glVertex3f (1000.0f, -1000.0f, 0.0f);
		glVertex3f (-1000.0f, 1000.0f, 0.0f);
		glVertex3f (1000.0f, 1000.0f, 0.0f);
		glEnd ();

		glEnable (GL_DEPTH_TEST);
		glDisable (GL_BLEND);
		glClear (GL_DEPTH_BUFFER_BIT);
	} else if (dClear) {	// completely
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	} else {
		glMatrixMode (GL_PROJECTION);
		glPushMatrix ();
		glLoadIdentity ();

		glMatrixMode (GL_MODELVIEW);
		glPushMatrix ();
		glLoadIdentity ();

		glDisable (GL_DEPTH_TEST);

		glRasterPos2i (-1, -1);

		glReadBuffer (GL_FRONT); 
		glDrawBuffer (GL_BACK);
		glCopyPixels (0, 0, XStuff->windowWidth, XStuff->windowHeight, GL_COLOR);  

		glEnable (GL_DEPTH_TEST);

		glMatrixMode (GL_PROJECTION);
		glPopMatrix ();

		glMatrixMode (GL_MODELVIEW);
		glPopMatrix ();
	}

	// Update and draw leaders
	for (i = 0; i < dLeaders; i++)
		lBugs[i].update (lBugs);
	// Update and draw followers
	for (i = 0; i < dFollowers; i++)
		fBugs[i].update (lBugs);

	for (i = 0; i < dLeaders; i++)
		lBugs[i].render (lBugs);
	for (i = 0; i < dFollowers; i++)
		fBugs[i].render (lBugs);

	glFlush ();
}

void hack_reshape (xstuff_t * XStuff)
{
	// Window initialization
	aspectRatio = float (XStuff->windowWidth) / float (XStuff->windowHeight);

	glViewport (0, 0, XStuff->windowWidth, XStuff->windowHeight);

	// calculate boundaries
	if (XStuff->windowWidth > XStuff->windowHeight) {
		high = deep = 160;
		wide = high * XStuff->windowWidth / XStuff->windowHeight;
	} else {
		wide = deep = 160;
		high = wide * XStuff->windowHeight / XStuff->windowWidth;
	}

	glEnable (GL_DEPTH_TEST);
	glFrontFace (GL_CCW);
	glEnable (GL_CULL_FACE);
	glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
	glEnable (GL_LINE_SMOOTH);
	glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);

	if (dGeometry) {	// Setup lights and build blobs
		glEnable (GL_LIGHTING);
		glEnable (GL_LIGHT0);
		float ambient[4] = {
			0.25f,
			0.25f,
			0.25f,
			0.0f
		};
		float diffuse[4] = {
			1.0f,
			1.0f,
			1.0f,
			0.0f
		};
		float specular[4] = {
			1.0f,
			1.0f,
			1.0f,
			0.0f
		};
		float position[4] = {
			500.0f,
			500.0f,
			500.0f,
			0.0f
		};

		glLightfv (GL_LIGHT0, GL_AMBIENT, ambient);
		glLightfv (GL_LIGHT0, GL_DIFFUSE, diffuse);
		glLightfv (GL_LIGHT0, GL_SPECULAR, specular);
		glLightfv (GL_LIGHT0, GL_POSITION, position);
		glEnable (GL_COLOR_MATERIAL);
		glMaterialf (GL_FRONT, GL_SHININESS, 10.0f);
		glColorMaterial (GL_FRONT, GL_SPECULAR);
		glColor3f (0.7f, 0.7f, 0.7f);
		glColorMaterial (GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

		glNewList (1, GL_COMPILE);
		GLUquadricObj *qobj = gluNewQuadric ();
		gluSphere (qobj, float (dSize) * 0.5f, dComplexity + 2, dComplexity + 1);

		gluDeleteQuadric (qobj);
		glEndList ();
	} else {
		if (dStretch == 0){
			// make GL_POINTS round instead of square
			glEnable(GL_POINT_SMOOTH);
			glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
		}
	}

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	gluPerspective (50.0, aspectRatio, 0.1, 2000);

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
	glTranslatef (0.0, 0.0, -float (wide * 2));

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void hack_init (xstuff_t * XStuff)
{
	int i;

	hack_reshape (XStuff);

	lBugs = new bug[dLeaders];
	fBugs = new bug[dFollowers];
	for (i = 0; i < dLeaders; i++)
		lBugs[i].initLeader ();
	for (i = 0; i < dFollowers; i++)
		fBugs[i].initFollower ();

	if (dTrail) {
		glShadeModel (GL_SMOOTH);
	}

	colorFade = float (dColorfadespeed) * 0.01f;
}

void hack_cleanup (xstuff_t * XStuff)
{
	glDeleteLists (1, 1);
}

#define PRESET_NORMAL 1
#define PRESET_TRAILS 2
#define PRESET_BLURRED_TRAILS 3
#define PRESET_BLURRED_CONNECTIONS 4
#define PRESET_CIRCLES 5
#define PRESET_CIRCLES_RANDOM 6
void setDefaults (int preset)
{
	dClear = 1;
	dCircles = 0;
	dRandomColors = 0;

	switch (preset) {
	case PRESET_NORMAL:
		dLeaders = 4;
		dFollowers = 400;
		dGeometry = 1;
		dSize = 10;
		dComplexity = 1;
		dSpeed = 15;
		dStretch = 20;
		dColorfadespeed = 15;
		dChromatek = 0;
		dConnections = 0;
		dTrail = 0;
		dBlur = 0;

		break;
	case PRESET_TRAILS:
		dLeaders = 4;
		dFollowers = 400;
		dGeometry = 1;
		dSize = 10;
		dComplexity = 1;
		dSpeed = 15;
		dStretch = 1;
		dColorfadespeed = 15;
		dChromatek = 0;
		dConnections = 0;
		dTrail = 100;
		dBlur = 0;

		break;
	case PRESET_BLURRED_TRAILS:
		dLeaders = 4;
		dFollowers = 200;
		dGeometry = 0;
		dSize = 1;
		dComplexity = 1;
		dSpeed = 15;
		dStretch = 0;
		dColorfadespeed = 15;
		dChromatek = 0;
		dConnections = 0;
		dTrail = 100;
		dBlur = 100;

		break;
	case PRESET_BLURRED_CONNECTIONS:
		dLeaders = 8;
		dFollowers = 400;
		dGeometry = 1;
		dSize = 10;
		dComplexity = 1;
		dSpeed = 15;
		dStretch = 20;
		dColorfadespeed = 15;
		dChromatek = 0;
		dConnections = 1;
		dTrail = 0;
		dBlur = 100;

		break;
	case PRESET_CIRCLES:
		dLeaders = 8;
		dFollowers = 400;
		dGeometry = 0;
		dSize = 10;
		dComplexity = 1;
		dSpeed = 15;
		dStretch = 20;
		dColorfadespeed = 15;
		dChromatek = 0;
		dConnections = 0;
		dTrail = 0;
		dBlur = 0;
		dClear = 0;
		dCircles = 1;
		dRandomColors = 0;

		break;
	case PRESET_CIRCLES_RANDOM:
		dLeaders = 8;
		dFollowers = 400;
		dGeometry = 0;
		dSize = 10;
		dComplexity = 1;
		dSpeed = 15;
		dStretch = 20;
		dColorfadespeed = 15;
		dChromatek = 0;
		dConnections = 0;
		dTrail = 0;
		dBlur = 0;
		dClear = 0;
		dCircles = 1;
		dRandomColors = 1;

		break;
	}
}

void hack_handle_opts (int argc, char **argv)
{
	int change_flag = 0;

	setDefaults (PRESET_NORMAL);

	while (1) {
		int c;

#ifdef HAVE_GETOPT_H
		static struct option long_options[] = {
			{"help", 0, 0, 'h'},
			DRIVER_OPTIONS_LONG {"preset", 1, 0, 'p'},
			{"regular", 0, 0, 1},
			{"trails", 0, 0, 2},
			{"blurt", 0, 0, 3},
			{"blurc", 0, 0, 4},
			{"circles", 0, 0, 5},
			{"circlesrandom", 0, 0, 6},
			{"leaders", 1, 0, 'l'},
			{"followers", 1, 0, 'f'},
			{"geometry", 0, 0, 'g'},
			{"no-geometry", 0, 0, 'G'},
			{"size", 1, 0, 'i'},
			{"complexity", 1, 0, 'o'},
			{"speed", 1, 0, 's'},
			{"stretch", 1, 0, 'S'},
			{"colorfadespeed", 1, 0, 'O'},
			{"chromatek", 0, 0, 'k'},
			{"no-chromatek", 0, 0, 'K'},
			{"connections", 0, 0, 'c'},
			{"no-connections", 0, 0, 'C'},
			{"traillength", 1, 0, 't'},
			{"blur", 1, 0, 'b'},
			{0, 0, 0, 0}
		};

		c = getopt_long (argc, argv, DRIVER_OPTIONS_SHORT "hp:l:f:gGi:o:s:S:O:kKcCt:b:", long_options, NULL);
#else
		c = getopt (argc, argv, DRIVER_OPTIONS_SHORT "hp:l:f:gGi:o:s:S:O:kKcCt:b:");
#endif
		if (c == -1)
			break;

		switch (c) {
			DRIVER_OPTIONS_CASES case 'h':printf ("%s:"
#ifndef HAVE_GETOPT_H
							      " Not built with GNU getopt.h, long options *NOT* enabled."
#endif
							      "\n" DRIVER_OPTIONS_HELP "\t--preset/-p <arg>\n" "\t--regular\n" "\t--trails\n" "\t--blurt\n" "\t--blurc\n" "\t--circles\n" "\t--circlesrandom\n"
							      "\t--leaders/-l <arg>\n" "\t--followers/-f <arg>\n" "\t--geometry/-g\n" "\t--no-geometry/-G\n" "\t--size/-i <arg>\n" 
							      "\t--complexity/-o <arg>\n" "\t--speed/-s <arg>\n" "\t--stretch/-S <arg>\n" "\t--colorfadespeed/-O <arg>\n" 
							      "\t--chromatek/-k\n" "\t--no-chromatek/-K\n" "\t--connections/-c\n" "\t--no-connections/-C\n" "\t--traillength/-t <arg>\n" 
							      "\t--blur/-b <arg>\n" "\t--circle/-e\n", argv[0]);
			exit (1);
		case 'p':
			change_flag = 1;

			c = strtol_minmaxdef (optarg, 10, 1, 5, 1, 1, "--preset: ");

			setDefaults (c);

			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			change_flag = 1;
			setDefaults(c);
			break;
		case 'l':
			change_flag = 1;
			dLeaders = strtol_minmaxdef (optarg, 10, 1, 100, 1, 4, "--leaders: ");
			break;
		case 'f':
			change_flag = 1;
			dFollowers = strtol_minmaxdef (optarg, 10, 0, 10000, 1, 400, "--followers: ");
			break;
		case 'g':
			change_flag = 1;
			dGeometry = 1;
			break;
		case 'G':
			change_flag = 1;
			dGeometry = 0;
			break;
		case 'i':
			change_flag = 1;
			dSize = strtol_minmaxdef (optarg, 10, 1, 100, 1, 10, "--size: ");
			break;
		case 'o':
			change_flag = 1;
			dComplexity = strtol_minmaxdef (optarg, 10, 1, 10, 1, 1, "--complexity: ");
			break;
		case 's':
			change_flag = 1;
			dSpeed = strtol_minmaxdef (optarg, 10, 1, 100, 1, 15, "--speed: ");
			break;
		case 'S':
			change_flag = 1;
			dStretch = strtol_minmaxdef (optarg, 10, 0, 100, 1, 20, "--stretch: ");
			break;
		case 'O':
			change_flag = 1;
			dColorfadespeed = strtol_minmaxdef (optarg, 10, 0, 100, 1, 15, "--colorfadespeed: ");
			break;
		case 'k':
			change_flag = 1;
			dChromatek = 1;
			break;
		case 'K':
			change_flag = 1;
			dChromatek = 0;
			break;
		case 'c':
			change_flag = 1;
			dConnections = 1;
			break;
		case 'C':
			change_flag = 1;
			dConnections = 0;
			break;
		case 't':
			change_flag = 1;
			dTrail = strtol_minmaxdef (optarg, 10, 0, 250, 1, 0, "--traillength: ");
			break;
		case 'b':
			change_flag = 1;
			dBlur = strtol_minmaxdef (optarg, 10, 0, 100, 1, 0, "--blur: ");
			break;
		}
	}

	if (!change_flag) {
		setDefaults (rsRandi (6) + 1);
	}
}
