#include "GetTickCount.h"

static unsigned int tickCount = 0;

void incrementTickCount(const unsigned int deltaCount) {
	tickCount += deltaCount;
}

int GetTickCount() {
	return tickCount;
}
