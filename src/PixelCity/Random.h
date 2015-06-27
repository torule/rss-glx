#ifndef _RANDOM_H_
#define _RANDOM_H_

#include <stdlib.h>

#define COIN_FLIP     (RandomVal (2) == 0)

inline long int RandomVal (long int range) { return range ? random() % range : 0; }
inline long int RandomVal (void) { return random(); }

#endif
