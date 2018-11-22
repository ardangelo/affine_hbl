#include <limits.h>
#include <stdio.h>

#include <tonc.h>

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
M7Map floorMap(
	BG_CBB(M7_CBB) | BG_SBB(FLOOR_SBB) | BG_AFF_128x128 | BG_PRIO(FLOOR_PRIO),
	cam.fov);
M7Level fanLevel(&cam, &floorMap);

/* implementations */

void init_map() {
	/* extract main bg */
	LZ77UnCompVram(bgPal, pal_bg_mem);
	LZ77UnCompVram(fanroomTiles, tile_mem[M7_CBB]);
	LZ77UnCompVram(fanroomMap, se_mem[FLOOR_SBB]);

	/* setup shadow fade */
	REG_BLDCNT = BLD_BUILD(BLD_BG2, BLD_BACKDROP, 3);
	REG_WININ = WININ_BUILD(WIN_BG2 | WIN_BLD, 0);
	REG_WIN0V = SCREEN_HEIGHT;
	pal_bg_mem[0] = CLR_GRAY / 2;

	/* registers */
	REG_DISPCNT = DCNT_MODE2 | DCNT_BG2 | DCNT_OBJ | DCNT_OBJ_1D;
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
