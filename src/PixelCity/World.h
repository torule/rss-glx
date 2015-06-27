#ifndef _WORLD_H_
#define _WORLD_H_

//Used in orienting roads and cars
enum
{
  NORTH,
  EAST,
  SOUTH,
  WEST
};

//Controls the density of cars.
#define CARS                500
//The "dead zone" along the edge of the world, with super-low detail.
#define WORLD_EDGE          200
//How often to rebuild the city
#define RESET_INTERVAL      120000 //milliseconds
//How long the screen fade takes when transitioning to a new city
#define FADE_TIME           1500 //milliseconds
//Debug ground texture that shows traffic lanes
#define SHOW_DEBUG_GROUND   0
//Controls the ammount of space available for buildings.
//Other code is wrtten assuming this will be a power of two.
#define WORLD_SIZE          1024
#define WORLD_HALF          (WORLD_SIZE / 2)

//Bitflags used to track how world space is being used.
#define CLAIM_ROAD          1
#define CLAIM_WALK          2
#define CLAIM_BUILDING      4
#define MAP_ROAD_NORTH      8
#define MAP_ROAD_SOUTH      16
#define MAP_ROAD_EAST       32
#define MAP_ROAD_WEST       64


GLrgba    WorldBloomColor ();
char      WorldCell (int x, int y);
GLrgba    WorldLightColor (unsigned index);
int       WorldLogoIndex ();
GLbbox    WorldHotZone ();
void      WorldInit (void);
float     WorldFade (void);
void      WorldRender ();
void      WorldReset (void);
int       WorldSceneBegin ();
int       WorldSceneElapsed ();
void      WorldTerm (void);
void      WorldUpdate (void);

#endif
