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

#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "loadTexture.h"
#include "skyrocket_smoke.h"
#include "smoketex.h"

extern int dSmoke;
extern int dExplosionsmoke;

// lifespans for smoke particles
float smokeTime[SMOKETIMES];	// lifespans of consecutive smoke particles
int whichSmoke[WHICHSMOKES];	// table to indicate which particles produce smoke

unsigned int smokelist[5];

// Initialize smoke texture objects and display lists
void initSmoke ()
{
	int i, j, k;
	unsigned int smoketex[5];
	unsigned char smoke[SMOKETEXSIZE][SMOKETEXSIZE][2];
	unsigned char *presmoke = NULL;

	glGenTextures (5, smoketex);

	for (k = 0; k < 5; k++) {
		if (k == 0) {
			LOAD_TEXTURE (presmoke, presmoke1, presmoke1_compressedsize, presmoke1_size)
		} else if (k == 1) {
			LOAD_TEXTURE (presmoke, presmoke2, presmoke2_compressedsize, presmoke2_size)
		} else if (k == 2) {
			LOAD_TEXTURE (presmoke, presmoke3, presmoke3_compressedsize, presmoke3_size)
		} else if (k == 3) {
			LOAD_TEXTURE (presmoke, presmoke4, presmoke4_compressedsize, presmoke4_size)
		} else if (k == 4) {
			LOAD_TEXTURE (presmoke, presmoke5, presmoke5_compressedsize, presmoke5_size)
		}

		for (i = 0; i < SMOKETEXSIZE; i++) {
			for (j = 0; j < SMOKETEXSIZE; j++) {
				smoke[i][j][0] = 255;
				smoke[i][j][1] = presmoke[i * SMOKETEXSIZE + j];
			}
		}

		glBindTexture (GL_TEXTURE_2D, smoketex[k]);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		gluBuild2DMipmaps (GL_TEXTURE_2D, 2, SMOKETEXSIZE, SMOKETEXSIZE, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, smoke);

		FREE_TEXTURE (presmoke)
	}

	smokelist[0] = glGenLists (5);
	smokelist[1] = smokelist[0] + 1;
	smokelist[2] = smokelist[0] + 2;
	smokelist[3] = smokelist[0] + 3;
	smokelist[4] = smokelist[0] + 4;
	for (i = 0; i < 5; i++) {
		glNewList (smokelist[i], GL_COMPILE);
		glBindTexture (GL_TEXTURE_2D, smoketex[i]);
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

	// set smoke lifespans  ( 1 2 1 4 1 2 1 8 )
	smokeTime[0] = smokeTime[2] = smokeTime[4] = smokeTime[6] = 0.4f;
	smokeTime[1] = smokeTime[5] = 0.8f;
	smokeTime[3] = 2.0f;
	smokeTime[7] = 4.0f;
	for (i = 0; i < SMOKETIMES; i++) {
		if (smokeTime[i] > float (dSmoke))
			smokeTime[i] = float (dSmoke);
	}
	if (smokeTime[7] < float (dSmoke))
		smokeTime[7] = float (dSmoke);

	// create table describing which particles will emit smoke
	// 0 = don't emit smoke
	// 1 = emit smoke
	for (i = 0; i < WHICHSMOKES; i++)
		whichSmoke[i] = 0;
	if (dExplosionsmoke) {
		float index = float (WHICHSMOKES) / float (dExplosionsmoke);

		for (i = 0; i < dExplosionsmoke; i++)
			whichSmoke[int (float (i) * index)] = 1;
	}
}

void cleanupSmoke ()
{
	glDeleteLists (smokelist[0], 5);
}
