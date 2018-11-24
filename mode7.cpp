#include <limits.h>
#include <tonc.h>

#include "mode7.h"

M7::Camera::Camera(FIXED f)
	: pos{ 8 << FIX_SHIFT, 2 << FIX_SHIFT, 2 << FIX_SHIFT }
	, theta{0x0000}
	, phi{0x0}
	, u{1 << FIX_SHIFT, 0, 0}
	, v{0, 1 << FIX_SHIFT, 0}
	, w{0, 0, 1 << FIX_SHIFT}
	, fov{f} {}

void M7::Camera::translate(VECTOR const& dPos) {
	pos.x += dPos.x;
	pos.y += dPos.y;
	pos.z += dPos.z;
}

void M7::Camera::rotate(FIXED dTheta) {
	/* limited to fixpoint range */
	theta += dTheta;
	theta &= 0xFFFF;

	FIXED ct = lu_cos(dTheta) >> 4;
	FIXED st = lu_sin(dTheta) >> 4;

	/* camera x-axis */
	u = {1 << 8, 0, 0};

	/* camera y-axis */
	v = {0, ct, -1 * st};

	/* camera z-axis */
	w = {0, st, ct};
}

M7::Layer::Layer(
			size_t cbb, const unsigned int tiles[],
			size_t sbb, const unsigned short map[],
			size_t mapSize, size_t prio,
			FIXED fov)
	: bgcnt(BG_CBB(cbb) | BG_SBB(sbb) | mapSize | BG_PRIO(prio)) {

	LZ77UnCompVram(tiles, tile_mem[cbb]);
	LZ77UnCompVram(map,   se_mem[sbb]);
}

M7::Level::Level(M7::Camera const& cam_, M7::Layer& layer_)
	: cam{cam_}
	, layer{layer_} {

	/* apply background control regs */
	REG_BG2CNT = layer.bgcnt;
}

void M7::Level::translateLocal(VECTOR const& dir) {
	VECTOR p = cam.pos;
	p.x += (cam.u.x * dir.x + cam.v.x * dir.y + cam.w.x * dir.z) >> 8;
	p.y += (0               + cam.v.y * dir.y + cam.w.y * dir.z) >> 8;
	p.z += (cam.u.z * dir.x + cam.v.z * dir.y + cam.w.z * dir.z) >> 8;

	const int mapX = fx2int(p.x);
	const int mapY = fx2int(p.y);
	const int mapZ = fx2int(p.z);
}
