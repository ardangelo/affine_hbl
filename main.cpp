#include <limits.h>
#include <stdio.h>

#include <tonc.h>

#include <utility>

#include "mode7.h"

#include "gfx/fanroom.h"
#include "gfx/bgpal.h"

#include "maps/fanroom.h"

#ifdef DEBUG
#define DEBUGFMT(fmt, ...)
#else
#include <stdlib.h>
char *dbg_str;
#define DEBUG(str) (nocash_puts(str))
#define DEBUGFMT(fmt, ...) do {	  \
		asprintf(&dbg_str, fmt, __VA_ARGS__); \
		nocash_puts(dbg_str); \
		free(dbg_str); \
	} while (0)
#endif

/* block mappings */
#define M7_CBB 0
#define FLOOR_SBB 24
#define FLOOR_PRIO 2
#define WALL_PRIO 3

/* m7 globals */

M7::Camera cam(FPi32<8>(M7::k::viewTop) / M7::k::focalLength);
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

FPi32<4> static constexpr OMEGA =   0xF;
FPi32<0> static constexpr VEL_X =   1;
FPi32<0> static constexpr VEL_Z =  -1;
auto input_game() {
	key_poll();

	Vector<0> dir
		{ .x = VEL_X * key_tri_shoulder() /* strafe */
		, .y = 0
		, .z = VEL_Z * key_tri_vert() /* forwards / backwards */
	};

	/* rotate */
	auto const theta = OMEGA * key_tri_horz();

	return std::make_pair(dir, theta);
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
		auto [dPos, dTheta] = input_game();
		cam.translate(dPos);
		cam.rotate(dTheta);
		fanLevel.translateLocal(dPos);

		/* update affine matrices */
		fanLevel.prepAffines();
	}

	return 0;

}
