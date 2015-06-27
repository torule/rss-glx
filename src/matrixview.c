/*
 * Copyright (C) 2003 Alex Zolotov <nightradio@knoppix.ru>
 * Mucked with by Tugrul Galatali <tugrul@galatali.com>
 *
 * MatrixView is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 *
 * MatrixView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// MatrixView screen saver

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <sys/time.h>

#include "driver.h"
#include "loadTexture.h"
#include "matrixview_textures.h"
#include "rsDefines.h"
#include "rsRand.h"

const char *hack_name = "MatrixView";

int cycleScene = 5;
int cycleMatrix = 5;
float contrast = -1;

// Settings for our light.  Try playing with these (or add more lights).
float Light_Ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
float Light_Diffuse[] = { 1.2f, 1.2f, 1.2f, 1.0f };
float Light_Position[] = { 2.0f, 2.0f, 0.0f, 1.0f };

unsigned char *font;
unsigned char *pics;

unsigned char flare[16] = { 0, 0, 0, 0,
	0, 180, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0
};

#define MAX_TEXT_X 320
#define MAX_TEXT_Y 240
int text_x = 90;
int text_y = 70;
#define _text_x text_x/2
#define _text_y text_y/2

// Scene position
#define Z_Off -128.0f
#define Z_Depth 8

unsigned char speed[MAX_TEXT_X];
unsigned char text[MAX_TEXT_X * MAX_TEXT_Y + MAX_TEXT_X];
unsigned char text_light[MAX_TEXT_X * MAX_TEXT_Y + MAX_TEXT_X];	//255 - white; 254 - none;
float text_depth[MAX_TEXT_X * MAX_TEXT_Y + MAX_TEXT_X];

unsigned char *pic = NULL;
float bump_pic[MAX_TEXT_X * MAX_TEXT_Y + MAX_TEXT_X];
int exit_mode = 0;		//0 - none; 1 - exit mode (explode main text);
float r1 = 0.2, r2 = 1, r3 = 0.4;
float texture_add = 0;
float exit_angle = 0;
long one_frame = 60;

int pic_mode = 0;		//0 - none; 1 - pic fade in; 2 - pic fade out;
int pic_fade = 0;

float *pts;
float *texcoords;
unsigned char *colors;

#include <magick/api.h>
#include <wand/magick-wand.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

char *dirName = NULL;
DIR *imageDir;

unsigned char *next_pic = NULL;

// Directory scanning + image loading code in a separate function callable either from loadNextImage or another thread if pthreads is available.
void loadNextImageFromDisk() {
	MagickWand *magick_wand = NewMagickWand();
	ExceptionInfo exception;
	int dirLoop = 0;

	GetExceptionInfo (&exception);

	int imageLoaded = 0;
	do {
		struct dirent *file;

		if (!imageDir) {
			if (dirLoop) {
				dirName = NULL;
				return;
			}

			imageDir = opendir (dirName);
			dirLoop = 1;
		}

		file = readdir (imageDir);
		if (file) {
			struct stat fileStat;
			char *full_path_and_name = (char *)malloc(strlen(dirName) + 1 + strlen(file->d_name) + 1);

			if (full_path_and_name) {
				sprintf(full_path_and_name, "%s/%s", dirName, file->d_name);

				if (!stat(full_path_and_name, (struct stat *)&fileStat)) {
					if (S_ISREG(fileStat.st_mode)) {
						if (MagickReadImage(magick_wand, full_path_and_name) == MagickFalse) {
							char *description;
							ExceptionType severity;

							description = MagickGetException (magick_wand, &severity);
							fprintf (stderr, "Error loading %s: %s\n", full_path_and_name, description);
							description = (char *)MagickRelinquishMemory (description);
						} else {
							imageLoaded = 1;
						}
					}
				}

				free(full_path_and_name);
			} else {
				fprintf (stderr, "Out of memory\n");
			}
		} else {
			closedir(imageDir);
			imageDir = NULL;
		}
	} while (!imageLoaded);

	MagickScaleImage (magick_wand, text_x, text_y);
	MagickNormalizeImage (magick_wand);
	MagickContrastImage (magick_wand, MagickTrue);
	MagickNegateImage (magick_wand, MagickFalse);

	if (!next_pic)
		next_pic = (unsigned char *)malloc (text_x * text_y);

	ExportImagePixels (GetImageFromMagickWand(magick_wand), 0, 0, text_x, text_y, "I", CharPixel, next_pic, &exception);

	magick_wand = DestroyMagickWand (magick_wand);
}

#include <pthread.h>
pthread_t *imageLoadingThread = NULL;
pthread_mutex_t *next_pic_mutex;
pthread_cond_t *next_pic_cond;
volatile int exiting = 0;

void *imageLoadingThreadMain(void *arg) {
	do {
		loadNextImageFromDisk();

		pthread_cond_wait(next_pic_cond, next_pic_mutex);
	} while (!exiting);

	pthread_exit(NULL);

	return NULL;
}

void loadNextImage ()
{
	if (dirName) {
		if (!imageLoadingThread) {
			next_pic_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
			pthread_mutex_init(next_pic_mutex, NULL);

			pthread_mutex_lock(next_pic_mutex);

			next_pic_cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
			pthread_cond_init(next_pic_cond, NULL);

			imageLoadingThread = (pthread_t *)malloc(sizeof(pthread_t));
			pthread_create(imageLoadingThread, NULL, imageLoadingThreadMain, NULL);
		} else {
			pthread_mutex_lock(next_pic_mutex);
			if (pic) {
				unsigned char *tmp = next_pic;
				next_pic = pic;
				pic = tmp;
			} else {
				pic = next_pic;
				next_pic = NULL;
			}
			pthread_mutex_unlock(next_pic_mutex);

			pthread_cond_signal(next_pic_cond);
		}
	} else {
		ExceptionInfo exception;
		Image *image = NULL, *scaled_image;
		ImageInfo *image_info;

		GetExceptionInfo (&exception);

		if (!pics)
			LOAD_TEXTURE (pics, cpics, cpics_compressedsize, cpics_size)

		if ((text_x != 90) || (text_y != 70)) {
			if (!pic)
				pic = (unsigned char *)malloc (text_x * text_y);

			image_info = CloneImageInfo ((ImageInfo *) NULL);
			image_info->size = AcquireMagickMemory(sizeof("90x70"));
			strcpy(image_info->size, "90x70");
			image = AcquireImage(image_info);

			ImportImagePixels(image, 0, 0, 90, 70, "I", CharPixel, (unsigned char *)(pics + ((random () & 15) * (90 * 70))));

			scaled_image = ScaleImage (image, text_x, text_y, &exception);

			ExportImagePixels (scaled_image, 0, 0, text_x, text_y, "I", CharPixel, pic, &exception);

			DestroyImage (image);
			DestroyImage (scaled_image);
		} else {
			if (!pics)
				LOAD_TEXTURE (pics, cpics, cpics_compressedsize, cpics_size)

			pic = (unsigned char *)(pics + ((random () & 15) * (text_x * text_y)));
		}
	}
}

void draw_illuminatedchar (long num, float x, float y, float z)
{
	float tx, ty;
	long num2, num3;

	num2 = num / 10;
	num3 = num - (num2 * 10);
	ty = (float)num2 / 7;
	tx = (float)num3 / 10;
	glNormal3f (0.0f, 0.0f, 1.0f);	// Needed for lighting
	glColor4f (0.9, 0.4, 0.3, .5);	// Basic polygon color

	glTexCoord2f (tx, ty);
	glVertex3f (x, y, z);
	glTexCoord2f (tx + 0.1, ty);
	glVertex3f (x + 1, y, z);
	glTexCoord2f (tx + 0.1, ty + 0.166);
	glVertex3f (x + 1, y - 1, z);
	glTexCoord2f (tx, ty + 0.166);
	glVertex3f (x, y - 1, z);
}

void draw_flare (float x, float y, float z)	//flare
{
	glNormal3f (0.0f, 0.0f, 1.0f);	// Needed for lighting
	glColor4f (0.9, 0.4, 0.3, .8);	// Basic polygon color

	glTexCoord2f (0, 0);
	glVertex3f (x - 1, y + 1, z);
	glTexCoord2f (0.75, 0);
	glVertex3f (x + 2, y + 1, z);
	glTexCoord2f (0.75, 0.75);
	glVertex3f (x + 2, y - 2, z);
	glTexCoord2f (0, 0.75);
	glVertex3f (x - 1, y - 2, z);
}

void draw_text ()
{
	int x, y;
	long p = 0;
	int c, c_pic;

	if (pic_mode == 1)
		if (pic_fade < 255)
			pic_fade++;

	if (pic_mode == 2)
		if (pic_fade > 0)
			pic_fade--;

	int vp = 0, tp = 0, cp = 0;
	for (y = _text_y; y > -_text_y; y--) {
		for (x = -_text_x; x < _text_x; x++) {
			c = text_light[p] - (text[p] >> 1);
			c += pic_fade;
			if (c > 255)
				c = 255;

			if (pic) {
				c_pic = pic[p] * contrast - (255 - pic_fade);

				if (c_pic < 0)
					c_pic = 0;

				c -= c_pic;

				if (c < 0)
					c = 0;

				bump_pic[p] = (255.0f - c_pic) / (256 / Z_Depth);
			} else {
				bump_pic[p] = Z_Depth;
			}

			if (c > 10)
				if (text[p]) {
					long num = text[p] + 1;
					float light = c;
					const float z = text_depth[p] + bump_pic[p];
					float tx, ty;
					long num2, num3;

					num &= 63;
					light = light / 255;	//light=7-light;num+=(light*60);
					num2 = num / 10;
					num3 = num - (num2 * 10);
					ty = (float)num2 / 7;
					tx = (float)num3 / 10;

#define VA(tx, ty, x, y, z, light) \
	colors[cp++] = 0.9 * 255; colors[cp++] = 0.4 * 255; colors[cp++] = 0.3 * 255; colors[cp++] = light * 255; \
	texcoords[tp++] = tx; texcoords[tp++] = ty; \
	pts[vp++] = x; pts[vp++] = y; pts[vp++] = z;

					VA(tx, ty, x, y, z, light);
					VA(tx + 0.1, ty, x + 1, y, z, light);
					VA(tx + 0.1, ty + 0.166, x + 1, y - 1, z, light);
					VA(tx, ty + 0.166, x, y - 1, z, light);
				}

			if (exit_mode)
				text_depth[p] += (((float)text[p] - 128) / 2000);
			else if (text_depth[p] < 0.1)
				text_depth[p] = 0;
			else
				text_depth[p] /= 1.1;

			p++;
		}
	}

	glNormal3f (0.0f, 0.0f, 1.0f);	// Needed for lighting

	glDrawArrays(GL_QUADS, 0, tp >> 1);
}

void draw_illuminatedtext (void)
{
	float x, y;
	long p = 0;

	for (y = _text_y; y > -_text_y; y--) {
		for (x = -_text_x; x < _text_x; x++) {
			if (text_light[p] > 128)
				if (text_light[p + text_x] < 10)
					draw_illuminatedchar (text[p] + 1, x, y, text_depth[p] + bump_pic[p]);
			p++;
		}
	}
}

void draw_flares (void)
{
	float x, y;
	long p = 0;

	for (y = _text_y; y > -_text_y; y--) {
		for (x = -_text_x; x < _text_x; x++) {
			if (text_light[p] > 128)
				if (text_light[p + text_x] < 10)
					draw_flare (x, y, text_depth[p] + bump_pic[p]);
			p++;
		}
	}
}

void scroll (double dCurrentTime)
{
	unsigned int a, s, polovina;
	static double dLastCycle = -1;
	static double dLastMove = -1;

	//====== CHANGING SCENE ======
	if (cycleScene > 0) {
		if (pic_mode == 1)
			if (dCurrentTime - dLastCycle > cycleScene) {
				pic_mode = 2;

				if ((random () & 63) == 63)
					exit_mode = 1;
			}

		if (dCurrentTime - dLastCycle > cycleScene + cycleMatrix)
			dLastCycle = dCurrentTime;

		if (dCurrentTime == dLastCycle) {
			loadNextImage ();

			// If a picture isn't available, don't switch modes and skip the cycleScene interval.
			if (pic)
				pic_mode = 1;	
			else
				dLastCycle -= cycleScene;

			if ((random () & 3) == 3)
				exit_mode = 0;
		}
	}

	if (dCurrentTime - dLastMove > 1.0 / (text_y / 1.5)) {
		dLastMove = dCurrentTime;

		polovina = (text_x * text_y) / 2;
		s = 0;
		for (a = (text_x * text_y) + text_x - 1; a > text_x; a--) {
			if (speed[s]) {
				text_light[a] = text_light[a - text_x];	//scroll light table down
			}
			s++;
			if (s >= text_x)
				s = 0;
		}

		//============================
		//for (a = (text_x * text_y) + text_x - 1; a > text_x; a--) {
		//      text_light[a] = text_light[a - text_x]; //scroll light table down
		//}
		memmove ((void *)(&text_light[0] + text_x), (void *)&text_light, (text_x * text_y) - 1);

		//for (a = 0; a < text_x; a++)
		//      text_light[a] = 253;    //clear top line (in light table)
		memset ((void *)&text_light, 253, text_x);

		s = 0;
		for (a = polovina; a < (text_x * text_y); a++) {
			if (text_light[a] == 255)
				text_light[s] = text_light[s + text_x] >> 1;	//make black bugs in top line

			s++;

			if (s >= text_x)
				s = 0;
		}
	}
}

void make_change (double dCurrentTime)
{
	unsigned int r = random () & 0xFFFF;

	r >>= 3;
	if (r < (text_x * text_y))
		text[r] += 133;	//random bugs

	r = random () & 0xFFFF;
	r >>= 7;
	if (r < text_x)
		if (text_light[r] != 0)
			text_light[r] = 255;	//white bugs

	scroll (dCurrentTime);
}

void hack_reshape (xstuff_t * XStuff)
{
	glViewport (0, 0, XStuff->windowWidth, XStuff->windowHeight);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	//gluPerspective (45.0f, (GLfloat) XStuff->windowWidth / (GLfloat) XStuff->windowHeight, 0.1f, 150.0f);
	glFrustum(-_text_x, _text_x, -_text_y, _text_y, -Z_Off - Z_Depth, -Z_Off);

	glMatrixMode (GL_MODELVIEW);
}

void load_texture ()
{
	long a;

	LOAD_TEXTURE (font, cfont, cfont_compressedsize, cfont_size)

	for (a = 0; a < 131072; a++) {
		if ((a >> 9) & 2)
			font[a] = font[a];
		else
			font[a] = font[a] >> 1;
	}
}

void make_text ()
{
	long a;
	unsigned int r;

	for (a = 0; a < (text_x * text_y); a++) {
		r = random () & 0xFFFF;
		text[a] = r;
	}

	for (a = 0; a < text_x; a++)
		speed[a] = random () & 1;
}

void ourBuildTextures ()
{
	GLenum gluerr;

	glBindTexture (GL_TEXTURE_2D, 1);

	if ((gluerr = gluBuild2DMipmaps (GL_TEXTURE_2D, GL_RGBA8, 512, 256, GL_GREEN, GL_UNSIGNED_BYTE, (void *)font))) {
		fprintf (stderr, "GLULib%s\n", gluErrorString (gluerr));
		exit (-1);
	}

	glBindTexture (GL_TEXTURE_2D, 2);

	if ((gluerr = gluBuild2DMipmaps (GL_TEXTURE_2D, GL_RGBA8, 512, 256, GL_LUMINANCE, GL_UNSIGNED_BYTE, (void *)font))) {
		fprintf (stderr, "GLULib%s\n", gluErrorString (gluerr));
		exit (-1);
	}

	FREE_TEXTURE (font)

	glBindTexture (GL_TEXTURE_2D, 3);

	if ((gluerr = gluBuild2DMipmaps (GL_TEXTURE_2D, GL_RGBA8, 4, 4, GL_LUMINANCE, GL_UNSIGNED_BYTE, (void *)flare))) {
		fprintf (stderr, "GLULib%s\n", gluErrorString (gluerr));
		exit (-1);
	}

	// Some pretty standard settings for wrapping and filtering.
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// We start with GL_DECAL mode.
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
}

void hack_init (xstuff_t * XStuff)	// Called right after the window is created, and OpenGL is initialized.
{
	load_texture ();
	make_text ();

	MagickWandGenesis ();

	ourBuildTextures ();

	// Buffers for drawing the bulk of the characters using arrays
	pts = (float *)malloc(text_y * text_x * 3 * 4 * sizeof(float));
	texcoords = (float *)malloc(text_y * text_x * 2 * 4 * sizeof(float));
	colors = (unsigned char *)malloc(text_y * text_x * 4 * 4 * sizeof(unsigned char));

	glVertexPointer(3, GL_FLOAT, 0, pts);
	glEnableClientState(GL_VERTEX_ARRAY);

	glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);
	glEnableClientState(GL_COLOR_ARRAY);

	glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	// Color to clear color buffer to.
	glClearColor (0.0f, 0.0f, 0.0f, 0.0f);

	// Depth to clear depth buffer to; type of test.
	glClearDepth (1.0);
	glDepthFunc (GL_LESS);

	// Enables Smooth Color Shading; try GL_FLAT for (lack of) fun.
	glShadeModel (GL_SMOOTH);

	// Set up a light, turn it on.
	glLightfv (GL_LIGHT1, GL_POSITION, Light_Position);
	glLightfv (GL_LIGHT1, GL_AMBIENT, Light_Ambient);
	glLightfv (GL_LIGHT1, GL_DIFFUSE, Light_Diffuse);
	glEnable (GL_LIGHT1);

	// A handy trick -- have surface material mirror the color.
	glColorMaterial (GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable (GL_COLOR_MATERIAL);

	hack_reshape (XStuff);
}

void hack_cleanup (xstuff_t * XStuff)
{
	if (imageLoadingThread) {
		// Try acquiring the lock first to make sure the thread is asleep and signallable. Otherwise the signal will miss the other thread and the pthread_join will hang.
		pthread_mutex_lock(next_pic_mutex);

		exiting = 1;
		pthread_cond_signal(next_pic_cond);

		pthread_mutex_unlock(next_pic_mutex);

		pthread_join(*imageLoadingThread, NULL);
	}

	free(pts);
	free(texcoords);
	free(colors);
}

void hack_draw (xstuff_t * XStuff, double currentTime, float frameTime)
{
	glBindTexture (GL_TEXTURE_2D, 1);
	glEnable (GL_BLEND);
	glEnable (GL_TEXTURE_2D);

	glDisable (GL_LIGHTING);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE);
	glDisable (GL_DEPTH_TEST);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
	glTranslatef (0.0f, 0.0f, Z_Off);

	// Clear the color and depth buffers.
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (exit_mode)
		exit_angle += 0.08;
	else
		exit_angle /= 1.1;

	if (texture_add > 0)
		texture_add /= 1.05;

	glRotatef (exit_angle, r1, r2, r3);

	// OK, let's start drawing our planer quads.
	draw_text ();

	glBindTexture (GL_TEXTURE_2D, 2);
	glBegin (GL_QUADS);
	draw_illuminatedtext ();
	glEnd ();

	glBindTexture (GL_TEXTURE_2D, 3);
	glBegin (GL_QUADS);
	draw_flares ();
	glEnd ();

	make_change (currentTime);

	glLoadIdentity ();
	glMatrixMode (GL_PROJECTION);
}

void hack_handle_opts (int argc, char **argv)
{
	while (1) {
		int c;

#ifdef HAVE_GETOPT_H
		static struct option long_options[] = {
			{"help", 0, 0, 'h'},
			DRIVER_OPTIONS_LONG 
			{"images", 1, 0, 'i'},
			{"scene_interval", 1, 0, 's'},
			{"matrix_interval", 1, 0, 'm'},
			{"matrix_width", 1, 0, 'X'},
			{"matrix_height", 1, 0, 'Y'},
			{"contrast", 1, 0, 'c'},
			{0, 0, 0, 0}
		};

		c = getopt_long (argc, argv, DRIVER_OPTIONS_SHORT "hs:m:X:Y:c:i:", long_options, NULL);
#else
		c = getopt (argc, argv, DRIVER_OPTIONS_SHORT "hs:m:X:Y:c:i:");
#endif
		if (c == -1)
			break;

		switch (c) {
			DRIVER_OPTIONS_CASES case 'h':printf ("%s:"
#ifndef HAVE_GETOPT_H
							      " Not built with GNU getopt.h, long options *NOT* enabled."
#endif
							      "\n" DRIVER_OPTIONS_HELP "\t--images/-i <arg>\n\t--scene_interval/-s <arg>\n" "\t--matrix_interval/-m <arg>\n"
								  "\t--matrix_width/-X <arg>\n" "\t--matrix_height/-Y <arg>\n" "\t--contrast/-c <arg>\n", argv[0]);

			exit (1);

		case 'i':
			{
				struct stat fileStat;

				if (!stat(optarg, (struct stat *)&fileStat)) {
					if (S_ISDIR(fileStat.st_mode)) {
						imageDir = opendir (optarg);
						if (imageDir) {
							dirName = (char *)malloc (strlen (optarg) + 1);
							strcpy (dirName, optarg);
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
		case 's':
			cycleScene = strtol_minmaxdef (optarg, 10, 0, 120, 1, 5, "--scene_interval: ");
			break;
		case 'm':
			cycleMatrix = strtol_minmaxdef (optarg, 10, 3, 120, 1, 5, "--matrix_interval: ");
			break;
		case 'X':
			text_x = strtol_minmaxdef (optarg, 10, 40, MAX_TEXT_X, 1, 90, "--matrix_width: ");
			break;
		case 'Y':
			text_y = strtol_minmaxdef (optarg, 10, 30, MAX_TEXT_Y, 1, 70, "--matrix_height: ");
			break;
		case 'c':
			contrast = strtol_minmaxdef (optarg, 10, 0, 100, 1, 75, "--contrast: ") / 100.0f;
			break;
		}
	}

#ifdef HAVE_IMAGEMAGICK
	// If contrast is unset, set it to 0.75 for external images, 1 for internal.
	if (contrast == -1) {
		if (dirName) 
			contrast = 0.75;
		else
			contrast = 1;
	}
#else
	// If contrast is unset, set it to 1.
	if (contrast == -1)
		contrast = 1;

	if (cycleScene > 0) {
		if ((text_x != 90) || (text_y != 70)) {
			printf("%s: Not built with ImageMagick, can not scale images. Resetting matrix resolution to 90x70.\n", argv[0]);

			text_x = 90;
			text_y = 70;
		}
	}
#endif
}

