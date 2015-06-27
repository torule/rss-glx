/*
 * Copyright (C) 2001 Ryan M. Geiss <guava at geissworks dot com>
 * Ported to Linux by Tugrul Galatali <tugrul@galatali.com>
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

/*
 * drempels
 */

const char *hack_name = "drempels";

#include "driver.h"
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef HAVE_GLEW
#include <GL/glew.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>

#include "gpoly.h"
#include "TexMgr.h"

using namespace std;

static GLuint tex, ptex, ctex, uvtex, btex;

#ifdef HAVE_GLEW
static GLuint indirect_fp;

static const char *indirect_fp_asm = {
"!!ARBfp1.0\n"

"TEMP texuv, srctex;\n"

"TEX texuv, fragment.texcoord[0], texture[1], 2D;\n"

#ifdef WORDS_BIGENDIAN
"PARAM scale = { 1.0, 0.00390625, 1.0, 0.00390625 };\n"
#else
"PARAM scale = { 0.00390625, 1.0, 0.00390625, 1.0 };\n"
#endif
"MUL texuv, scale, texuv;\n"
"ADD texuv.r, texuv.r, texuv.g;\n"
"ADD texuv.g, texuv.b, texuv.a;\n"

"TEX srctex, texuv, texture[0], 2D;\n"
"MUL srctex, srctex, fragment.color;\n"
"MOV result.color, srctex;\n"

"END\n"
};

static bool dShader = true;
#endif // HAVE_GLEW

static unsigned int dCells = 16;
static unsigned int dCellResolution = 16;
static unsigned int dTexInterval = 10;
static float dTexFadeInterval = 1;
static unsigned int dGenTexSize = 256;

static unsigned int cells;
static unsigned int cellResolution;

static bool fadeComplete = false;
static uint32_t *fadeBuf = NULL;

static float fAnimTime = 0;

// hidden:
static float warp_factor = 0.22f;			// 0.05 = no warping, 0.25 = good warping, 0.75 = chaos
static float rotational_speed = 0.05f;
static float mode_focus_exponent = 2.9f;	// 1 = simple linear combo (very blendy) / 4 = fairly focused / 16 = 99% singular
static int tex_scale = 10;			// goes from 0 to 20; 10 is normal; 0 is down 4X; 20 is up 4X
static int speed_scale = 10;		// goes from 0 to 20; 10 is normal; 0 is down 8X; 20 is up 8X

static float anim_speed = 1.0f;
static float master_zoom = 1.0f;	// 2 = zoomed in 2x, 0.5 = zoomed OUT 2x.
static float mode_switch_speed_multiplier = 1.0f;		// 1 = normal speed, 2 = double speed, 0.5 = half speed
static int   motion_blur = 7;		// goes from 0 to 10


void RandomizeStartValues();
static float fRandStart1;
static float fRandStart2;
static float fRandStart3;
static float fRandStart4;
static float warp_w[4];
static float warp_uscale[4];
static float warp_vscale[4];
static float warp_phase[4];

static TexMgr texmgr;
static double lastTexChange = 0;

static td_cellcornerinfo *cell = NULL;
static unsigned short *buf = NULL;


void RandomizeStartValues()
{
	fRandStart1 = 6.28f * (random() % 4096) / 4096.0f;
	fRandStart2 = 6.28f * (random() % 4096) / 4096.0f;
	fRandStart3 = 6.28f * (random() % 4096) / 4096.0f;
	fRandStart4 = 6.28f * (random() % 4096) / 4096.0f;

	// randomomize rotational direction
	if (random()%2) rotational_speed *= -1.0f;

	// randomomize warping parameters
	for (int i=0; i<4; i++)
	{
		warp_w[i]      = 0.02f + 0.015f * (random() % 4096) / 4096.0f;
		warp_uscale[i] = 0.23f + 0.120f * (random() % 4096) / 4096.0f;
		warp_vscale[i] = 0.23f + 0.120f * (random() % 4096) / 4096.0f;
		warp_phase[i]  = 6.28f * (random() % 4096) / 4096.0f;
		if (random() % 2) warp_w[i] *= -1;
		if (random() % 2) warp_uscale[i] *= -1;
		if (random() % 2) warp_vscale[i] *= -1;
		if (random() % 2) warp_phase[i] *= -1;
	}
}


inline uint32_t rgbScale(const uint32_t c, const unsigned char scale)
{
	const uint32_t lsb = (((c & 0x00ff00ff) * scale) >> 8) & 0x00ff00ff;
	const uint32_t msb = (((c & 0xff00ff00) >> 8) * scale) & 0xff00ff00;

	return lsb | msb;
}

inline uint32_t rgbLerp(const uint32_t &c0, const uint32_t &c1, const unsigned char &scale)
{
	uint32_t sc0 = rgbScale(c0, 255 - scale);
	uint32_t sc1 = rgbScale(c1, scale);

	return sc0 + sc1;
}


void hack_draw (xstuff_t * XStuff, double currentTime, float frameTime)
{
	const unsigned int UVCELLSX = cells + 2;
	const unsigned int UVCELLSY = cells + 2;
	const unsigned int FXW = (UVCELLSX - 2) * cellResolution;
	const unsigned int FXH = (UVCELLSY - 2) * cellResolution;

	if (currentTime > lastTexChange + dTexInterval)
	{
		if (texmgr.getNext())
		{
			lastTexChange = currentTime;

			glBindTexture (GL_TEXTURE_2D, ctex);
			glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, texmgr.getCurW(), texmgr.getCurH(), 0, GL_RGBA, GL_UNSIGNED_BYTE, texmgr.getCurTex());

			if (texmgr.getPrevTex())
			{
				glBindTexture (GL_TEXTURE_2D, ptex);
				glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, texmgr.getPrevW(), texmgr.getPrevH(), 0, GL_RGBA, GL_UNSIGNED_BYTE, texmgr.getPrevTex());
			}

			fadeComplete = false;
		}
	}

	if (texmgr.getPrevTex()
		&& (currentTime < lastTexChange + dTexFadeInterval)
		// Can only fade if the window is large enough to render both textures...
		&& (texmgr.getPrevW() < XStuff->windowWidth)
		&& (texmgr.getPrevH() < XStuff->windowHeight)
		&& (texmgr.getCurW() < XStuff->windowWidth)
		&& (texmgr.getCurH() < XStuff->windowHeight)
		)
	{
		const double blend = (currentTime - lastTexChange) / dTexFadeInterval;

		glViewport (0, 0, texmgr.getPrevW(), texmgr.getPrevH());

		glEnable (GL_BLEND);

		glColor4f (1.0, 1.0, 1.0, 1.0);
		glBindTexture (GL_TEXTURE_2D, ptex);
		
		glBegin (GL_QUADS);
		glTexCoord2f (0.0, 0.0); glVertex2f (0.0, 0.0);
		glTexCoord2f (0.0, 1.0); glVertex2f (0.0, 1.0);
		glTexCoord2f (1.0, 1.0); glVertex2f (1.0, 1.0);
		glTexCoord2f (1.0, 0.0); glVertex2f (1.0, 0.0);
		glEnd ();

		glColor4f (1.0, 1.0, 1.0, blend);
		glBindTexture (GL_TEXTURE_2D, ctex);
		
		glBegin (GL_QUADS);
		glTexCoord2f (0.0, 0.0); glVertex2f (0.0, 0.0);
		glTexCoord2f (0.0, 1.0); glVertex2f (0.0, 1.0);
		glTexCoord2f (1.0, 1.0); glVertex2f (1.0, 1.0);
		glTexCoord2f (1.0, 0.0); glVertex2f (1.0, 0.0);
		glEnd ();

		glDisable (GL_BLEND);

		glBindTexture (GL_TEXTURE_2D, tex);

#ifdef HAVE_GLEW
		if (dShader)
		{
			glCopyTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, 0, 0, texmgr.getPrevW(), texmgr.getPrevH(), 0);
		}
		else
#endif
		{
			glReadPixels (0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, fadeBuf);
		}
	}
	else if (!fadeComplete)
	{
		fadeComplete = true;

#ifdef HAVE_GLEW
		if (dShader)
		{
			glBindTexture (GL_TEXTURE_2D, tex);
			glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, texmgr.getCurW(), texmgr.getCurH(), 0, GL_RGBA, GL_UNSIGNED_BYTE, texmgr.getCurTex());
		}
#endif
	}

	{
		float fmult = anim_speed;
		fmult *= 0.75f;
		fmult *= pow(8.0f, 1.0f - speed_scale*0.1f);
		fAnimTime += frameTime * fmult;
	}

	if (texmgr.getCurTex())
	{
		const float intframe2 = fAnimTime*22.5f;
		const float scale = 0.45f + 0.1f*sin(intframe2*0.01f);
		const float rot = fAnimTime*rotational_speed*6.28f;

		if (cell == NULL) cell = new td_cellcornerinfo[UVCELLSX * UVCELLSY];

#define CELL(i,j) cell[((i) * UVCELLSX) + (j)]
		
		memset(cell, 0, sizeof(td_cellcornerinfo)*(UVCELLSX)*(UVCELLSY));

		#define NUM_MODES 7

		float t[NUM_MODES];
		t[0] = pow(0.50f + 0.50f*sin(fAnimTime*mode_switch_speed_multiplier * 0.1216f + fRandStart1), 1.0f);
		t[1] = pow(0.48f + 0.48f*sin(fAnimTime*mode_switch_speed_multiplier * 0.0625f + fRandStart2), 2.0f);
		t[2] = pow(0.45f + 0.45f*sin(fAnimTime*mode_switch_speed_multiplier * 0.0253f + fRandStart3), 12.0f);
		t[3] = pow(0.50f + 0.50f*sin(fAnimTime*mode_switch_speed_multiplier * 0.0916f + fRandStart4), 2.0f);
		t[4] = pow(0.50f + 0.50f*sin(fAnimTime*mode_switch_speed_multiplier * 0.0625f + fRandStart1), 2.0f);
		t[5] = pow(0.70f + 0.50f*sin(fAnimTime*mode_switch_speed_multiplier * 0.0466f + fRandStart2), 1.0f);
		t[6] = pow(0.50f + 0.50f*sin(fAnimTime*mode_switch_speed_multiplier * 0.0587f + fRandStart3), 2.0f);
		//t[(intframe/120) % NUM_MODES] += 20.0f;
		
		// normalize
		{
			float sum = 0.0f;
			for (int i=0; i<NUM_MODES; i++) sum += t[i]*t[i];
			const float mag = 1.0f/sqrt(sum);
			for (int i=0; i<NUM_MODES; i++) t[i] *= mag;
		}

		// keep top dog at 1.0, and scale all others down by raising to some exponent
		for (int i=0; i<NUM_MODES; i++) t[i] = pow(t[i], mode_focus_exponent);

		// bias t[1] by bass (stomach)
		//t[1] += max(0, (bass - 1.1f)*2.5f);

		// bias t[2] by treble (crazy)
		//t[2] += max(0, (treb - 1.1f)*1.5f);

		// give bias to original drempels effect
		t[0] += 0.2f;
		
		// re-normalize
		{
			float sum = 0.0f;
			for (int i=0; i<NUM_MODES; i++) sum += t[i]*t[i];
			const float mag = 1.0f/sqrt(sum);
			for (int i=0; i<NUM_MODES; i++) t[i] *= mag;
		}

		// orig: 1.0-4.5... now: 1.0 + 1.15*[0.0...3.0]
		const float fscale1 = 1.0f + 1.15f*(pow(2.0f, 1.0f + 0.5f*sin(fAnimTime*0.892f) + 0.5f*sin(fAnimTime*0.624f)) - 1.0f);
		const float fscale2 = 4.0f + 1.0f*sin(fRandStart3 + fAnimTime*0.517f) + 1.0f*sin(fRandStart4 + fAnimTime*0.976f);
		const float fscale3 = 4.0f + 1.0f*sin(fRandStart1 + fAnimTime*0.654f) + 1.0f*sin(fRandStart1 + fAnimTime*1.044f);
		const float fscale4 = 4.0f + 1.0f*sin(fRandStart2 + fAnimTime*0.517f) + 1.0f*sin(fRandStart3 + fAnimTime*0.976f);
		const float fscale5 = 4.0f + 1.0f*sin(fRandStart4 + fAnimTime*0.654f) + 1.0f*sin(fRandStart2 + fAnimTime*1.044f);

		const float t3_uc = 0.3f*sin(0.217f*(fAnimTime+fRandStart1)) + 0.2f*sin(0.185f*(fAnimTime+fRandStart2));
		const float t3_vc = 0.3f*cos(0.249f*(fAnimTime+fRandStart3)) + 0.2f*cos(0.153f*(fAnimTime+fRandStart4));
		const float t3_rot = 3.3f*cos(0.1290f*(fAnimTime+fRandStart2)) + 2.2f*cos(0.1039f*(fAnimTime+fRandStart3));
		const float cos_t3_rot = cos(t3_rot);
		const float sin_t3_rot = sin(t3_rot);
		const float t4_uc = 0.2f*sin(0.207f*(fAnimTime+fRandStart2)) + 0.2f*sin(0.145f*(fAnimTime+fRandStart4));
		const float t4_vc = 0.2f*cos(0.219f*(fAnimTime+fRandStart1)) + 0.2f*cos(0.163f*(fAnimTime+fRandStart3));
		const float t4_rot = 0.61f*cos(0.1230f*(fAnimTime+fRandStart4)) + 0.43f*cos(0.1009f*(fAnimTime+fRandStart1));
		const float cos_t4_rot = cos(t4_rot);
		const float sin_t4_rot = sin(t4_rot);

		const float u_delta = 0.05f;
		const float v_delta = 0.05f;

		const float u_offset = 0.5f;
		const float v_offset = 0.5f;

		for (unsigned int i=0; i<UVCELLSX; i++)
		for (unsigned int j=0; j<UVCELLSY; j++)
		{
			float base_u = (i/2*2)/(float)(UVCELLSX-2) - u_offset;
			float base_v = (j/2*2)/(float)(UVCELLSY-2) - v_offset;
			if (i & 1) base_u += u_delta;
			if (j & 1) base_v += v_delta;
			base_v *= -1.0f;
			
			CELL(i,j).u = 0;
			CELL(i,j).v = 0;

			// correct for aspect ratio:
			base_u *= 1.333f;


			//------------------------------ v1.0 code
			{
				float u = base_u;
				float v = base_v;
				u += warp_factor*0.65f*sin(intframe2*warp_w[0] + (base_u*warp_uscale[0] + base_v*warp_vscale[0])*6.28f + warp_phase[0]);
				v += warp_factor*0.65f*sin(intframe2*warp_w[1] + (base_u*warp_uscale[1] - base_v*warp_vscale[1])*6.28f + warp_phase[1]);
				u += warp_factor*0.35f*sin(intframe2*warp_w[2] + (base_u*warp_uscale[2] - base_v*warp_vscale[2])*6.28f + warp_phase[2]);
				v += warp_factor*0.35f*sin(intframe2*warp_w[3] + (base_u*warp_uscale[3] + base_v*warp_vscale[3])*6.28f + warp_phase[3]);
				u /= scale;
				v /= scale;

				const float ut = u;
				const float vt = v;
				u = ut*cos(rot) - vt*sin(rot);
				v = ut*sin(rot) + vt*cos(rot);

				// NOTE: THIS MULTIPLIER WAS --2.7-- IN THE ORIGINAL DREMPELS 1.0!!!
				u += 2.0f*sin(intframe2*0.00613f);
				v += 2.0f*cos(intframe2*0.0138f);

				CELL(i,j).u += u * t[0];
				CELL(i,j).v += v * t[0];
			}
			//------------------------------ v1.0 code

			{
				// stomach
				float u = base_u;
				float v = base_v;

				float rad = sqrt(u*u + v*v);
				float ang = atan2(u, v);

				rad *= 1.0f + 0.3f*sin(fAnimTime * 0.53f + ang*1.0f + fRandStart2);
				ang += 0.9f*sin(fAnimTime * 0.45f + rad*4.2f + fRandStart3);
				
				u = rad*cos(ang)*1.7f;
				v = rad*sin(ang)*1.7f;

				CELL(i,j).u += u * t[1];
				CELL(i,j).v += v * t[1];
			}						


			{
				// crazy
				float u = base_u;
				float v = base_v;

				float rad = sqrt(u*u + v*v);
				float ang = atan2(u, v);

				rad *= 1.0f + 0.3f*sin(fAnimTime * 1.59f + ang*20.4f + fRandStart3);
				ang += 1.8f*sin(fAnimTime * 1.35f + rad*22.1f + fRandStart4);
				
				u = rad*cos(ang);
				v = rad*sin(ang);

				CELL(i,j).u += u * t[2];
				CELL(i,j).v += v * t[2];
			}

			{
				// rotation
				//float u = (i/(float)UVCELLSX)*1.6f - 0.5f - t3_uc;
				//float v = (j/(float)UVCELLSY)*1.6f - 0.5f - t3_vc;
				float u = base_u*1.6f - t3_uc;
				float v = base_v*1.6f - t3_vc;
				float u2 = u*cos_t3_rot - v*sin_t3_rot + t3_uc;
				float v2 = u*sin_t3_rot + v*cos_t3_rot + t3_vc;

				CELL(i,j).u += u2 * t[3];
				CELL(i,j).v += v2 * t[3];
			}

			{
				// zoom out & minor rotate (to keep it interesting, if isolated)
				//float u = i/(float)UVCELLSX - 0.5f - t4_uc;
				//float v = j/(float)UVCELLSY - 0.5f - t4_vc;
				float u = base_u - t4_uc;
				float v = base_v - t4_vc;

				u = u*fscale1 + t4_uc - t3_uc;
				v = v*fscale1 + t4_vc - t3_uc;

				float u2 = u*cos_t4_rot - v*sin_t4_rot + t3_uc;
				float v2 = u*sin_t4_rot + v*cos_t4_rot + t3_vc;

				CELL(i,j).u += u2 * t[4];
				CELL(i,j).v += v2 * t[4];
			}

			{
				// SWIRLIES!
				float u = base_u*1.4f;
				float v = base_v*1.4f;
				float offset = 0;//((u+2.0f)*(v-2.0f) + u*u + v*v)*50.0f;

				float u2 = u + 0.03f*sin(u*(fscale2 + 2.0f) + v*(fscale3 + 2.0f) + fRandStart4 + fAnimTime*1.13f + 3.0f + offset);
				float v2 = v + 0.03f*cos(u*(fscale4 + 2.0f) - v*(fscale5 + 2.0f) + fRandStart2 + fAnimTime*1.03f - 7.0f + offset);
				u2 += 0.024f*sin(u*(fscale3*-0.1f) + v*(fscale5*0.9f) + fRandStart3 + fAnimTime*0.53f - 3.0f);
				v2 += 0.024f*cos(u*(fscale2*0.9f) + v*(fscale4*-0.1f) + fRandStart1 + fAnimTime*0.58f + 2.0f);

				CELL(i,j).u += u2*1.25f * t[5];
				CELL(i,j).v += v2*1.25f * t[5];
			}						

			
			{
				// tunnel
				float u = base_u*1.4f - t4_vc;
				float v = base_v*1.4f - t4_uc;

				float rad = sqrt(u*u + v*v);
				float ang = atan2(u, v);

				u = rad + 3.0f*sin(fAnimTime*0.133f + fRandStart1) + t4_vc;
				v = rad*0.5f * 0.1f*cos(ang + fAnimTime*0.079f + fRandStart4) + t4_uc;
								
				CELL(i,j).u += u * t[6];
				CELL(i,j).v += v * t[6];
			}

		}
	
		{
			float inv_master_zoom = 1.0f / (master_zoom * 1.8f);
			inv_master_zoom *= pow(4.0f, 1.0f - tex_scale*0.1f);
			const float int_scalar = 256.0f*(INTFACTOR);
			for (unsigned int j=0; j<UVCELLSY; j++)
			for (unsigned int i=0; i<UVCELLSX; i++)
			{
				CELL(i,j).u *= inv_master_zoom;
				CELL(i,j).v *= inv_master_zoom;
				CELL(i,j).u += 0.5f;
				CELL(i,j).v += 0.5f;
				CELL(i,j).u *= int_scalar;
				CELL(i,j).v *= int_scalar;
			}
		}

		for (unsigned int j=0; j<UVCELLSY; j++)
		for (unsigned int i=0; i<UVCELLSX-1; i+=2)
		{
			CELL(i,j).r = (CELL(i+1,j).u - CELL(i,j).u) / (u_delta*FXW);
			CELL(i,j).s = (CELL(i+1,j).v - CELL(i,j).v) / (v_delta*FXW);
		}

		for (unsigned int j=0; j<UVCELLSY-1; j+=2)
		for (unsigned int i=0; i<UVCELLSX; i+=2)
		{
			CELL(i,j).dudy = (CELL(i,j+1).u - CELL(i,j).u) / (u_delta*FXH);
			CELL(i,j).dvdy = (CELL(i,j+1).v - CELL(i,j).v) / (v_delta*FXH);
			CELL(i,j).drdy = (CELL(i,j+1).r - CELL(i,j).r) / (u_delta*FXH);
			CELL(i,j).dsdy = (CELL(i,j+1).s - CELL(i,j).s) / (v_delta*FXH);
		}

		if (buf == NULL) buf = new unsigned short [FXW * FXH * 2];
		for (unsigned int jj = 0; jj < UVCELLSY - 2; jj += 2)
		{
			for (unsigned int ii = 0; ii < UVCELLSX - 2; ii += 2)
			{
				const unsigned int x0 = ii * cellResolution;
				const unsigned int y0 = jj * cellResolution;
				const unsigned int x1 = (ii + 2) * cellResolution;
				const unsigned int y1 = (jj + 2) * cellResolution;

				Warp(CELL(ii,jj), CELL(ii + 2,jj), CELL(ii,jj + 2), CELL(ii + 2,jj + 2), x1 - x0, y1 - y0, &buf[(y0 * FXW + x0) * 2], FXW * 2);
			}
		}

		const float blurAmount = 0.97f*pow(motion_blur*0.1f, 0.27f);

		// Can't linearly interpolate the (u,v) mapping texture as it "overflows" from 1 to 0,
		// so render it one to one first, and then linearly scaled
		glViewport (0, 0, FXW, FXH);

		glBindTexture (GL_TEXTURE_2D, btex);

		glColor4f (1.0f, 1.0f, 1.0f, 1.0f);
		glBegin (GL_QUADS);
		glTexCoord2f (0.0, 0.0); glVertex2f (0.0, 0.0);
		glTexCoord2f (0.0, 1.0); glVertex2f (0.0, 1.0);
		glTexCoord2f (1.0, 1.0); glVertex2f (1.0, 1.0);
		glTexCoord2f (1.0, 0.0); glVertex2f (1.0, 0.0);
		glEnd ();

		glEnable (GL_BLEND);
		glColor4f (1.0f, 1.0f, 1.0f, 1.0f - blurAmount);
		glBindTexture (GL_TEXTURE_2D, tex);

#ifdef HAVE_GLEW
		if (dShader)
		{
			glActiveTextureARB (GL_TEXTURE1_ARB);
			glBindTexture (GL_TEXTURE_2D, uvtex);
			glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, FXW, FXH, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);

			glEnable (GL_FRAGMENT_PROGRAM_ARB);

			glBegin (GL_QUADS);
			glTexCoord2f (0.0, 0.0); glVertex2f (0.0, 0.0);
			glTexCoord2f (0.0, 1.0); glVertex2f (0.0, 1.0);
			glTexCoord2f (1.0, 1.0); glVertex2f (1.0, 1.0);
			glTexCoord2f (1.0, 0.0); glVertex2f (1.0, 0.0);
			glEnd ();

			glDisable (GL_FRAGMENT_PROGRAM_ARB);

			glBindTexture (GL_TEXTURE_2D, 0);
			glActiveTextureARB (GL_TEXTURE0_ARB);
		}
		else
#endif
		{
			unsigned short *uvbuf = buf;
			uint32_t *texbuf = fadeComplete ? texmgr.getCurTex() : fadeBuf;
			uint32_t *outbuf = (uint32_t *)buf;
			for (unsigned int ii = 0; ii < FXW * FXH; ++ii) {
				const uint16_t u0 = *uvbuf++;
				const uint16_t v0 = *uvbuf++;
				const uint16_t u1 = u0 + 256;
				const uint16_t v1 = v0 + 256;

				const uint32_t tl = texbuf[(v0 & 0xff00) | (u0 >> 8)];
				const uint32_t tr = texbuf[(v0 & 0xff00) | (u1 >> 8)];
				const uint32_t bl = texbuf[(v1 & 0xff00) | (u0 >> 8)];
				const uint32_t br = texbuf[(v1 & 0xff00) | (u1 >> 8)];

				const uint32_t l = rgbLerp(tl, bl, v0);
				const uint32_t r = rgbLerp(tr, br, v0);

				*outbuf++ = rgbLerp(l, r, u0);
			}

			glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, FXW, FXH, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);

			glBegin (GL_QUADS);
			glTexCoord2f (0.0, 0.0); glVertex2f (0.0, 0.0);
			glTexCoord2f (0.0, 1.0); glVertex2f (0.0, 1.0);
			glTexCoord2f (1.0, 1.0); glVertex2f (1.0, 1.0);
			glTexCoord2f (1.0, 0.0); glVertex2f (1.0, 0.0);
			glEnd ();
		}

		glDisable (GL_BLEND);
		glBindTexture (GL_TEXTURE_2D, btex);

		glCopyTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, 0, 0, FXW, FXH, 0);

		glViewport (0, 0, XStuff->windowWidth, XStuff->windowHeight);

		glColor4f (1.0f, 1.0f, 1.0f, 1.0f);
		glBegin (GL_QUADS);
		glTexCoord2f (0.0, 0.0); glVertex2f (0.0, 0.0);
		glTexCoord2f (0.0, 1.0); glVertex2f (0.0, 1.0);
		glTexCoord2f (1.0, 1.0); glVertex2f (1.0, 1.0);
		glTexCoord2f (1.0, 0.0); glVertex2f (1.0, 0.0);
		glEnd ();
	}
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

	// Keep cells * cellResolution < resolution of the window
	cells = dCells;
	cellResolution = dCellResolution;
	while ((cells * cellResolution > XStuff->windowWidth) || (cells * cellResolution > XStuff->windowHeight)) {
		if (cells > cellResolution)
		{
			cells = cells >> 1;
		}
		else if (cellResolution > cells)
		{
			cellResolution = cellResolution >> 1;
		}
		else
		{
			cells = cells >> 1;
		}	
	}

	delete [] cell; cell = NULL;
	delete [] buf; buf = NULL;
}

#ifdef HAVE_GLEW
bool initExtensions()
{
	if (!glewInitialized)
		return false;

	const char *necessaryExtensions[2] = { "GL_ARB_multitexture", "GL_ARB_fragment_program" };

	for (int i = 0; i < 2; i++)
		if (GL_TRUE != glewGetExtension(necessaryExtensions[i]))
			return false;

	return true;
}
#endif

void hack_init (xstuff_t * XStuff)
{
	hack_reshape(XStuff);

#ifdef HAVE_GLEW
	// initialize extensions
	dShader &= initExtensions();

	if (dShader)
	{
		// Indirect texture coordinate lookup fragment program
		glGenProgramsARB (1, &indirect_fp);
		glBindProgramARB (GL_FRAGMENT_PROGRAM_ARB, indirect_fp);
		glProgramStringARB (GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen((const char *)indirect_fp_asm), indirect_fp_asm);
	}

	// If we are doing texture lookups on the CPU, its hard coded for 256x256 source texture
	if (!dShader)
#endif
	{
		dGenTexSize = 256;
		texmgr.setTexSize(256, 256);

		fadeBuf = new uint32_t[256 * 256];
	}

	texmgr.setGenTexSize(dGenTexSize, dGenTexSize);

	glEnable (GL_TEXTURE_2D);

	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Seamless source texture
	glGenTextures (1, &tex);
	glBindTexture (GL_TEXTURE_2D, tex);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Current texture (for fading)
	glGenTextures (1, &ctex);
	glBindTexture (GL_TEXTURE_2D, ctex);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Previous texture (for fading)
	glGenTextures (1, &ptex);
	glBindTexture (GL_TEXTURE_2D, ptex);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Indirect texture coordinate texture
	glGenTextures (1, &uvtex);
	glBindTexture (GL_TEXTURE_2D, uvtex);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	// Blurred result texture
	glGenTextures (1, &btex);
	glBindTexture (GL_TEXTURE_2D, btex);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	{
		unsigned char buf = 0;

		glTexImage2D (GL_TEXTURE_2D, 0, GL_LUMINANCE, 1, 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, &buf);
	}

	RandomizeStartValues();

	texmgr.start();
}

void hack_cleanup (xstuff_t * XStuff)
{
	texmgr.stop();

	delete [] fadeBuf;
	delete [] cell;
	delete [] buf;
}

void hack_handle_opts (int argc, char **argv)
{
	while (1) {
		int c;

#ifdef HAVE_GETOPT_H
		static struct option long_options[] = {
			{"help", 0, 0, 'h'},
			{"blur", 1, 0, 'b'},
			{"speed", 1, 0, 's'},
			{"scale", 1, 0, 'S'},
			{"cells", 1, 0, 'c'},
			{"cellresolution", 1, 0, 'R'},
			{"images", 1, 0, 'i'},
			{"texinterval", 1, 0, 'I'},
			{"texfadeinterval", 1, 0, 'f'},
			{"gentexsize", 1, 0, 'g'},
			{"no-shader", 0, 0, 'H'},
			DRIVER_OPTIONS_LONG
		};

		c = getopt_long (argc, argv, DRIVER_OPTIONS_SHORT "hb:s:S:c:R:i:I:f:g:H", long_options, NULL);
#else
		c = getopt (argc, argv, DRIVER_OPTIONS_SHORT "hb:s:S:c:R:i:I:f:g:H");
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
				"\t--blur/-b <arg>\n"
				"\t--speed/-s <arg>\n"
				"\t--scale/-S <arg>\n"
				"\t--cells/-c <arg>\n"
				"\t--cellresolution/-R <arg>\n"
				"\t--images/-i <arg>\n"
				"\t--texinterval/-I <arg>\n"
				"\t--texfadeinterval/-f <arg>\n"
				"\t--gentexsize/-g <arg>\n"
				"\t--no-shader/-H\n", argv[0]);
			exit (1);
		case 'b':
			motion_blur = strtol_minmaxdef (optarg, 10, 0, 10, 1, 7, "--blur: ");
			break;
		case 's':
			speed_scale = 20 - strtol_minmaxdef (optarg, 10, 0, 20, 1, 10, "--speed: ");
			break;
		case 'S':
			tex_scale = strtol_minmaxdef (optarg, 10, 0, 20, 1, 10, "--scale: ");
			break;
		case 'c':
			dCells = 1 << strtol_minmaxdef (optarg, 10, 4, 8, 1, 4, "--cells: ");
			break;
		case 'R':
			dCellResolution = 1 << strtol_minmaxdef (optarg, 10, 4, 8, 1, 4, "--cellresolution: ");
			break;
		case 'i':
			{
				struct stat fileStat;

				if (!stat(optarg, (struct stat *)&fileStat)) {
					if (S_ISDIR(fileStat.st_mode)) {
						DIR *imageDir = opendir (optarg);
						if (imageDir) {
							closedir(imageDir);
							texmgr.setImageDir (optarg);
						} else {
							fprintf (stderr, "--images: Could not open %s\n", optarg);
						}
					} else {
						fprintf(stderr, "--images: %s is not a directory\n", optarg);
					}
				} else {
					fprintf(stderr, "--images: Could not stat %s\n", optarg);
				}
			}

			break;
		case 'I':
			dTexInterval = strtol_minmaxdef (optarg, 10, 1, 3600, 1, 10, "--texinterval: ");
			break;
		case 'f':
			dTexFadeInterval = strtol_minmaxdef (optarg, 10, 1, 100, 1, 10, "--texfadeinterval: ") / 10.0;
			break;
		case 'g':
			dGenTexSize = 1 << strtol_minmaxdef (optarg, 10, 4, 12, 1, 8, "--gentexsize: ");
			break;
#ifdef HAVE_GLEW
		case 'H':
			dShader = false;
			break;
#endif
		}
	}
}
