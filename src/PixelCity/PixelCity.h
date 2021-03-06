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

#ifndef _PIXELCITY_H_
#define _PIXELCITY_H_

#define LOADING_SCREEN 1

bool  RenderBloom ();
bool  RenderFlat ();
float RenderFogDistance ();
bool  RenderFog ();
void  RenderFogFX (float scalar);
int   RenderMaxTextureSize ();
bool  RenderWireframe ();

void  RenderPrint (int x, int y, int font, GLrgba color, const char *fmt, ...);

#endif
