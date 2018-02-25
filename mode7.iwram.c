#include <tonc.h>

#include "mode7.h"

void m7_hbl() {

	FIXED lam = cam_pos.y * lu_div(REG_VCOUNT) >> 12;
	FIXED lcf = lam * g_cosf >> 8;
	FIXED lsf = lam * g_sinf >> 8;

	/* rotations */
	REG_BG2PA = lcf >> 4;
	REG_BG2PC = lsf >> 4;

	/* offsets */
	FIXED lxr, lyr;

	/* horizontal */
	lxr = 120 * (lcf >> 4);
	lyr = (M7_D * lsf) >> 4;
	REG_BG2X = cam_pos.x - lxr + lyr;

	/* vertical */
	lxr = 120 * (lsf >> 4);
	lyr = (M7_D * lcf) >> 4;
	REG_BG2Y = cam_pos.z - lxr - lyr;
}

