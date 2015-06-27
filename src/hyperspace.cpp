/*
 * Copyright (C) 2005  Terence M. Welsh
 * Ported to Linux by Tugrul Galatali <tugrul@galatali.com>
 *
 * This file is part of Hyperspace.
 *
 * Hyperspace is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 *
 * Hyperspace is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


// Hyperspace screensaver
// Terry Welsh
// Originally wrote this saver in 2001, but computers weren't fast
// enough to run it at a decent frame rate.


#include "config.h"


#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#ifdef HAVE_GLEW
#include <GL/glew.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>

#include "loadTexture.h"

#include "flare.h"
#include "causticTextures.h"
#include "wavyNormalCubeMaps.h"
#include "splinePath.h"
#include "tunnel.h"
#include "goo.h"
#include "stretchedParticle.h"
#include "starBurst.h"
#ifdef HAVE_GLEW
#include "shaders.h"
#endif
#include "rsMath/rsMath.h"

const char *hack_name = "hyperspace";

#define NEBULAMAPSIZE 256
extern unsigned char *nebulamap;
extern unsigned int nebulamap_size;
extern unsigned int nebulamap_compressedsize;

int readyToDraw = 0;
int xsize, ysize;
float aspectRatio;
// Parameters edited in the dialog box
int dSpeed;
int dStars;
int dStarSize;
int dResolution;
int dDepth;
int dFov;
#ifdef HAVE_GLEW
bool dShaders = true;
#endif

float unroll;
float billboardMat[16];
double modelMat[16];
double projMat[16];
int viewport[4];

float camPos[3] = {0.0f, 0.0f, 0.0f};
float depth;
int numAnimTexFrames = 20;
causticTextures* theCausticTextures;
wavyNormalCubeMaps* theWNCM; 
int whichTexture = 0;
splinePath* thePath;
tunnel* theTunnel;
goo* theGoo;
float shiftx, shiftz;
unsigned int speckletex, spheretex, nebulatex;
unsigned int goo_vp, goo_fp, tunnel_vp, tunnel_fp;

stretchedParticle** stars;
stretchedParticle* sunStar;
starBurst* theStarBurst;



float goo_c[4];  // goo constants
float goo_cp[4] = {0.0f, 1.0f, 2.0f, 3.0f};  // goo constants phase
float goo_cs[4];  // goo constants speed
float gooFunction(float* position){
	const float px(position[0] + shiftx);
	const float pz(position[2] + shiftz);
	const float camx(px - camPos[0]);
	const float camz(pz - camPos[2]);

	return
		// This first term defines upper and lower surfaces.
		position[1] * position[1] * 1.25f
		// These terms make the surfaces wavy.
		+ goo_c[0] * rsCosf(px - 2.71f * position[1])
		+ goo_c[1] * rsCosf(4.21f * position[1] + pz)
		+ goo_c[2] * rsCosf(1.91f * px - 1.67f * pz)
		+ goo_c[3] * rsCosf(1.53f * px + 1.11f * position[1] + 2.11f * pz)
		// The last term creates a bubble around the eyepoint so it doesn't
		// punch through the surface.
		- 0.1f / (camx * camx + position[1] * position[1] + camz * camz);
}


void hack_draw (xstuff_t * XStuff, double currentTime, float frameTime)
{
	int i;

	static int first = 1;
	if (first) {
		glDisable(GL_FOG);

		// Caustic textures can only be created after rendering context has been created
		// because they have to be drawn and then read back from the framebuffer.
		theCausticTextures = new causticTextures(XStuff, 8, numAnimTexFrames, 100, 256, 1.0f, 0.01f, 20.0f);

		glEnable(GL_FOG);

#ifdef HAVE_GLEW
		if (dShaders)
			theWNCM = new wavyNormalCubeMaps(numAnimTexFrames, 128);
#endif

		first = 0;
	}

	glMatrixMode(GL_MODELVIEW);

	// Camera movements
	static float camHeading[3] = {0.0f, 0.0f, 0.0f};  // current, target, and last
	static int changeCamHeading = 1;
	static float camHeadingChangeTime[2] = {20.0f, 0.0f};  // total, elapsed
	static float camRoll[3] = {0.0f, 0.0f, 0.0f};  // current, target, and last
	static int changeCamRoll = 1;
	static float camRollChangeTime[2] = {1.0f, 0.0f};  // total, elapsed
	camHeadingChangeTime[1] += frameTime;
	if(camHeadingChangeTime[1] >= camHeadingChangeTime[0]){  // Choose new direction
		camHeadingChangeTime[0] = rsRandf(15.0f) + 5.0f;
		camHeadingChangeTime[1] = 0.0f;
		camHeading[2] = camHeading[1];  // last = target
		if(changeCamHeading){
			// face forward most of the time
			if(rsRandi(6))
				camHeading[1] = 0.0f;
			// face backward the rest of the time
			else if(rsRandi(2))
				camHeading[1] = RS_PI;
			else
				camHeading[1] = -RS_PI;
			changeCamHeading = 0;
		}
		else
			changeCamHeading = 1;
	}
	float t = camHeadingChangeTime[1] / camHeadingChangeTime[0];
	t = 0.5f * (1.0f - cosf(RS_PI * t));
	camHeading[0] = camHeading[1] * t + camHeading[2] * (1.0f - t);
	camRollChangeTime[1] += frameTime;
	if(camRollChangeTime[1] >= camRollChangeTime[0]){  // Choose new roll angle
		camRollChangeTime[0] = rsRandf(5.0f) + 10.0f;
		camRollChangeTime[1] = 0.0f;
		camRoll[2] = camRoll[1];  // last = target
		if(changeCamRoll){
			camRoll[1] = rsRandf(RS_PIx2*2) - RS_PIx2;
			changeCamRoll = 0;
		}
		else
			changeCamRoll = 1;
	}
	t = camRollChangeTime[1] / camRollChangeTime[0];
	t = 0.5f * (1.0f - cosf(RS_PI * t));
	camRoll[0] = camRoll[1] * t + camRoll[2] * (1.0f - t);

	static float pathDir[3] = {0.0f, 0.0f, -1.0f};
	thePath->moveAlongPath(float(dSpeed) * frameTime * 0.03f);
	thePath->update(frameTime);
	thePath->getPoint(dDepth + 2, thePath->step, camPos);
	thePath->getBaseDirection(dDepth + 2, thePath->step, pathDir);
	float pathAngle = atan2f(-pathDir[0], -pathDir[2]);

	glLoadIdentity();
	glRotatef((pathAngle + camHeading[0]) * RS_RAD2DEG, 0, 1, 0);
	glRotatef(camRoll[0] * RS_RAD2DEG, 0, 0, 1);
	glGetFloatv(GL_MODELVIEW_MATRIX, billboardMat);
	glLoadIdentity();
	glRotatef(-camRoll[0] * RS_RAD2DEG, 0, 0, 1);
	glRotatef((-pathAngle - camHeading[0]) * RS_RAD2DEG, 0, 1, 0);
	glTranslatef(-camPos[0], -camPos[1], -camPos[2]);
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMat);
	unroll = camRoll[0] * RS_RAD2DEG;

	// calculate diagonal fov
	float diagFov = 0.5f * float(dFov) / RS_RAD2DEG;
	diagFov = tanf(diagFov);
	diagFov = sqrtf(diagFov * diagFov + (diagFov * aspectRatio * diagFov * aspectRatio));
	diagFov = 2.0f * atanf(diagFov);
	theGoo->update(camPos[0], camPos[2], pathAngle + camHeading[0], diagFov);

	// clear
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// draw stars
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, flaretex[0]);
	static float temppos[2];
	for(i=0; i<dStars; i++){
		temppos[0] = stars[i]->pos[0] - camPos[0];
		temppos[1] = stars[i]->pos[2] - camPos[2];
		if(temppos[0] > depth){
			stars[i]->pos[0] -= depth * 2.0f;
			stars[i]->lastPos[0] -= depth * 2.0f;
		}
		if(temppos[0] < -depth){
			stars[i]->pos[0] += depth * 2.0f;
			stars[i]->lastPos[0] += depth * 2.0f;
		}
		if(temppos[1] > depth){
			stars[i]->pos[2] -= depth * 2.0f;
			stars[i]->lastPos[2] -= depth * 2.0f;
		}
		if(temppos[1] < -depth){
			stars[i]->pos[2] += depth * 2.0f;
			stars[i]->lastPos[2] += depth * 2.0f;
		}
		stars[i]->draw(camPos);
	}

	// pick animated texture frame
	static float textureTime = 0.0f;
	textureTime += frameTime;
	// loop frames every 2 seconds
	const float texFrameTime(2.0f / float(numAnimTexFrames));
	while(textureTime > texFrameTime){
		textureTime -= texFrameTime;
		whichTexture ++;
	}
	while(whichTexture >= numAnimTexFrames)
		whichTexture -= numAnimTexFrames;

	// draw goo
	// calculate color
	static float goo_rgb_phase[3] = {-0.1f, -0.1f, -0.1f};
	static float goo_rgb_speed[3] = {rsRandf(0.02f) + 0.02f, rsRandf(0.02f) + 0.02f, rsRandf(0.02f) + 0.02f};
	float goo_rgb[4];
	for(i=0; i<3; i++){
		goo_rgb_phase[i] += goo_rgb_speed[i] * frameTime;
		if(goo_rgb_phase[i] >= RS_PIx2)
			goo_rgb_phase[i] -= RS_PIx2;
		//goo_rgb[i] = (sinf(goo_rgb_phase[i]) + 1.0f) * 0.5f;
		goo_rgb[i] = sinf(goo_rgb_phase[i]);
		if(goo_rgb[i] < 0.0f)
			goo_rgb[i] = 0.0f;
	}
#ifdef HAVE_GLEW
	// alpha component gets normalmap lerp value
	float lerp = textureTime / texFrameTime;
	// setup textures
	if (dShaders) {
		goo_rgb[3] = lerp;
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_TEXTURE_CUBE_MAP_ARB);
		glActiveTextureARB(GL_TEXTURE2_ARB);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, nebulatex);
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, theWNCM->texture[(whichTexture + 1) % numAnimTexFrames]);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, theWNCM->texture[whichTexture]);
		glBindProgramARB(GL_VERTEX_PROGRAM_ARB, goo_vp);
		glEnable(GL_VERTEX_PROGRAM_ARB);
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, goo_fp);
		glEnable(GL_FRAGMENT_PROGRAM_ARB);
	} else {
#endif
		goo_rgb[3] = 1.0f;
		glBindTexture(GL_TEXTURE_2D, nebulatex);
		glEnable(GL_TEXTURE_2D);
		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);
#ifdef HAVE_GLEW
	}
#endif
	// update goo function constants
	for(i=0; i<4; i++){
		goo_cp[i] += goo_cs[i] * frameTime;
		if(goo_cp[i] >= RS_PIx2)
			goo_cp[i] -= RS_PIx2;
		goo_c[i] = 0.25f * cosf(goo_cp[i]);
	}
	// draw it
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glEnable(GL_BLEND);
	glColor4fv(goo_rgb);
	theGoo->draw();
#ifdef HAVE_GLEW
	if (dShaders) {
		glDisable(GL_VERTEX_PROGRAM_ARB);
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
		glDisable(GL_TEXTURE_CUBE_MAP_ARB);
	} else {
#endif
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
#ifdef HAVE_GLEW
	}
#endif

	// update starburst
	static float starBurstTime = 300.0f;  // burst after 5 minutes
	starBurstTime -= frameTime;
	if(starBurstTime <= 0.0f){
		float pos[] = {camPos[0] + (pathDir[0] * depth * 0.5f) + rsRandf(depth * 0.5f) - depth * 0.25f,
			rsRandf(2.0f) - 1.0f,
			camPos[2] + (pathDir[2] * depth * 0.5f) + rsRandf(depth * 0.5f) - depth * 0.25f};
		theStarBurst->restart(pos);  // it won't actually restart unless it's ready to
		starBurstTime = rsRandf(540.0f) + 60.0f;  // burst again within 1-10 minutes
	}
#ifdef HAVE_GLEW
	if (dShaders)
		theStarBurst->draw(lerp);
	else
#endif
		theStarBurst->draw();

	// draw tunnel
	theTunnel->make(frameTime);
	glEnable(GL_TEXTURE_2D);
#ifdef HAVE_GLEW
	if (dShaders) {
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glBindTexture(GL_TEXTURE_2D, theCausticTextures->caustictex[(whichTexture + 1) % numAnimTexFrames]);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glBindTexture(GL_TEXTURE_2D, theCausticTextures->caustictex[whichTexture]);
		glBindProgramARB(GL_VERTEX_PROGRAM_ARB, tunnel_vp);
		glEnable(GL_VERTEX_PROGRAM_ARB);
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, tunnel_fp);
		glEnable(GL_FRAGMENT_PROGRAM_ARB);
		theTunnel->draw(lerp);
	} else {
#endif
		glBindTexture(GL_TEXTURE_2D, theCausticTextures->caustictex[whichTexture]);
		theTunnel->draw();
#ifdef HAVE_GLEW
	}
	if(dShaders){
		glDisable(GL_VERTEX_PROGRAM_ARB);
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
	}
#endif

	// draw sun with lens flare
	glDisable(GL_FOG);
	double flarepos[3] = {0.0f, 2.0f, 0.0f};
	glBindTexture(GL_TEXTURE_2D, flaretex[0]);
	sunStar->draw(camPos);
	float diff[3] = {flarepos[0] - camPos[0], flarepos[1] - camPos[1], flarepos[2] - camPos[2]};
	float alpha = 0.5f - 0.005f * sqrtf(diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2]);
	if(alpha > 0.0f)
		flare(flarepos, 1.0f, 1.0f, 1.0f, alpha);
	glEnable(GL_FOG);
}


void hack_reshape (xstuff_t * XStuff)
{
	aspectRatio = float(XStuff->windowWidth) / float(XStuff->windowHeight);

	// setup viewport
	viewport[0] = 0;
	viewport[1] = 0;
	viewport[2] = XStuff->windowWidth;
	viewport[3] = XStuff->windowHeight;

	glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

	// setup projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(float(dFov), aspectRatio, 0.001f, 200.0f);
	glGetDoublev(GL_PROJECTION_MATRIX, projMat);
	glMatrixMode(GL_MODELVIEW);
}


#ifdef HAVE_GLEW
bool initExtensions() 
{
	if (!glewInitialized)
		return false;

	const char *necessaryExtensions[4] = { "GL_ARB_multitexture", "GL_ARB_texture_cube_map", "GL_ARB_vertex_program", "GL_ARB_fragment_program" };

	for (int i = 0; i < 4; i++)
		if (GL_TRUE != glewGetExtension(necessaryExtensions[i]))
			return false;

	return true;
}
#endif


void hack_init (xstuff_t * XStuff)
{
	int i, j;

	rsSinCosInit();

	hack_reshape (XStuff);

#ifdef HAVE_GLEW
	// initialize extensions
	dShaders &= initExtensions();
#endif

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

	initFlares();

	thePath = new splinePath((dDepth * 2) + 6);

	theTunnel = new tunnel(thePath, 20);

	// To avoid popping, depth, which will be used for fogging, is set to
	// dDepth * goo grid size - size of one goo cubelet
	depth = float(dDepth) * 2.0f - 2.0f / float(dResolution);

	theGoo = new goo(dResolution, depth, gooFunction);
	for(i=0; i<4; i++)
		goo_cs[i] = 0.1f + rsRandf(0.4f);

	stars = new stretchedParticle*[dStars];
	for(i=0; i<dStars; i++){
		stars[i] = new stretchedParticle;
		stars[i]->radius = rsRandf(float(dStarSize) * 0.001f) + float(dStarSize) * 0.001f;
		stars[i]->color[0] = 0.5f + rsRandf(0.5f);
		stars[i]->color[1] = 0.5f + rsRandf(0.5f);
		stars[i]->color[2] = 0.5f + rsRandf(0.5f);
		stars[i]->color[rsRandi(3)] = 1.0f;
		stars[i]->pos[0] = rsRandf(2.0f * depth) - depth;
		stars[i]->pos[1] = rsRandf(4.0f) - 2.0f;
		stars[i]->pos[2] = rsRandf(2.0f * depth) - depth;
		stars[i]->fov = float(dFov);
	}

	sunStar = new stretchedParticle;
	sunStar->radius = float(dStarSize) * 0.004f;
	sunStar->pos[0] = 0.0f;
	sunStar->pos[1] = 2.0f;
	sunStar->pos[2] = 0.0f;
	sunStar->fov = float(dFov);

	theStarBurst = new starBurst;
	for(i=0; i<SB_NUM_STARS; i++)
		theStarBurst->stars[i]->radius = rsRandf(float(dStarSize) * 0.001f) + float(dStarSize) * 0.001f;

	unsigned char *nebulamapbuf;
	LOAD_TEXTURE (nebulamapbuf, nebulamap, nebulamap_compressedsize, nebulamap_size);

	glGenTextures(1, &nebulatex);
#ifdef HAVE_GLEW
	if(dShaders){
		numAnimTexFrames = 20;

		glGenProgramsARB(1, &goo_vp);
		glBindProgramARB(GL_VERTEX_PROGRAM_ARB, goo_vp);
		glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen((const char *)goo_vp_asm), goo_vp_asm);

		glGenProgramsARB(1, &goo_fp);
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, goo_fp);
		glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen((const char *)goo_fp_asm), goo_fp_asm);

		glGenProgramsARB(1, &tunnel_vp);
		glBindProgramARB(GL_VERTEX_PROGRAM_ARB, tunnel_vp);
		glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen((const char *)tunnel_vp_asm), tunnel_vp_asm);

		glGenProgramsARB(1, &tunnel_fp);
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, tunnel_fp);
		glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen((const char *)tunnel_fp_asm), tunnel_fp_asm);

		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, nebulatex);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		gluBuild2DMipmaps(GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB, 3, NEBULAMAPSIZE, NEBULAMAPSIZE, GL_RGB, GL_UNSIGNED_BYTE, nebulamapbuf);
		gluBuild2DMipmaps(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, 3, NEBULAMAPSIZE, NEBULAMAPSIZE, GL_RGB, GL_UNSIGNED_BYTE, nebulamapbuf);
		gluBuild2DMipmaps(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB, 3, NEBULAMAPSIZE, NEBULAMAPSIZE, GL_RGB, GL_UNSIGNED_BYTE, nebulamapbuf);
		gluBuild2DMipmaps(GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB, 3, NEBULAMAPSIZE, NEBULAMAPSIZE, GL_RGB, GL_UNSIGNED_BYTE, nebulamapbuf);
		gluBuild2DMipmaps(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB, 3, NEBULAMAPSIZE, NEBULAMAPSIZE, GL_RGB, GL_UNSIGNED_BYTE, nebulamapbuf);
		gluBuild2DMipmaps(GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB, 3, NEBULAMAPSIZE, NEBULAMAPSIZE, GL_RGB, GL_UNSIGNED_BYTE, nebulamapbuf);
	} else {
#endif
		unsigned char nebulamapbuf2[NEBULAMAPSIZE][NEBULAMAPSIZE][3];

		assert(sizeof(nebulamapbuf2) == nebulamap_size);
		memcpy (&nebulamapbuf2, nebulamapbuf, nebulamap_size);

		numAnimTexFrames = 60;
		float x, y, temp;
		const int halfsize(NEBULAMAPSIZE / 2);
		for(i=0; i<NEBULAMAPSIZE; ++i){
			for(j=0; j<NEBULAMAPSIZE; ++j){
				x = float(i - halfsize) / float(halfsize);
				y = float(j - halfsize) / float(halfsize);
				temp = (x * x) + (y * y);
				if(temp > 1.0f)
					temp = 1.0f;
				if(temp < 0.0f)
					temp = 0.0f;
				temp = temp * temp;
				temp = temp * temp;
				nebulamapbuf2[i][j][0] = (unsigned char)(float(nebulamapbuf2[i][j][0]) * temp);
				nebulamapbuf2[i][j][1] = (unsigned char)(float(nebulamapbuf2[i][j][1]) * temp);
				nebulamapbuf2[i][j][2] = (unsigned char)(float(nebulamapbuf2[i][j][2]) * temp);
			}
		}
		glBindTexture(GL_TEXTURE_2D, nebulatex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		gluBuild2DMipmaps(GL_TEXTURE_2D, 3, NEBULAMAPSIZE, NEBULAMAPSIZE, GL_RGB, GL_UNSIGNED_BYTE, nebulamapbuf2);
#ifdef HAVE_GLEW
	}
#endif

	FREE_TEXTURE (nebulamapbuf);

	glEnable(GL_FOG);
	float fog_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	glFogfv(GL_FOG_COLOR, fog_color);
	glFogf(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_START, depth * 0.7f);
	glFogf(GL_FOG_END, depth);
}


void hack_cleanup (xstuff_t * XStuff)
{
	// Free memory
	delete theGoo;
	delete theTunnel;
	delete thePath;
	delete theCausticTextures;
	delete theWNCM;
}


void setDefaults(){
	dSpeed = 10;
	dStars = 1000;
	dStarSize = 10;
	dResolution = 10;
	dDepth = 5;
	dFov = 50;
#ifdef HAVE_GLEW
	dShaders = true;
#endif
}


void hack_handle_opts (int argc, char **argv)
{
	setDefaults();

	while (1) {
		int c;

#ifdef HAVE_GETOPT_H
		static struct option long_options[] = {
			{"help", 0, 0, 'h'},
			DRIVER_OPTIONS_LONG 
			{"speed", 1, 0, 's'},
			{"stars", 1, 0, 'S'},
			{"starsize", 1, 0, 't'},
			{"resolution", 1, 0, 'R'},
			{"depth", 1, 0, 'd'},
			{"fov", 1, 0, 'f'},
			{"no-shader", 0, 0, 'H'},
			{0, 0, 0, 0}
		};

		c = getopt_long (argc, argv, DRIVER_OPTIONS_SHORT "hs:S:t:z:R:d:f:H", long_options, NULL);
#else
		c = getopt (argc, argv, DRIVER_OPTIONS_SHORT "hs:S:t:z:R:d:f:H");
#endif
		if (c == -1)
			break;

		switch (c) {
			DRIVER_OPTIONS_CASES case 'h':printf ("%s:"
#ifndef HAVE_GETOPT_H
							      " Not built with GNU getopt.h, long options *NOT* enabled."
#endif
							      "\n" DRIVER_OPTIONS_HELP "\t--speed/-s <arg>\n" "\t--stars/-S <arg>\n" "\t--starsize/-t <arg>\n"
							      "\t--resolution/-R <arg>\n" "\t--depth/-d <arg>\n" "\t--fov/-f <arg>\n" "\t--no-shader/-H\n", argv[0]);
			exit (1);
		case 's':
			dSpeed = strtol_minmaxdef (optarg, 10, 1, 100, 1, 10, "--speed: ");
			break;
		case 'S':
			dStars = strtol_minmaxdef (optarg, 10, 0, 10000, 1, 1000, "--stars: ");
			break;
		case 't':
			dStarSize = strtol_minmaxdef (optarg, 10, 1, 100, 1, 10, "--starsize: ");
			break;
		case 'R':
			dResolution = strtol_minmaxdef (optarg, 10, 4, 20, 1, 10, "--resolution: ");
			break;
		case 'd':
			dDepth = strtol_minmaxdef (optarg, 10, 1, 10, 1, 5, "--depth: ");
			break;
		case 'f':
			dFov = strtol_minmaxdef (optarg, 10, 10, 150, 1, 50, "--fov: ");
			break;
#ifdef HAVE_GLEW
		case 'H':
			dShaders = false;
			break;
#endif
		}
	}
}

