#include <limits.h>
#include <tonc.h>

#include "mode7.hpp"

M7::Camera::Camera(fp8 const fov_)
	: pos{120, 80}
	, theta{0x3FFF}
	, phi{0x0}
	, fov{fov_} {}

void M7::Camera::translate(v0 const& dPos) {
	pos += dPos;
}

void M7::Camera::rotate(int32_t const dTheta) {
	/* limited to fixpoint range */
	theta += dTheta;
	theta &= 0xFFFF;
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

