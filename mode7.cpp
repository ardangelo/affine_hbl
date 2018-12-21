#include <limits.h>
#include <tonc.h>

#include "mode7.h"

M7::Camera::Camera(fp8 const& fov_)
	: pos{120, 160, 80}
	, theta{0x3FFF}
	, phi{0x0}
	, u{1, 0, 0}
	, v{0, 1, 0}
	, w{0, 0, 1}
	, fov{fov_} {}

void M7::Camera::translate(v0 const& dPos) {
	pos += dPos;
}

void M7::Camera::rotate(int32_t const dTheta) {
	/* limited to fixpoint range */
	theta += dTheta;
	theta &= 0xFFFF;

	auto const cos_theta = luCos(theta);
	auto const sin_theta = luSin(theta);

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

void M7::Level::translateLocal(v0 const& dPos) {
	auto pos = cam.pos;
	pos.x += (cam.u.x * dPos.x + cam.v.x * dPos.y + cam.w.x * dPos.z);
	pos.y += (0                + cam.v.y * dPos.y + cam.w.y * dPos.z);
	pos.z += (cam.u.z * dPos.x + cam.v.z * dPos.y + cam.w.z * dPos.z);
}
