#include <limits.h>
#include <tonc.h>

#include "mode7.h"

M7Camera::M7Camera(const FixedPixel& f) : fov(f) {
	pos = { FixedPixel(8), FixedPixel(2), FixedPixel(2) };
	theta = FixedAngle(0);
	phi = FixedAngle(0);

	u = {FixedPixel(1), FixedPixel(0), FixedPixel(0)};
	v = {FixedPixel(0), FixedPixel(1), FixedPixel(0)};
	w = {FixedPixel(0), FixedPixel(0), FixedPixel(1)};
}

void M7Camera::rotate(const FixedAngle& th) {
	/* limited to fixpoint range */
	FIXED theta = th.get() & 0xFFFF;

	FIXED ct = lu_cos(theta) >> 4;
	FIXED st = lu_sin(theta) >> 4;

	/* camera x-axis */
	u = {FixedPixel(1), FixedPixel(0), FixedPixel(0)};

	/* camera y-axis */
	v = {FixedPixel(0), FixedPixel(ct), FixedPixel(st * -1)};

	/* camera z-axis */
	w = {FixedPixel(0), FixedPixel(st), FixedPixel(ct)};
}

M7Map::M7Map(u16 bgc, const int *bl, const Vector<Block>& bD, const Block *extents, FixedPixel *eW, FixedPixel *eO, const FixedPixel& fov) :
	bgcnt(bgc), blocks(bl), blocksDim(bD),
	extentWidths(eW), extentOffs(eO) {

	/* calculate texture dimensions */
	textureDim = {
		FixedPixel(bD.x.get() * PIX_PER_BLOCK),
		FixedPixel(bD.y.get() * PIX_PER_BLOCK), 
		FixedPixel(bD.z.get() * PIX_PER_BLOCK)};

	/* precalculate extent widths */
	for (int i = 0; i < blocksDim.y; i++) {
		extentWidths[i] = fxmul(
			int2fx(
				(extents[i * 2 + 1] - extents[i * 2])
				* PIX_PER_BLOCK), // normal extent width
			fov); // adjust for fov
		extentOffs[i] = (int2fx(blocksDim.x + extents[i * 2])) / 2;
	}
}

M7Level::M7Level(M7Camera& c, const M7Map *m1, const M7Map *m2) : cam(c) {
	maps[0] = m1;
	maps[1] = m2;

	/* apply background control regs */
	REG_BG3CNT = m1->bgcnt;
	REG_BG2CNT = m2->bgcnt;
}

void M7Level::translateLocal(const Vector<FixedPixel>& dir) {
	const M7Map *m = maps[0];

	Vector<FixedPixel> p = cam.pos;
	p.x += (cam.u.x * dir.x + cam.v.x * dir.y + cam.w.x * dir.z) >> 8;
	p.y += ( 0              + cam.v.y * dir.y + cam.w.y * dir.z) >> 8;
	p.z += (cam.u.z * dir.x + cam.v.z * dir.y + cam.w.z * dir.z) >> 8;

	const int mapX = fx2int(p.x);
	const int mapY = fx2int(p.y);
	const int mapZ = fx2int(p.z);

	/* update x */
	if ((0 <= mapX) && (mapX <= m->blocksDim.x)) {
		cam.pos.x = p.x;
	}

	/* check y / z wall collision independently */
	if ((mapY >= 0) && (mapY < m->blocksDim.y)) {
		if (m->blocks[(mapY * m->blocksDim.z) + fx2int(cam.pos.z)] == 0) {
			cam.pos.y = p.y;
		}
	}
	if ((mapX >= 0) && (mapZ < m->blocksDim.z)) {
		if (m->blocks[(fx2int(cam.pos.y) * m->blocksDim.z) + mapZ] == 0) {
			cam.pos.z = p.z;
		}
	}
}
