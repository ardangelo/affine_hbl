#include <limits.h>
#include <stdio.h>

#include <tonc.h>

#include <utility>

#include "mode7.hpp"

#include "gfx/fanroom.h"
#include "gfx/bgpal.h"

#define DEBUG_MODE
#ifndef DEBUG_MODE
#define DEBUGFMT(fmt, ...)
#else
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#define DEBUGFMT(fmt, ...) do {	  \
		sprintf(nocash_buffer, fmt, __VA_ARGS__); \
		nocash_message(); \
	} while (0)
#endif


/* block mappings */
#define M7_CBB 0
#define FLOOR_SBB 24
#define FLOOR_PRIO 2
#define WALL_PRIO 3

/* m7 globals */

M7::Camera cam(fp8{M7::k::viewTop} / fp0{M7::k::focalLength});
M7::Layer floorLayer(
	M7_CBB,    fanroomTiles,
	FLOOR_SBB, fanroomMap,
	BG_AFF_128x128, FLOOR_PRIO);
M7::Level fanLevel(cam, floorLayer);

/* implementations */

void init_map() {
	/* extract main bg */
	LZ77UnCompVram(bgPal, pal_bg_mem);

	/* setup shadow fade */
	REG_BLDCNT = BLD_BUILD(BLD_BG2, BLD_BACKDROP, 3);
	REG_WININ  = WININ_BUILD(WIN_BG2 | WIN_BLD, 0);
	REG_WIN0V  = M7::k::screenHeight;
	pal_bg_mem[0] = CLR_GRAY / 2;

	/* registers */
	REG_DISPCNT = DCNT_MODE2 | DCNT_BG2 | DCNT_OBJ | DCNT_OBJ_1D;
}

auto static constexpr OMEGA = int32_t{0xFF};
auto static constexpr VEL_X = fp0{ 1};
auto static constexpr VEL_Z = fp0{-1};
auto input_game() {
	key_poll();

	auto const dPos = v0
		{ VEL_X * fp0{key_tri_shoulder()} // strafe
		, VEL_Z * fp0{key_tri_vert()}     // forwards / backwards
	};

	/* rotate */
	auto const dTheta = int32_t{OMEGA * key_tri_horz()};

	return std::make_pair(dPos, dTheta);
}

int main() {
	init_map();

	/* irqs */
	irq_init(nullptr);
	irq_add(II_HBLANK, (fnptr)m7_hbl);
	irq_add(II_VBLANK, nullptr);

	while (1) {
		VBlankIntrWait();

		/* update camera based on input */
		auto const [dPos, dTheta] = input_game();
		cam.translate(dPos);
		cam.rotate(dTheta);

		/* update affine matrices */
		fanLevel.prepAffines();

#if 0
		DEBUGFMT("cam:(%ld, %ld, %ld)", cam.pos.x, cam.pos.y, cam.pos.z);
		DEBUGFMT("lam[0]:%f", float(fanLevel.layer.bgaff[0].pa));
#endif
	}

	return 0;

}
