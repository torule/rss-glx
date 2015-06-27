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

#include "skyrocket_sound.h"

#ifdef HAVE_OPENAL

#include <math.h>
#include <stdlib.h>
#include <AL/al.h>
#include <AL/alut.h>

#include <list>

#include "loadTexture.h"

// reference distance, max distance
// reference distance = sound is partly attenuated
// maximum distance = sound is completely attenuated
float sound_distances[NUM_BUFFERS][2] = { 
	{ 300.0f, 600.0f },	// launch sounds
	{ 300.0f, 600.0f },
	{ 4000.0f, 8000.0f },	// booms
	{ 4000.0f, 8000.0f },
	{ 4000.0f, 8000.0f },
	{ 6000.0f, 12000.0f },
	{ 2500.0f, 5000.0f },	// poppers
	{ 10000.0f, 20000.0f },	// suck
	{ 20000.0f, 40000.0f },	// nuke
	{ 2500.0f, 5000.0f }	// whistle
};

class soundnode;

ALuint g_Buffers[NUM_BUFFERS];
ALuint g_Sources[NUM_SOURCES];
extern float elapsedTime;
extern int dSound;
extern int kSlowMotion;

void swap_bytes (unsigned char *data, int bytes)
{
	int i = 0;

	for (i = 0; i < bytes - 1; i += 2) {
		int temp = data[i];

		data[i] = data[i + 1];
		data[i + 1] = temp;
	}
}

void initSound ()
{
	int i = 1;
	unsigned char *j;
	unsigned char *l_snd;

	j = (unsigned char *)&i;

	alutInit (NULL, 0);

	alDistanceModel (AL_INVERSE_DISTANCE);
	// As of October, 2001, AL_ROLLOFF_FACTOR isn't implemented in the Windows 
	// version of OpenAL, so the distance attenuation models won't work.  You 
	// have to set the gains yourself.

	//alDistanceModel(AL_NONE);

	alDopplerVelocity (1130.0f);	// Sound travels at 1130 feet/sec
	alListenerf (AL_GAIN, float (dSound) * 0.01f);	// Volume

	// Initialize sound data
	alGenBuffers (NUM_BUFFERS, g_Buffers);
	alGenSources (NUM_SOURCES, g_Sources);

#define BUFFER_SOUND(index, data, compressedsize, size) \
	LOAD_TEXTURE(l_snd, data, compressedsize, size) \
	if (!*j) swap_bytes(l_snd, size); \
	alBufferData(g_Buffers[index], AL_FORMAT_MONO16, l_snd, size, 44100); \
	FREE_TEXTURE(l_snd)

	BUFFER_SOUND (BOOM1SOUND, launch1SoundData, launch1SoundData_compressedsize, launch1SoundData_size)
	BUFFER_SOUND (BOOM2SOUND, launch2SoundData, launch2SoundData_compressedsize, launch2SoundData_size)
	BUFFER_SOUND (BOOM1SOUND, boom1SoundData, boom1SoundData_compressedsize, boom1SoundData_size)
	BUFFER_SOUND (BOOM2SOUND, boom2SoundData, boom2SoundData_compressedsize, boom2SoundData_size)
	BUFFER_SOUND (BOOM3SOUND, boom3SoundData, boom3SoundData_compressedsize, boom3SoundData_size)
	BUFFER_SOUND (BOOM4SOUND, boom4SoundData, boom4SoundData_compressedsize, boom4SoundData_size)
	BUFFER_SOUND (POPPERSOUND, popperSoundData, popperSoundData_compressedsize, popperSoundData_size)
	BUFFER_SOUND (POPPERSOUND, suckSoundData, suckSoundData_compressedsize, suckSoundData_size)
	BUFFER_SOUND (POPPERSOUND, nukeSoundData, nukeSoundData_compressedsize, nukeSoundData_size)
	BUFFER_SOUND (POPPERSOUND, whistleSoundData, whistleSoundData_compressedsize, whistleSoundData_size)
}

class soundnode {
      public:
	int sound;
	float pos[3];
	float dist;
	float time;		// time until sound plays

	  soundnode () {
	};

	~soundnode () {
	};
};

std::list < soundnode > soundlist;

void insertSoundNode (int sound, rsVec source, rsVec observer)
{
	soundnode *node;
	rsVec dir = observer - source;

	soundlist.push_back (soundnode ());
	node = &soundlist.back ();
	node->sound = sound;
	node->pos[0] = source[0];
	node->pos[1] = source[1];
	node->pos[2] = source[2];
	// distance to sound
	node->dist = sqrt (dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
	// Sound travels at 1130 feet/sec
	node->time = node->dist * 0.000885f;
	if (node->sound == POPPERSOUND)	// poppers have a little delay
		node->time += 2.5f;
}

void updateSound (float *listenerPos, float *listenerVel, float *listenerOri)
{
	static int source = 0;

	// Set current listener attributes
	alListenerfv (AL_POSITION, listenerPos);
	alListenerfv (AL_VELOCITY, listenerVel);
	alListenerfv (AL_ORIENTATION, listenerOri);

	std::list < soundnode >::iterator cursound = soundlist.begin ();
	while (cursound != soundlist.end ()) {
		cursound->time -= elapsedTime;
		if (cursound->time <= 0.0f) {
			if (cursound->dist < sound_distances[cursound->sound][1]) {
				alSourceStop (g_Sources[source]);
				alSourcei (g_Sources[source], AL_BUFFER, g_Buffers[cursound->sound]);
				//alSourcef(g_Sources[source], AL_REFERENCE_DISTANCE, sound_distances[cursound->sound][0]);
				//alSourcef(g_Sources[source], AL_ROLLOFF_FACTOR, 0.013557606f);
				// HACK:  AL_REFERENCE_DISTANCE must be set or OpenAL won't make any sound at all.
				// I assume this will be fixed in future implementations of OpenAL.
				alSourcef (g_Sources[source], AL_REFERENCE_DISTANCE, 1000000.0f);
				// As of October, 2001, AL_ROLLOFF_FACTOR isn't implemented in the Windows 
				// version of OpenAL, so the distance attenuation models won't work.  You 
				// have to set the gains yourself.
				// Here I implement the AL_INVERSE_DISTANCE model formula from the OpenAL spec.
				//float gain = 1.0f - 20.0f * log10(1.0f + 0.013557606f 
				//      * (cursound->dist - sound_distances[cursound->sound][0]) 
				//      / sound_distances[cursound->sound][0]);
				// But I don't like the way it sounds, so I'll just fudge everything
				float gain = 1.0f - (cursound->dist / sound_distances[cursound->sound][1]);

				if (gain > 1.0f)
					gain = 1.0f;
				if (gain < 0.0f)
					gain = 0.0f;
				alSourcef (g_Sources[source], AL_GAIN, gain);
				alSourcefv (g_Sources[source], AL_POSITION, cursound->pos);
				alSourcei (g_Sources[source], AL_LOOPING, AL_FALSE);
				if (kSlowMotion)	// Slow down the sound
					alSourcef (g_Sources[source], AL_PITCH, 0.5f);
				else	// Sound at regular speed
					alSourcef (g_Sources[source], AL_PITCH, 1.0f);
				alSourcePlay (g_Sources[source]);
				source++;
				if (source >= NUM_SOURCES)
					source = 0;
			}
			cursound = soundlist.erase (cursound);
		} else
			cursound++;
	}
}

void cleanupSound ()
{
	alDeleteSources (NUM_SOURCES, g_Sources);
	alDeleteBuffers (NUM_BUFFERS, g_Buffers);

	ALCcontext * const context = alcGetCurrentContext ();
	ALCdevice * const device = alcGetContextsDevice (context);

	alcMakeContextCurrent (NULL);
	alcDestroyContext (context);
	alcCloseDevice (device);

	alutExit ();
}

#endif
