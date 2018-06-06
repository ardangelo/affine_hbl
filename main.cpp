#include <limits.h>
#include <stdio.h>

#include <tonc.h>
#include <Chip/Unknown/Nintendo/GBA.hpp>

#include <Register/Register.hpp>
using namespace Kvasir::Register;

#include "mode7.h"

#include "gfx/fanroom.h"
#include "gfx/bgpal.h"

#include "maps/fanroom.h"

#define DEBUG(fmt, ...)
#define DEBUGFMT(fmt, ...)

/*
#include <stdlib.h>
char *dbg_str;
#define DEBUG(str) (nocash_puts(str))
#define DEBUGFMT(fmt, ...) do {	  \
		asprintf(&dbg_str, fmt, __VA_ARGS__); \
		nocash_puts(dbg_str); \
		free(dbg_str); \
	} while (0)
*/

/* block mappings */
#define M7_CBB 0
#define FLOOR_SBB 24
#define FLOOR_PRIO 2
#define WALL_PRIO 3

/* m7 globals */

M7Camera cam(fxdiv(int2fx(M7_TOP), int2fx(M7_D)));
M7Map floorMap(M7_CBB, FLOOR_SBB, FLOOR_PRIO, Kvasir::BackgroundSizes::aff128x128,
	fanroomFloorBlocks, 16, 16, 32, floorExtentWidths, floorExtentOffs,
	floorExtents, cam.fov);
M7Map wallMap(M7_CBB, FLOOR_SBB, WALL_PRIO, Kvasir::BackgroundSizes::aff128x128,
	fanroomWallBlocks, 16, 16, 32, wallExtentWidths, wallExtentOffs,
	wallExtents, cam.fov);
M7Level fanLevel(&cam, &floorMap, &wallMap);

/* implementations */

void init_map() {
	/* extract main bg */
	LZ77UnCompVram(bgPal, pal_bg_mem);
	LZ77UnCompVram(fanroomTiles, tile_mem[M7_CBB]);
	LZ77UnCompVram(fanroomMap, se_mem[FLOOR_SBB]);

	/* precompute for mode 7 */
	pre.inv_fov = fxdiv(int2fx(1), cam.fov);
	pre.inv_fov_x_ppb = fxdiv(int2fx(1), cam.fov * PIX_PER_BLOCK);
	for (int h = 0; h < SCREEN_HEIGHT; h++) {
		pre.x_cs[h] = fxsub(2 * fxdiv(int2fx(h), int2fx(SCREEN_HEIGHT)), int2fx(1));
	}

	/* setup shadow fade */
	REG_BLDCNT = 0;
	{ using namespace Kvasir::BLDCNT;
		apply(
			set(bg2FirstTargetPixel, bg3FirstTargetPixel),
			set(backdropSecondTargetPixel),
			write(colorSpecialEffect, ColorSpecialEffects::brightnessDecrease));
	}
	REG_WININ = 0;
	{ using namespace Kvasir::WININ;
		apply(
			set(window0EnableBg2, window0EnableBg3, window0EnableColorSpecialEffect));
	}
	REG_WIN0V = 0;
	{ using namespace Kvasir::WIN0V;
		apply(
			write(y, SCREEN_HEIGHT));
	}
	pal_bg_mem[0] = CLR_GRAY / 2;

	/* display control register */
	REG_DISPCNT = 0;
	{ using namespace Kvasir::DISPCNT;
		apply(
			write(bgMode, BackgroundModes::mode2),
			set(screenDisplayBg2, screenDisplayBg3, screenDisplayObj),
			write(objCharacterVramMapping,
				ObjCharacterVramMappingMode::oneDim),
			set(window0DisplayFlag));
	}
}

const FIXED OMEGA =  0x400;
const FIXED VEL_X =  0x1 << 8;
const FIXED VEL_Z = -0x1 << 8;
void input_game(VECTOR *dir) {
	key_poll();

	/* strafe */
	dir->x = VEL_X * key_tri_shoulder();

	/* forwards / backwards */
	dir->z = VEL_Z * key_tri_vert();

	/* rotate */
	cam.theta -= OMEGA * key_tri_horz();
}

int main() {
	init_map();

	/* irqs */
	irq_init(NULL);
	irq_add(II_HBLANK, (fnptr)m7_hbl);
	irq_add(II_VBLANK, NULL);

	while(1) {
		VBlankIntrWait();

		/* update camera based on input */
		VECTOR dir = {0, 0, 0};
		input_game(&dir);
		cam.rotate(cam.theta);
		fanLevel.translateLocal(&dir);

		/* update affine matrices */
		fanLevel.prepAffines();
	}

	return 0;

}
