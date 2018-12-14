#include <limits.h>
#include <tonc.h>

#include "mode7.h"

M7::Camera::Camera(FPi32<8> const& fov_)
	: pos{8, 2, 2}
	, theta{0x0000}
	, phi{0x0}
	, u{1, 0, 0}
	, v{0, 1, 0}
	, w{0, 0, 1}
	, fov{fov_} {}

void M7::Camera::translate(Vector<0> const& dPos) {
	pos = pos + dPos;
}

void M7::Camera::rotate(FPi32<4> const& dTheta) {
	/* limited to fixpoint range */
	theta += dTheta;
	theta &= 0xFFFF;

	FPi32<8> const cos_theta = luCos(theta);
	FPi32<8> const sin_theta = luCos(theta);

	/* camera x-axis */
	u = {1, 0, 0};

	/* camera y-axis */
	v = {0, cos_theta, -1 * sin_theta};

	/* camera z-axis */
	w = {0, sin_theta, cos_theta};
}

M7::Layer::Layer(
			size_t cbb, const unsigned int tiles[],
			size_t sbb, const unsigned short map[],
			size_t mapSize, size_t prio)
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

void M7::Level::translateLocal(Vector<0> const& dir) {
	Vector p = cam.pos;
	p.x += (cam.u.x * dir.x + cam.v.x * dir.y + cam.w.x * dir.z);
	p.y += (0               + cam.v.y * dir.y + cam.w.y * dir.z);
	p.z += (cam.u.z * dir.x + cam.v.z * dir.y + cam.w.z * dir.z);
}
