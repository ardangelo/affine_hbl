#ifndef MAIN_H_
#define MAIN_H_

#include <tonc.h>

/* mode 7 distance */
#define M7_D 160

extern VECTOR cam_pos;
extern u16 cam_phi;
extern FIXED g_cosf, g_sinf;

IWRAM_CODE void m7_hbl();

#endif