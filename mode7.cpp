#include <limits.h>
#include <tonc.h>

#include "mode7.h"

M7Camera::M7Camera(FIXED f) {
	pos = { 8 << FIX_SHIFT, 2 << FIX_SHIFT, 2 << FIX_SHIFT };
	theta = 0x0000;
	phi = 0x0;

	u = {1 << FIX_SHIFT, 0, 0};
	v = {0, 1 << FIX_SHIFT, 0};
	w = {0, 0, 1 << FIX_SHIFT};
	fov = f;
}

void M7Camera::rotate(FIXED th) {
	/* limited to fixpoint range */
	th &= 0xFFFF;

	FIXED ct = lu_cos(th) >> 4;
	FIXED st = lu_sin(th) >> 4;

	/* camera x-axis */
	u = {1 << 8, 0, 0};

	/* camera y-axis */
	v = {0, ct, -1 * st};

	/* camera z-axis */
	w = {0, st, ct};
}

M7Map::M7Map(u16 bgc, FIXED fov)
	: bgcnt(bgc) {}

M7Level::M7Level(M7Camera *c, M7Map *m1)
	: cam{c}
	, map{m1} {

	/* apply background control regs */
	REG_BG2CNT = m1->bgcnt;
}

void M7Level::translateLocal(const VECTOR *dir) {
	VECTOR p = cam->pos;
	p.x += (cam->u.x * dir->x + cam->v.x * dir->y + cam->w.x * dir->z) >> 8;
	p.y += ( 0                + cam->v.y * dir->y + cam->w.y * dir->z) >> 8;
	p.z += (cam->u.z * dir->x + cam->v.z * dir->y + cam->w.z * dir->z) >> 8;

	const int mapX = fx2int(p.x);
	const int mapY = fx2int(p.y);
	const int mapZ = fx2int(p.z);
}
