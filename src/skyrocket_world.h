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

#ifndef WORLD_H
#define WORLD_H

#define STARMESH 12
#define STARTEXSIZE 512
#define MOONGLOWTEXSIZE 128
#define CLOUDMESH 48

// For building mountain sillohettes in sunset
void makeHeights (int first, int last, int *h);
void initWorld ();
void updateWorld ();
void drawWorld ();
void cleanupWorld ();

#endif
