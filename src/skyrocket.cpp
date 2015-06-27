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

// Skyrocket screen saver

#include <math.h>
#include <stdio.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <list>

#include "driver.h"
#include "rsDefines.h"
#include "rsRand.h"
#include "rsMath/rsVec.h"
#include "skyrocket_flare.h"
#include "skyrocket_particle.h"
#include "skyrocket_smoke.h"
#include "skyrocket_sound.h"
#include "skyrocket_shockwave.h"
#include "skyrocket_world.h"

const char *hack_name = "skyrocket";

double modelMat[16], projMat[16];
int viewport[4];
extern double billboardMat[16];

// Global variables
int readyToDraw = 0;

// list of particles
std::list < particle > particles;
// Time from one frame to the next
float elapsedTime = 0.0f;

// Window variables
int xsize, ysize, centerx, centery;
float aspectRatio;

// Camera variables
rsVec lookFrom[3];	// 3 = position, target position, last position
rsVec lookAt[3]		// 3 = position, target position, last position
	= { rsVec (0.0f, 1200.0f, 0.0f),
	rsVec (0.0f, 1200.0f, 0.0f),
	rsVec (0.0f, 1200.0f, 0.0f)
};
rsVec cameraPos;		// used for positioning sounds (same as lookFrom[0])
rsVec cameraVel;		// used for doppler shift

// Mouse variables
float mouseIdleTime;
int mouseButtons, mousex, mousey;
float mouseSpeed;

int numRockets = 0;

#define MAXFLARES 110		// Required 100 and 10 extra for good measure
float allflares[MAXFLARES][7];	// type, x, y, r, g, b, alpha
int numFlares = 0;

// Parameters edited in the dialog box
int dMaxrockets;
int dSmoke;
int dExplosionsmoke;
int dWind;
int dAmbient;
int dStardensity;
int dFlare;
int dMoonglow;
int dMoon;
int dClouds;
int dEarth;
int dIllumination;
int dSound;
int dPriority;

// Commands given from keyboard
int kAction = 1;
int kCamera = 1;		// 0 = paused, 1 = autonomous, 2 = mouse control
int kNewCamera = 0;
int kSlowMotion = 0;
int userDefinedExplosion = -1;

particle *addParticle ()
{
	static particle *tempPart;

	particles.push_back (particle ());
	tempPart = &(particles.back ());
	tempPart->depth = 10000;

	return tempPart;
}

// Makes list of lens flares.  Must be a called even when action is paused
// because camera might still be moving.
void makeFlareList ()
{
	rsVec cameraDir, partDir;

	cameraDir = lookAt[0] - lookFrom[0];
	cameraDir.normalize ();
	std::list < particle >::iterator curlight = particles.begin ();
	while (curlight != particles.end () && numFlares < MAXFLARES) {
		if (curlight->type == EXPLOSION) {
			double winx, winy, winz;

			gluProject (curlight->xyz[0], curlight->xyz[1], curlight->xyz[2], modelMat, projMat, viewport, &winx, &winy, &winz);
			partDir = curlight->xyz - cameraPos;
			if (partDir.dot (cameraDir) > 1.0f) {	// is light source in front of camera?
				allflares[numFlares][0] = 0;
				allflares[numFlares][1] = (float (winx) / float (xsize))*aspectRatio;
				allflares[numFlares][2] = float (winy) / float (ysize);

				allflares[numFlares][3] = curlight->rgb[0];
				allflares[numFlares][4] = curlight->rgb[1];
				allflares[numFlares][5] = curlight->rgb[2];
				rsVec vector = curlight->xyz - cameraPos;	// find distance attenuation factor
				float distatten = (10000.0f - vector.length ()) * 0.0001f;

				if (distatten < 0.0f)
					distatten = 0.0f;
				allflares[numFlares][6] = curlight->bright * float (dFlare) * 0.01f * distatten;

				numFlares++;
			}
		}
		if (curlight->type == SUCKER || curlight->type == SHOCKWAVE || curlight->type == STRETCHER || curlight->type == BIGMAMA) {
			double winx, winy, winz;
			float temp;

			gluProject (curlight->xyz[0], curlight->xyz[1], curlight->xyz[2], modelMat, projMat, viewport, &winx, &winy, &winz);
			partDir = curlight->xyz - cameraPos;
			if (partDir.dot (cameraDir) > 1.0f) {	// is light source in front of camera?
				allflares[numFlares][0] = 1;	// superFlare
				allflares[numFlares][1] = (float (winx) / float (xsize))*aspectRatio;
				allflares[numFlares][2] = float (winy) / float (ysize);

				allflares[numFlares][3] = curlight->rgb[0];
				allflares[numFlares][4] = curlight->rgb[1];
				allflares[numFlares][5] = curlight->rgb[2];
				rsVec vector = curlight->xyz - cameraPos;	// find distance attenuation factor
				float distatten = (20000.0f - vector.length ()) * 0.00005f;

				if (distatten < 0.0f)
					distatten = 0.0f;
				temp = 1.0f - (curlight->bright - 0.5f) * float (dFlare) * 0.02f;

				allflares[numFlares][6] = (1.0f - temp * temp * temp * temp) * distatten;
				numFlares++;
			}
		}
		curlight++;
	}
}

void draw ()
{
	int i, j;
	static int lastCameraMode = kCamera;
	// time, elapsed time, step (1.0 - 0.0)
	static float cameraTime[3] = { 20.0f, 0.0f, 0.0f };

	// super fast easter egg
	if (!rsRandi (2000))
		elapsedTime *= 5.0f;

	// build viewing matrix
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	gluPerspective (60.0f, aspectRatio, 1.0f, 40000.0f);
	glGetDoublev (GL_PROJECTION_MATRIX, projMat);

	if (kNewCamera) {	// Make new random camera view
		cameraTime[0] = rsRandf (25.0f) + 5.0f;
		cameraTime[1] = 0.0f;
		cameraTime[2] = 0.0f;
		// choose new positions
		lookFrom[1][0] = rsRandf (6000.0f) - 3000.0f;	// new target position
		lookFrom[1][1] = rsRandf (1700.0f) + 5.0f;
		lookFrom[1][2] = rsRandf (6000.0f) - 3000.0f;
		lookAt[1][0] = rsRandf (1000.0f) - 500.0f;	// new target position
		lookAt[1][1] = rsRandf (1100.0f) + 200.0f;
		lookAt[1][2] = rsRandf (1000.0f) - 500.0f;
		// cut to a new view
		lookFrom[2][0] = rsRandf (6000.0f) - 3000.0f;	// new last position
		lookFrom[2][1] = rsRandf (1700.0f) + 5.0f;
		lookFrom[2][2] = rsRandf (6000.0f) - 3000.0f;
		lookAt[2][0] = rsRandf (1000.0f) - 500.0f;	// new last position
		lookAt[2][1] = rsRandf (1100.0f) + 200.0f;
		lookAt[2][2] = rsRandf (1000.0f) - 500.0f;
		kNewCamera = 0;
	}

	if (kCamera == 1) {	// if the camera is active
		if (lastCameraMode == 2) {	// camera was controlled by mouse last frame
			cameraTime[0] = 10.0f;
			cameraTime[1] = 0.0f;
			cameraTime[2] = 0.0f;
			lookFrom[2] = lookFrom[0];
			lookFrom[1][0] = rsRandf (6000.0f) - 3000.0f;	// new target position
			lookFrom[1][1] = rsRandf (1700.0f) + 5.0f;
			lookFrom[1][2] = rsRandf (6000.0f) - 3000.0f;
			lookAt[2] = lookAt[0];
			lookAt[1][0] = rsRandf (1000.0f) - 500.0f;	// new target position
			lookAt[1][1] = rsRandf (1100.0f) + 200.0f;
			lookAt[1][2] = rsRandf (1000.0f) - 500.0f;
		}

		cameraTime[1] += elapsedTime;
		cameraTime[2] = cameraTime[1] / cameraTime[0];
		if (cameraTime[2] >= 1.0f) {	// reset camera sequence
			// reset timer
			cameraTime[0] = rsRandf (25.0f) + 5.0f;
			cameraTime[1] = 0.0f;
			cameraTime[2] = 0.0f;
			// choose new positions
			lookFrom[2] = lookFrom[1];	// last = target
			lookFrom[1][0] = rsRandf (6000.0f) - 3000.0f;	// new target position
			lookFrom[1][1] = rsRandf (1700.0f) + 5.0f;
			lookFrom[1][2] = rsRandf (6000.0f) - 3000.0f;
			lookAt[2] = lookAt[1];	// last = target
			lookAt[1][0] = rsRandf (1000.0f) - 500.0f;	// new target position
			lookAt[1][1] = rsRandf (1100.0f) + 200.0f;
			lookAt[1][2] = rsRandf (1000.0f) - 500.0f;
			if (!rsRandi (3)) {	// cut to a new view
				lookFrom[2][0] = rsRandf (6000.0f) - 3000.0f;	// new last position
				lookFrom[2][1] = rsRandf (1700.0f) + 5.0f;
				lookFrom[2][2] = rsRandf (6000.0f) - 3000.0f;
				lookAt[2][0] = rsRandf (1000.0f) - 500.0f;	// new last position
				lookAt[2][1] = rsRandf (1100.0f) + 200.0f;
				lookAt[2][2] = rsRandf (1000.0f) - 500.0f;
			}
		}

		// change camera position and angle
		float cameraStep = 0.5f * (1.0f - cos (cameraTime[2] * PI));

		lookFrom[0] = lookFrom[2] + ((lookFrom[1] - lookFrom[2]) * cameraStep);
		lookAt[0] = lookAt[2] + ((lookAt[1] - lookAt[2]) * cameraStep);
		// update variables used for sound and lens flares
		cameraVel = lookFrom[0] - cameraPos;
		cameraPos = lookFrom[0];
	}

	// Build modelview matrix
	// Don't use gluLookAt() because it's easier to find the billboard matrix
	// if we know the heading and pitch
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
	static float heading, pitch;

	// Control camera with the mouse
	if (kCamera == 2) {
		heading += 100.0f * elapsedTime * aspectRatio * float (centerx - mousex) / float (xsize);
		pitch += 100.0f * elapsedTime * float (centery - mousey) / float (ysize);

		if (heading > 180.0f)
			heading -= 360.0f;
		if (heading < -180.0f)
			heading += 360.0f;
		if (pitch > 90.0f)
			pitch = 90.0f;
		if (pitch < -90.0f)
			pitch = -90.0f;
/*
		if(mouseButtons & MK_LBUTTON)
			mouseSpeed += 400.0f * elapsedTime;
		if(mouseButtons & MK_RBUTTON)
			mouseSpeed -= 400.0f * elapsedTime;
		if((mouseButtons & MK_MBUTTON) || ((mouseButtons & MK_LBUTTON) && (mouseButtons & MK_RBUTTON)))
			mouseSpeed = 0.0f;
		if(mouseSpeed > 4000.0f)
			mouseSpeed = 4000.0f;
		if(mouseSpeed < -4000.0f)
			mouseSpeed = -4000.0f;
*/
		float ch = cos (DEG2RAD * heading);
		float sh = sin (DEG2RAD * heading);
		float cp = cos (DEG2RAD * pitch);
		float sp = sin (DEG2RAD * pitch);

		lookFrom[0][0] -= mouseSpeed * sh * cp * elapsedTime;
		lookFrom[0][1] += mouseSpeed * sp * elapsedTime;
		lookFrom[0][2] -= mouseSpeed * ch * cp * elapsedTime;
		cameraPos = lookFrom[0];
		// Calculate new lookAt position so that lens flares will be computed correctly
		// and so that transition back to autonomous camera mode is smooth
		lookAt[0][0] = lookFrom[0][0] - 500.0f * sh * cp;
		lookAt[0][1] = lookFrom[0][1] + 500.0f * sp;
		lookAt[0][2] = lookFrom[0][2] - 500.0f * ch * cp;
	} else {
		float radius = sqrt ((lookAt[0][0] - lookFrom[0][0]) * (lookAt[0][0] - lookFrom[0][0])
				     + (lookAt[0][2] - lookFrom[0][2]) * (lookAt[0][2] - lookFrom[0][2]));

		pitch = RAD2DEG * atan2 (lookAt[0][1] - lookFrom[0][1], radius);
		heading = RAD2DEG * atan2 (lookFrom[0][0] - lookAt[0][0], lookFrom[0][2] - lookAt[0][2]);
	}

	glRotatef (-pitch, 1, 0, 0);
	glRotatef (-heading, 0, 1, 0);
	glTranslatef (-lookFrom[0][0], -lookFrom[0][1], -lookFrom[0][2]);
	// get modelview matrix for flares
	glGetDoublev (GL_MODELVIEW_MATRIX, modelMat);

	// store this frame's camera mode for next frame
	lastCameraMode = kCamera;

	// Update mouse idle time
	if (kCamera == 2) {
		mouseIdleTime += elapsedTime;
		if (mouseIdleTime > 300.0f)	// return to autonomous camera mode after 5 minutes
			kCamera = 1;
	}

	// update billboard rotation matrix for particles
	glPushMatrix ();
	glLoadIdentity ();
	glRotatef (heading, 0, 1, 0);
	glRotatef (pitch, 1, 0, 0);
	glGetDoublev (GL_MODELVIEW_MATRIX, billboardMat);
	glPopMatrix ();

	// clear the screen
	glClear (GL_COLOR_BUFFER_BIT);

	// Slows fireworks, but not camera
	if (kSlowMotion)
		elapsedTime *= 0.5f;

	// Pause the animation?
	if (kAction) {
		// update world
		updateWorld ();

		// darken smoke
		std::list < particle >::iterator darkener = particles.begin ();
		static float ambientlight = float (dAmbient) * 0.01f;

		while (darkener != particles.end ()) {
			if (darkener->type == SMOKE)
				darkener->rgb[0] = darkener->rgb[1] = darkener->rgb[2] = ambientlight;
			darkener++;
		}

		// Change rocket firing rate
		static float rocketTimer = 0.0f;
		static float rocketTimeConst = 10.0f / float (dMaxrockets);
		static float changeRocketTimeConst = 20.0f;

		changeRocketTimeConst -= elapsedTime;
		if (changeRocketTimeConst <= 0.0f) {
			float temp = rsRandf (4.0f);

			rocketTimeConst = (temp * temp) + (10.0f / float (dMaxrockets));
			changeRocketTimeConst = rsRandf (30.0f) + 10.0f;
		}

		// add new rocket to list
		rocketTimer -= elapsedTime;
		if ((rocketTimer <= 0.0f) || (userDefinedExplosion >= 0)) {
			if (numRockets < dMaxrockets) {
				particle *rock = addParticle ();

				if (rsRandi (30) || (userDefinedExplosion >= 0)) {	// Usually launch a rocket
					rock->initRocket ();
					if (userDefinedExplosion >= 0)
						rock->explosiontype = userDefinedExplosion;
					else {
						if (!rsRandi (3000)) {	// big one!
							if (rsRandi (2))
								rock->explosiontype = 19;	// sucker and shockwave
							else
								rock->explosiontype = 20;	// stretcher and bigmama
						} else {
							// Distribution of regular explosions
							if (rsRandi (2)) {	// 0 - 2 (all types of spheres)
								if (!rsRandi (10))
									rock->explosiontype = 2;
								else
									rock->explosiontype = rsRandi (2);
							} else {
								if (!rsRandi (3))	//  ring, double sphere, sphere and ring
									rock->explosiontype = rsRandi (3) + 3;
								else {
									if (rsRandi (2)) {	// 6, 7, 8, 9, 10, 11
										if (rsRandi (2))
											rock->explosiontype = rsRandi (2) + 6;
										else
											rock->explosiontype = rsRandi (4) + 8;
									} else {
										if (rsRandi (2))	// 12, 13, 14
											rock->explosiontype = rsRandi (3) + 12;
										else	// 15 - 18
											rock->explosiontype = rsRandi (4) + 15;
									}
								}
							}
						}
					}
					numRockets++;
				} else {	// sometimes make fountains and spinners instead of rockets
					rock->initFountain ();
					i = rsRandi (2);
					for (j = 0; j < i; j++) {
						rock = addParticle ();
						rock->initFountain ();
					}
				}
			}

			if (dMaxrockets)
				rocketTimer = rsRandf (rocketTimeConst);
			else
				rocketTimer = 60.0f;	// arbitrary number since no rockets ever fire

			if (userDefinedExplosion >= 0) {
				userDefinedExplosion = -1;
				rocketTimer = 20.0f;	// Wait 20 seconds after user launches a rocket before launching any more
			}
		}

		// update particles
		numRockets = 0;
		std::list < particle >::iterator curpart = particles.begin ();
		while (curpart != particles.end ()) {
			curpart->update ();

			if (curpart->type == ROCKET)
				numRockets++;

			curpart->findDepth ();
			if (curpart->life <= 0.0f || curpart->xyz[1] < 0.0f) {
				if (curpart->type == ROCKET) {	// become explosion
					if (curpart->xyz[1] <= 0.0f) {	// move above ground for explosion
						curpart->xyz[1] = 0.1f;
						curpart->vel[1] *= -0.7f;
					}
					if (curpart->explosiontype == 18)
						curpart->initSpinner ();
					else
						curpart->initExplosion ();
				} else if (curpart->type == POPPER) {
					switch (curpart->explosiontype) {
					case STAR:
						curpart->explosiontype = 100;
						curpart->initExplosion ();
						break;
					case STREAMER:
						curpart->explosiontype = 101;
						curpart->initExplosion ();
						break;
					case METEOR:
						curpart->explosiontype = 102;
						curpart->initExplosion ();
						break;
					case POPPER:
						curpart->type = STAR;
						curpart->rgb.set (1.0f, 0.8f, 0.6f);
						curpart->t = curpart->tr = 0.2f;
					}
				} else if (curpart->type == SUCKER) {
					curpart->initShockwave ();
				} else if (curpart->type == STRETCHER) {	// become big explosion
					curpart->initBigmama ();
				} else	// remove particles from list
					curpart = particles.erase(curpart)--;
			}

			curpart++;
		}
	} else {
		// Only sort particles if they're not being updated (the camera could still be moving)
		std::list < particle >::iterator curpart = particles.begin ();
		while (curpart != particles.end ()) {
			curpart->findDepth ();
			curpart++;
		}
	}

	// the world
	drawWorld ();

	// draw particles
	glEnable (GL_BLEND);
	std::list < particle >::iterator drawer = particles.begin ();
	while (drawer != particles.end ()) {
		drawer->draw ();
		drawer++;
	}

	// draw lens flares
	if (dFlare) {
		makeFlareList ();
		for (i = 0; i < numFlares; i++) {
			if (allflares[i][0] == 0)
				flare (allflares[i][1], allflares[i][2], allflares[i][3], allflares[i][4], allflares[i][5], allflares[i][6]);
			else
				superFlare (allflares[i][1], allflares[i][2], allflares[i][3], allflares[i][4], allflares[i][5], allflares[i][6]);
		}
		numFlares = 0;
	}

#ifdef HAVE_OPENAL
	// do sound stuff
	if (dSound) {
		float listenerOri[6];
		listenerOri[0] = float (-modelMat[2]);
		listenerOri[1] = float (-modelMat[6]);
		listenerOri[2] = float (-modelMat[10]);
		listenerOri[3] = float (modelMat[1]);
		listenerOri[4] = float (modelMat[5]);
		listenerOri[5] = float (modelMat[9]);

		updateSound (cameraPos.v, cameraVel.v, listenerOri);
	}
#endif

	glFlush ();
}

void hack_draw (xstuff_t * XStuff, double currentTime, float frameTime)
{
	static float times[5] = { 0.03f, 0.03f, 0.03f, 0.03f, 0.03f };
	static int timeindex = 0;
#ifdef BENCHMARK
	static int a = 1;
#endif

	if (frameTime > 0) {
		times[timeindex] = frameTime;
	} else {		// else use elapsedTime from last frame
		times[timeindex] = elapsedTime;
	}

#ifdef BENCHMARK
	times[timeindex] = 0.03f;
#endif

	// average last 5 frame times together
	elapsedTime = 0.2f * (times[0] + times[1] + times[2] + times[3] + times[4]);

	timeindex++;
	if (timeindex >= 5)
		timeindex = 0;

	if (elapsedTime > 0.0f)
		draw ();

#ifdef BENCHMARK
	if (a++ == 1000)
		exit(0);
#endif
}

void hack_reshape (xstuff_t * XStuff)
{
	xsize = XStuff->windowWidth;
	ysize = XStuff->windowHeight;

	centerx = xsize / 2;
	centery = ysize / 2;

	glViewport (0, 0, xsize, ysize);
	glGetIntegerv (GL_VIEWPORT, viewport);
	aspectRatio = float (xsize) / float (ysize);
}

void hack_init (xstuff_t * XStuff)
{
	hack_reshape (XStuff);

	// Set OpenGL state
	glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
	glDisable (GL_DEPTH_TEST);
	glEnable (GL_TEXTURE_2D);
	glFrontFace (GL_CCW);
	glEnable (GL_CULL_FACE);

	// Initialize data structures
	initFlares ();
	if (dSmoke)
		initSmoke ();
	initWorld ();
	initShockwave ();

	lookFrom[1] = rsVec (rsRandf (3000.0f) - 1500.0f, 400.0f, rsRandf (3000.0f) - 1500.0f);
	lookFrom[2] = rsVec (rsRandf (1000.0f) + 5000.0f, 5.0f, rsRandf (4000.0f) - 2000.0f);

#ifdef HAVE_OPENAL
	if (dSound)
		initSound ();
#endif
}

void hack_cleanup (xstuff_t * XStuff)
{
	cleanupFlares ();
	cleanupSmoke ();
	cleanupWorld ();
#ifdef HAVE_OPENAL
	cleanupSound ();
#endif
}

void hack_handle_opts (int argc, char **argv)
{
	dMaxrockets = 8;
	dSmoke = 5;
	dExplosionsmoke = 0;
	dWind = 20;
	dAmbient = 10;
	dStardensity = 20;
	dFlare = 20;
	dMoonglow = 20;
	dSound = 0;
	dMoon = 1;
	dClouds = 1;
	dEarth = 1;
	dIllumination = 1;

	while (1) {
		int c;

#ifdef HAVE_GETOPT_H
		static struct option long_options[] = {
			{"help", 0, 0, 'h'},
			DRIVER_OPTIONS_LONG {"maxrockets", 1, 0, 'm'},
			{"smoke", 1, 0, 's'},
			{"explosionsmoke", 1, 0, 'S'},
			{"wind", 1, 0, 'w'},
			{"ambient", 1, 0, 'a'},
			{"stardensity", 1, 0, 'd'},
			{"flare", 1, 0, 'f'},
			{"moonglow", 1, 0, 'g'},
			{"volume", 1, 0, 'v'},
			{"moon", 0, 0, 'o'},
			{"no-moon", 0, 0, 'O'},
			{"clouds", 0, 0, 'c'},
			{"no-clouds", 0, 0, 'C'},
			{"earth", 0, 0, 'e'},
			{"no-earth", 0, 0, 'E'},
			{"illumination", 0, 0, 'i'},
			{"no-illumination", 0, 0, 'I'},
			{0, 0, 0, 0}
		};

		c = getopt_long (argc, argv, DRIVER_OPTIONS_SHORT "hm:s:S:a:d:f:g:v:oOcCeEiI", long_options, NULL);
#else
		c = getopt (argc, argv, DRIVER_OPTIONS_SHORT "hm:s:S:a:d:f:g:v:oOcCeEiI");
#endif
		if (c == -1)
			break;

		switch (c) {
			DRIVER_OPTIONS_CASES case 'h':printf ("%s:"
#ifndef HAVE_GETOPT_H
							      " Not built with GNU getopt.h, long options *NOT* enabled."
#endif
							      "\n" DRIVER_OPTIONS_HELP "\t--maxrockets/-m <arg>\n" "\t--smoke/-s <arg>\n" "\t--explosionsmoke/-S <arg>\n"
							      "\t--wind/-w <arg>\n" "\t--ambient/-a <arg>\n" "\t--stardensity/-d <arg>\n" "\t--flare/-f <arg>\n"
							      "\t--moonglow/-g <arg>\n" "\t--volume/-v <arg>\n" "\t--moon/-o\n" "\t--no-moon/-O\n" "\t--clouds/-c\n"
							      "\t--no-clouds/-C\n" "\t--earth/-e\n" "\t--no-earth/-E\n" "\t--illumination/-i\n" "\t--no-illumination/-I\n",
							      argv[0]);
			exit (1);
		case 'm':
			dMaxrockets = strtol_minmaxdef (optarg, 10, 1, 100, 1, 8, "--maxrockets: ");
			break;
		case 's':
			dSmoke = strtol_minmaxdef (optarg, 10, 0, 60, 1, 5, "--smoke: ");
			break;
		case 'S':
			dExplosionsmoke = strtol_minmaxdef (optarg, 10, 0, 100, 1, 0, "--explosionsmoke: ");
			break;
		case 'w':
			dWind = strtol_minmaxdef (optarg, 10, 0, 100, 1, 20, "--wind: ");
			break;
		case 'a':
			dAmbient = strtol_minmaxdef (optarg, 10, 0, 100, 1, 10, "--ambient: ");
			break;
		case 'd':
			dStardensity = strtol_minmaxdef (optarg, 10, 0, 100, 1, 20, "--stardensity: ");
			break;
		case 'f':
			dFlare = strtol_minmaxdef (optarg, 10, 0, 100, 1, 20, "--flare: ");
			break;
		case 'g':
			dMoonglow = strtol_minmaxdef (optarg, 10, 0, 100, 1, 20, "--moonglow: ");
			break;
		case 'v':
			dSound = strtol_minmaxdef (optarg, 10, 0, 100, 1, 0, "--volume: ");
			break;
		case 'o':
			dMoon = 1;
			break;
		case 'O':
			dMoon = 0;
			break;
		case 'c':
			dClouds = 1;
			break;
		case 'C':
			dClouds = 0;
			break;
		case 'e':
			dEarth = 1;
			break;
		case 'E':
			dEarth = 0;
			break;
		case 'i':
			dIllumination = 1;
			break;
		case 'I':
			dIllumination = 0;
			break;
		}
	}
}

/*
LONG ScreenSaverProc(HWND hwnd, UINT msg, WPARAM wpm, LPARAM lpm){
	switch(msg){
	case WM_CREATE:
		readRegistry();
		initSaver(hwnd);
		switch(dPriority){
		case 0:
			SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
			break;
		case 1:
			SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
			break;
		case 2:
			SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
		}
		readyToDraw = 1;
		break;
	case WM_KEYDOWN:
		switch(int(wpm)){
		// pause the motion of the fireworks
		case 'a':
		case 'A':
			if(kAction)
				kAction = 0;
			else
				kAction = 1;
			if(kSlowMotion)
				kSlowMotion = 0;
			return(0);
		// pause the motion of the camera
		case 'c':
		case 'C':
			if(kCamera == 0)
				kCamera = 1;
			else{
				if(kCamera == 1)
					kCamera = 0;
				else{
					if(kCamera == 2)
						kCamera = 1;
				}
			}
			return(0);
		// toggle mouse camera control
		case 'm':
		case 'M':
			if(kCamera == 2)
				kCamera = 1;
			else{
				kCamera = 2;
				mouseSpeed = 0.0f;
				mouseIdleTime = 0.0f;
			}
			return(0);
		// new camera view
		case 'n':
		case 'N':
			kNewCamera = 1;
			return(0);
		// slow the motion of the fireworks
		case 's':
		case 'S':
			if(kSlowMotion)
				kSlowMotion = 0;
			else
				kSlowMotion = 1;
			if(!kAction)
				kAction = 1;
			return(0);
		// choose which type of rocket explosion
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			userDefinedExplosion = int(wpm) - 49;  // explosions 0 - 8
			return(0);
		case '0':
			userDefinedExplosion = 9;
			return(0);
		case 'q':
		case 'Q':
			userDefinedExplosion = 10;
			return(0);
		case 'w':
		case 'W':
			userDefinedExplosion = 11;
			return(0);
		case 'e':
		case 'E':
			userDefinedExplosion = 12;
			return(0);
		case 'r':
		case 'R':
			userDefinedExplosion = 13;
			return(0);
		case 't':
		case 'T':
			userDefinedExplosion = 14;
			return(0);
		case 'y':
		case 'Y':
			userDefinedExplosion = 15;
			return(0);
		case 'u':
		case 'U':
			userDefinedExplosion = 16;
			return(0);
		case 'i':
		case 'I':
			userDefinedExplosion = 17;
			return(0);
		case 'o':
		case 'O':
			userDefinedExplosion = 18;  // spinner
			return(0);
		case 'b':
		case 'B':
		case 'd':
		case 'D':
		case 'f':
		case 'F':
		case 'g':
		case 'G':
		case 'h':
		case 'H':
		case 'j':
		case 'J':
		case 'k':
		case 'K':
		case 'l':
		case 'L':
		case 'p':
		case 'P':
		case 'v':
		case 'V':
		case 'x':
		case 'X':
		case 'z':
		case 'Z':
			// These letters do nothing
			// Disabling them makes it harder to make mistakes
			return(0);
		}
		break;
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		if(kCamera == 2){
			mouseButtons = wpm;        // key flags 
			mousex = LOWORD(lpm);  // horizontal position of cursor 
			mousey = HIWORD(lpm);
			mouseIdleTime = 0.0f;
			return(0);
		}
		break;
	case WM_DESTROY:
		readyToDraw = 0;
		cleanup(hwnd);
		break;
	}
	return DefScreenSaverProc(hwnd, msg, wpm, lpm);
}
*/
