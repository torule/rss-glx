/*
 * Copyright (C) 2002  Jeremie Allard (Hufo / N.A.A.)
 *
 * Note: Credits for the 2d warping field fly to
 * Hugo Elias: http://freespace.virgin.net/hugo.elias
 * I've transposed to 3d.
 *
 * Ported to Linux by Tugrul Galatali <tugrul@galatali.com>
 *
 * hufo_smoke is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * hufo_smoke is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef FMOTION_H
#define FMOTION_H

#include <rsMath/rsVec.h>

struct PARTICLE {
        rsVec p, v;
};

bool FMotionInit ();
void FMotionQuit ();
void FMotionAnimate (const float &dt);
void FMotionWarp (rsVec &p, const float &dt);
void AffFMotion ();

#endif
