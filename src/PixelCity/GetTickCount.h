#ifndef _GETTICKCOUNT_H_
#define _GETTICKCOUNT_H_

#ifdef __cplusplus
extern "C" {
#endif 

void incrementTickCount(const unsigned int deltaCount);
int GetTickCount(void);

#ifdef __cplusplus
}
#endif 

#endif
