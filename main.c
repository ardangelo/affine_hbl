#include <limits.h>
#include <stdio.h>

#include <tonc.h>

#include "mode7.h"

#include "gfx/bc1floor.h"
#include "gfx/bc1sky.h"
#include "gfx/bgpal.h"

/* block mappings */
#define M7_CBB 0
#define SKY_SBB 22
#define FLOOR_SBB 24
#define M7_PRIO 3

/* obj globals */
OBJ_ATTR *obj_cross = &oam_mem[0];

/* m7 globals */
m7_cam_t m7_cam;
BG_AFFINE m7_aff_arrs[SCREEN_HEIGHT+1];
m7_level_t m7_level;

static const m7_cam_t m7_cam_default = {
	{ 0x0D100, 0x1900, 0x38800 },
	0x0A00,
	0x2600,
	{ 256, 0, 0 }, {0, 256, 0}, {0, 0, 256}
};

/* prototypes */

void init_main();
void init_cross();
void win_textbox(int bg_num, int left, int top, int right, int bottom, int blend_y);

void input_game();

/* implementations */

void init_map() {
	/* init mode 7 */
	m7_init(&m7_level, &m7_cam, m7_aff_arrs,
		BG_CBB(M7_CBB) | BG_SBB(SKY_SBB) | BG_REG_64x32 | BG_PRIO(M7_PRIO),
		BG_CBB(M7_CBB) | BG_SBB(FLOOR_SBB) | BG_AFF_128x128 | BG_WRAP | BG_PRIO(M7_PRIO));
	*(m7_level.camera) = m7_cam_default;

	/* extract main bg */
	LZ77UnCompVram(bgPal, pal_bg_mem);
	LZ77UnCompVram(bc1floorTiles, tile_mem[M7_CBB]);
	LZ77UnCompVram(bc1floorMap, se_mem[FLOOR_SBB]);

	/* setup orange fade */
	REG_BLDCNT = BLD_BUILD(BLD_BG2, BLD_BACKDROP, 1);
	pal_bg_mem[0] = CLR_ORANGE;

	/* extract sky bg */
	LZ77UnCompVram(bc1skyTiles, &tile_mem[M7_CBB][128]);
	LZ77UnCompVram(bc1skyMap, se_mem[SKY_SBB]);

	// Registers
	REG_DISPCNT = DCNT_MODE1 | DCNT_BG2 | DCNT_OBJ | DCNT_OBJ_1D;
}


void init_cross() {
	TILE cross = {{
		0x00011100, 0x00100010, 0x01022201, 0x01021201, 
		0x01022201, 0x00100010, 0x00011100, 0x00000000,
	}};
	tile_mem[4][1] = cross;

	pal_obj_mem[0x01] = pal_obj_mem[0x12] = CLR_WHITE;
	pal_obj_mem[0x02] = pal_obj_mem[0x11] = CLR_BLACK;

	obj_cross->attr2 = 0x0001;
}

void input_game() {
	const FIXED VEL_H = 0x200;
	const VEL_Y = 0x80;
	const OMEGA = 0x140;

	VECTOR dir = {0, 0, 0};

	key_poll();
}

int main() {
	init_map();

	/* hud */
	init_cross();

	/* irqs */
	irq_init(NULL);
	irq_add(II_HBLANK, (fnptr)m7_hbl);
	irq_add(II_VBLANK, NULL);

	while(1) {
		VBlankIntrWait();
		input_game();

		/* update cross position  */
		obj_set_pos(obj_cross, 0, 0);

		/* update horizon */
		m7_prep_horizon(&m7_level);
		if (m7_level.horizon > 0) {
			BFN_SET(REG_DISPCNT, DCNT_MODE0, DCNT_MODE);
			REG_BG2CNT = m7_level.bgcnt_sky;
			REG_BLDALPHA = 16;
		}
		m7_update_sky(&m7_level);

		/* update affine matrices */
		m7_prep_affines(&m7_level);
	}

	return 0;

}
