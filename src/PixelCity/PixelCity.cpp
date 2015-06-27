/*
 * Copyright (C) 2009 Shamus Young
 * Ported to Linux by Tugrul Galatali <tugrul@galatali.com>
 *
 * PixelCity is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Plasma is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define RENDER_DISTANCE     1280
#define COLOR_CYCLE_TIME    10000 //milliseconds
#define COLOR_CYCLE         (COLOR_CYCLE_TIME / 4)
#define BLOOM_SCALING       0.07f

#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include "driver.h"

#ifdef HAVE_GLC
#include <GL/glc.h>
#endif

#include "glTypes.h"
#include "Entity.h"
#include "Car.h"
#include "Camera.h"
#include "GetTickCount.h"
#include "Light.h"
#include "Macro.h"
#include "Random.h"
#include "PixelCity.h"
#include "Sky.h"
#include "Texture.h"
#include "Visible.h"
#include "World.h"

const char *hack_name = "pixelcity";

enum
{
  EFFECT_NONE,
  EFFECT_BLOOM,
  EFFECT_BLOOM_RADIAL,
  EFFECT_COLOR_CYCLE,
  EFFECT_GLASS_CITY,
};

static float            render_aspect;
static float            fog_distance;
static int              render_width;
static int              render_height;
static bool             letterbox;
static int              letterbox_offset;
static int              effect;
static bool             show_wireframe;
static bool             flat;
static bool             show_fog;

#ifdef HAVE_GLC
static GLint            glc_ctx;

#define FONT_COUNT          (sizeof (fonts) / sizeof (struct glFont))
#define FONT_SIZE           (LOGO_PIXELS - LOGO_PIXELS / 8)
#define MAX_TEXT            256

struct glFont
{
  const char *name;
  GLint id;
} fonts[] =
{
  { "Courier New",      0 },
  { "Arial",            0 },
  { "Times New Roman",  0 },
  { "Arial Black",      0 },
  { "Impact",           0 },
  { "Verdana",          0 },
  { "DejaVu Sans Mono", 0 },
};
#endif

/*-----------------------------------------------------------------------------

  Draw a clock-ish progress.. widget... thing.  It's cute.

-----------------------------------------------------------------------------*/

static void do_progress (float center_x, float center_y, float radius, float opacity, float progress)
{

  int     i;
  int     end_angle;
  float   inner, outer;
  float   angle;
  float   s, c;
  float   gap;

  //Outer Ring
  gap = radius * 0.05f;
  outer = radius;
  inner = radius - gap * 2;
  glColor4f (1,1,1, opacity);
  glBegin (GL_QUAD_STRIP);
  for (i = 0; i <= 360; i+= 15) {
    angle = (float)i * DEGREES_TO_RADIANS;
    s = sinf (angle);
    c = -cosf (angle);
    glVertex2f (center_x + s * outer, center_y + c * outer);
    glVertex2f (center_x + s * inner, center_y + c * inner);
  }
  glEnd ();
  //Progress indicator
  glColor4f (1,1,1, opacity);
  end_angle = (int)(360 * progress);
  outer = radius - gap * 3;
  glBegin (GL_TRIANGLE_FAN);
  glVertex2f (center_x, center_y);
  for (i = 0; i <= end_angle; i+= 3) {
    angle = (float)i * DEGREES_TO_RADIANS;
    s = sinf (angle);
    c = -cosf (angle);
    glVertex2f (center_x + s * outer, center_y + c * outer);
  }
  glEnd ();
  //Tic lines
  glLineWidth (2.0f);
  outer = radius - gap * 1;
  inner = radius - gap * 2;
  glColor4f (0,0,0, opacity);
  glBegin (GL_LINES);
  for (i = 0; i <= 360; i+= 15) {
    angle = (float)i * DEGREES_TO_RADIANS;
    s = sinf (angle);
    c = -cosf (angle);
    glVertex2f (center_x + s * outer, center_y + c * outer);
    glVertex2f (center_x + s * inner, center_y + c * inner);
  }
  glEnd ();

}

/*-----------------------------------------------------------------------------

-----------------------------------------------------------------------------*/

static void do_effects (int type)
{

  float           hue1, hue2, hue3, hue4;
  GLrgba          color;
  float           fade;
  int             radius;
  int             x, y;
  int             i;
  int             bloom_radius;
  int             bloom_step;

  fade = WorldFade ();
  bloom_radius = 15;
  bloom_step = bloom_radius / 3;
  if (!TextureReady ())
    return;
  //Now change projection modes so we can render full-screen effects
  glMatrixMode (GL_PROJECTION);
  glPushMatrix ();
  glLoadIdentity ();
  glOrtho (0, render_width, render_height, 0, 0.1f, 2048);
  glMatrixMode (GL_MODELVIEW);
  glPushMatrix ();
  glLoadIdentity();
  glTranslatef(0, 0, -1.0f);
  glDisable (GL_CULL_FACE);
  glDisable (GL_FOG);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  //Render full-screen effects
  glBlendFunc (GL_ONE, GL_ONE);
  glEnable (GL_TEXTURE_2D);
  glDisable(GL_DEPTH_TEST);
  glDepthMask (false);
  glBindTexture(GL_TEXTURE_2D, TextureId (TEXTURE_BLOOM));
  switch (type) {
  case EFFECT_BLOOM_RADIAL:
    //Psychedelic bloom
    glEnable (GL_BLEND);
    glBegin (GL_QUADS);
    color = WorldBloomColor () * BLOOM_SCALING * 2;
    glColor3fv (&color.red);
    for (i = 0; i <= 100; i+=10) {
      glTexCoord2f (0, 0);  glVertex2i (-i, i + render_height);
      glTexCoord2f (0, 1);  glVertex2i (-i, -i);
      glTexCoord2f (1, 1);  glVertex2i (i + render_width, -i);
      glTexCoord2f (1, 0);  glVertex2i (i + render_width, i + render_height);
    }
    glEnd ();
    break;
  case EFFECT_COLOR_CYCLE:
    //Oooh. Pretty colors.  Tint the scene according to screenspace.
    hue1 = (float)(GetTickCount () % COLOR_CYCLE_TIME) / COLOR_CYCLE_TIME;
    hue2 = (float)((GetTickCount () + COLOR_CYCLE) % COLOR_CYCLE_TIME) / COLOR_CYCLE_TIME;
    hue3 = (float)((GetTickCount () + COLOR_CYCLE * 2) % COLOR_CYCLE_TIME) / COLOR_CYCLE_TIME;
    hue4 = (float)((GetTickCount () + COLOR_CYCLE * 3) % COLOR_CYCLE_TIME) / COLOR_CYCLE_TIME;
    glBindTexture(GL_TEXTURE_2D, 0);
    glEnable (GL_BLEND);
    glBlendFunc (GL_ONE, GL_ONE);
    glBlendFunc (GL_DST_COLOR, GL_SRC_COLOR);
    glBegin (GL_QUADS);
    color = glRgbaFromHsl (hue1, 1.0f, 0.6f);
    glColor3fv (&color.red);
    glTexCoord2f (0, 0);  glVertex2i (0, render_height);
    color = glRgbaFromHsl (hue2, 1.0f, 0.6f);
    glColor3fv (&color.red);
    glTexCoord2f (0, 1);  glVertex2i (0, 0);
    color = glRgbaFromHsl (hue3, 1.0f, 0.6f);
    glColor3fv (&color.red);
    glTexCoord2f (1, 1);  glVertex2i (render_width, 0);
    color = glRgbaFromHsl (hue4, 1.0f, 0.6f);
    glColor3fv (&color.red);
    glTexCoord2f (1, 0);  glVertex2i (render_width, render_height);
    glEnd ();
    break;
  case EFFECT_BLOOM:
    //Simple bloom effect
    glBegin (GL_QUADS);
    color = WorldBloomColor () * BLOOM_SCALING;
    glColor3fv (&color.red);
    for (x = -bloom_radius; x <= bloom_radius; x += bloom_step) {
      for (y = -bloom_radius; y <= bloom_radius; y += bloom_step) {
        if (abs (x) == abs (y) && x)
          continue;
        glTexCoord2f (0, 0);  glVertex2i (x, y + render_height);
        glTexCoord2f (0, 1);  glVertex2i (x, y);
        glTexCoord2f (1, 1);  glVertex2i (x + render_width, y);
        glTexCoord2f (1, 0);  glVertex2i (x + render_width, y + render_height);
      }
    }
    glEnd ();
    break;
  }
  //Do the fade to / from darkness used to hide scene transitions
//printf("%d %d %f\n", TextureReady (), EntityReady (), fade);
  if (LOADING_SCREEN) {
    if (fade > 0.0f) {
      glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable (GL_BLEND);
      glDisable (GL_TEXTURE_2D);
      glColor4f (0, 0, 0, fade);
      glBegin (GL_QUADS);
      glVertex2i (0, 0);
      glVertex2i (0, render_height);
      glVertex2i (render_width, render_height);
      glVertex2i (render_width, 0);
      glEnd ();
    }
    if (TextureReady () && !EntityReady () && fade != 0.0f) {
      radius = render_width / 16;
      do_progress ((float)render_width / 2, (float)render_height / 2, (float)radius, fade, EntityProgress ());
    }
  }
  glPopMatrix ();
  glMatrixMode (GL_PROJECTION);
  glPopMatrix ();
  glMatrixMode (GL_MODELVIEW);
  glEnable(GL_DEPTH_TEST);

}


/*-----------------------------------------------------------------------------

-----------------------------------------------------------------------------*/

void hack_reshape (xstuff_t * XStuff)
{

  float     fovy;

  render_width = XStuff->windowWidth;
  render_height = XStuff->windowHeight;

  if (letterbox) {
    letterbox_offset = render_height / 6;
    render_height = render_height - letterbox_offset * 2;
  } else
    letterbox_offset = 0;
  //render_aspect = (float)render_height / (float)render_width;
  glViewport (0, letterbox_offset, render_width, render_height);
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  render_aspect = (float)render_width / (float)render_height;
  fovy = 60.0f;
  if (render_aspect > 1.0f)
    fovy /= render_aspect;
  gluPerspective (fovy, render_aspect, 0.1f, RENDER_DISTANCE);
  glMatrixMode (GL_MODELVIEW);

}

/*-----------------------------------------------------------------------------

-----------------------------------------------------------------------------*/

void hack_init (xstuff_t * XStuff)
{
  //Load the fonts for printing debug info to the window.
#ifdef HAVE_GLC
  glc_ctx = glcGenContext ();
  glcContext (glc_ctx);

  for (unsigned int ii = 0; ii < FONT_COUNT; ii++) {
    fonts[ii].id = glcGenFontID();
    if (glcNewFontFromFamily(fonts[ii].id, fonts[ii].name) != fonts[ii].id) {
printf("%s\n", fonts[ii].name);
      glcDeleteFont(fonts[ii].id);
      fonts[ii].id = 0;
    }
  }

  glcScale (FONT_SIZE, FONT_SIZE);
#endif

  hack_reshape (XStuff);

  TextureInit ();
  WorldInit ();
}

bool RenderBloom ()
{
  return effect == EFFECT_BLOOM || effect == EFFECT_BLOOM_RADIAL || effect == EFFECT_COLOR_CYCLE;
}

int RenderMaxTextureSize ()
{
  int mts;

  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &mts);
  mts = MIN (mts, render_width);
  return MIN (mts, render_height);
}

bool RenderFlat ()
{
  return flat;
}

float RenderFogDistance ()
{
  return fog_distance;
}

void RenderFogFX (float scalar)
{

  if (scalar >= 1.0f) {
    glDisable (GL_FOG);
    return;
  }
  glFogf (GL_FOG_START, 0.0f);
  glFogf (GL_FOG_END, fog_distance * 2.0f * scalar);
  glEnable (GL_FOG);

}

#ifdef HAVE_GLC
void RenderPrint (int x, int y, int font, GLrgba color, const char *fmt, ...)
{
  char text[MAX_TEXT];
  va_list ap;

  text[0] = 0;
  if (fmt == NULL) return;

  va_start(ap, fmt);
  vsprintf(text, fmt, ap);
  va_end(ap);

  for (; fonts[font % FONT_COUNT].id == 0; ++font);
  glcFont(fonts[font % FONT_COUNT].id);

  glColor3fv (&color.red);
  glRasterPos2i (x, y);
  glcRenderString(text);
}
#endif

void hack_draw (xstuff_t * XStuff, double currentTime, float frameTime)
{

  GLvector        pos;
  GLvector        angle;
  GLrgba          color;
  int             elapsed;

  incrementTickCount(frameTime * 1000);

  CameraUpdate ();
  EntityUpdate ();
  WorldUpdate ();
  TextureUpdate ();
  VisibleUpdate ();
  CarUpdate ();

  glViewport (0, 0, render_width, render_height);
  glDepthMask (true);
  glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  if (letterbox)
    glViewport (0, letterbox_offset, render_width, render_height);
  if (LOADING_SCREEN && TextureReady () && !EntityReady ()) {
    do_effects (EFFECT_NONE);
    return;
  }
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glShadeModel(GL_SMOOTH);
  glFogi (GL_FOG_MODE, GL_LINEAR);
  glDepthFunc(GL_LEQUAL);
  glEnable (GL_CULL_FACE);
  glCullFace (GL_BACK);
  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glMatrixMode (GL_TEXTURE);
  glLoadIdentity();
  glMatrixMode (GL_MODELVIEW);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glLoadIdentity();
  glLineWidth (1.0f);
  pos = CameraPosition ();
  angle = CameraAngle ();
  glRotatef (angle.x, 1.0f, 0.0f, 0.0f);
  glRotatef (angle.y, 0.0f, 1.0f, 0.0f);
  glRotatef (angle.z, 0.0f, 0.0f, 1.0f);
  glTranslatef (-pos.x, -pos.y, -pos.z);
  glEnable (GL_TEXTURE_2D);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  //Render all the stuff in the whole entire world.
  glDisable (GL_FOG);
  SkyRender ();
  if (show_fog) {
    glEnable (GL_FOG);
    glFogf (GL_FOG_START, fog_distance - 100);
    glFogf (GL_FOG_END, fog_distance);
    color = glRgba (0.0f);
    glFogfv (GL_FOG_COLOR, &color.red);
  }
  WorldRender ();
  if (effect == EFFECT_GLASS_CITY) {
    glDisable (GL_CULL_FACE);
    glEnable (GL_BLEND);
    glBlendFunc (GL_ONE, GL_ONE);
    glDepthFunc (false);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode (GL_TEXTURE);
    glTranslatef ((pos.x + pos.z) / SEGMENTS_PER_TEXTURE, 0, 0);
    glMatrixMode (GL_MODELVIEW);
  } else {
    glEnable (GL_CULL_FACE);
    glDisable (GL_BLEND);
  }
  EntityRender ();
  if (!LOADING_SCREEN) {
    elapsed = 3000 - WorldSceneElapsed ();
    if (elapsed >= 0 && elapsed <= 3000) {
      RenderFogFX ((float)elapsed / 3000.0f);
      glDisable (GL_TEXTURE_2D);
      glEnable (GL_BLEND);
      glBlendFunc (GL_ONE, GL_ONE);
      EntityRender ();
    }
  }
  if (EntityReady ())
    LightRender ();
  CarRender ();
  if (show_wireframe) {
    glDisable (GL_TEXTURE_2D);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    EntityRender ();
  }
  do_effects (effect);
}

void hack_cleanup (xstuff_t * XStuff)
{
  TextureTerm ();
  WorldTerm ();
}

void hack_handle_opts (int argc, char **argv)
{
	letterbox = 0;
	show_wireframe = 0;
	flat = 0;
	effect = EFFECT_BLOOM;
	fog_distance = WORLD_HALF;
	show_fog = 1;

	while (1) {
		int c;

#ifdef HAVE_GETOPT_H
		static struct option long_options[] = {
			{"help", 0, 0, 'h'},
			DRIVER_OPTIONS_LONG
			{"effect_none", 0, 0, 'n'},
			{"effect_bloom", 0, 0, 'b'},
			{"effect_bloom_radial", 0, 0, 'B'},
			{"effect_glass", 0, 0, 'g'},
			{"effect_color_cycle", 0, 0, 'c'},
			{"wireframe", 0, 0, 'w'},
			{"letterbox", 0, 0, 'l'},
			{"no-fog", 0, 0, 'F'},
			{0, 0, 0, 0}
		};

		c = getopt_long (argc, argv, DRIVER_OPTIONS_SHORT "hnbBgcwlF", long_options, NULL);
#else
		c = getopt (argc, argv, DRIVER_OPTIONS_SHORT "hnbBgcwlF");
#endif
		if (c == -1)
			break;

		switch (c) {
			DRIVER_OPTIONS_CASES case 'h':printf ("%s:"
#ifndef HAVE_GETOPT_H
							      " Not built with GNU getopt.h, long options *NOT* enabled."
#endif
							      "\n" DRIVER_OPTIONS_HELP
							      "\t--effect_none/-n\n"
							      "\t--effect_bloom/-b\n"
							      "\t--effect_bloom_radial/-B\n"
							      "\t--effect_glass/-g\n"
							      "\t--effect_color_cycle/-c\n"
							      "\t--wireframe/-w\n"
							      "\t--letterbox/-l\n"
							      "\t--no-fog/-F\n"
							      , argv[0]);
			exit (1);
		case 'n':
			effect = EFFECT_NONE;
			break;
		case 'b':
			effect = EFFECT_BLOOM;
			break;
		case 'B':
			effect = EFFECT_BLOOM_RADIAL;
			break;
		case 'g':
			effect = EFFECT_GLASS_CITY;
			break;
		case 'c':
			effect = EFFECT_COLOR_CYCLE;
			break;
		case 'w':
			show_wireframe = 1;
			break;
		case 'l':
			letterbox = 1;
			break;
		case 'F':
			show_fog = 0;
			break;
		}
	}
}
