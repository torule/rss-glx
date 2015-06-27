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

#ifndef SKYROCKET_SOUND_H
#define SKYROCKET_SOUND_H

#include "config.h"

#ifdef HAVE_OPENAL

#include "rsMath/rsVec.h"

#define NUM_SOURCES 16
#define NUM_BUFFERS 10
#define LAUNCH1SOUND 0
#define LAUNCH2SOUND 1
#define BOOM1SOUND 2
#define BOOM2SOUND 3
#define BOOM3SOUND 4
#define BOOM4SOUND 5
#define POPPERSOUND 6
#define SUCKSOUND 7
#define NUKESOUND 8
#define WHISTLESOUND 9

void initSound ();
void insertSoundNode (int sound, rsVec source, rsVec observer);
void updateSound (float *listenerPos, float *listenerVel, float *listenerOri);
void cleanupSound ();

extern unsigned int boom1SoundData_size;
extern unsigned int boom2SoundData_size;
extern unsigned int boom3SoundData_size;
extern unsigned int boom4SoundData_size;
extern unsigned int boom1SoundData_compressedsize;
extern unsigned int boom2SoundData_compressedsize;
extern unsigned int boom3SoundData_compressedsize;
extern unsigned int boom4SoundData_compressedsize;
extern unsigned char *boom1SoundData;
extern unsigned char *boom2SoundData;
extern unsigned char *boom3SoundData;
extern unsigned char *boom4SoundData;

extern unsigned int launch1SoundData_size;
extern unsigned int launch2SoundData_size;
extern unsigned int launch1SoundData_compressedsize;
extern unsigned int launch2SoundData_compressedsize;
extern unsigned char *launch1SoundData;
extern unsigned char *launch2SoundData;

extern unsigned int nukeSoundData_size;
extern unsigned int nukeSoundData_compressedsize;
extern unsigned char *nukeSoundData;

extern unsigned int popperSoundData_size;
extern unsigned int popperSoundData_compressedsize;
extern unsigned char *popperSoundData;

extern unsigned int suckSoundData_size;
extern unsigned int suckSoundData_compressedsize;
extern unsigned char *suckSoundData;

extern unsigned int whistleSoundData_size;
extern unsigned int whistleSoundData_compressedsize;
extern unsigned char *whistleSoundData;

#endif

#endif
