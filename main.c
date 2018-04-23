#include <limits.h>
#include <stdio.h>

#include <tonc.h>

#include "mode7.h"

#include "gfx/bc1floor.h"
#include "gfx/bc1sky.h"
#include "gfx/bgpal.h"

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
#define SKY_SBB 22
#define FLOOR_SBB 24
#define TTE_CBB 1
#define TTE_SBB 20
#define M7_PRIO 3

/* m7 globals */
m7_cam_t m7_cam;
BG_AFFINE m7_aff_arrs[SCREEN_HEIGHT+1];
u16 m7_winh[SCREEN_HEIGHT + 1];
m7_level_t m7_level;

static const m7_cam_t m7_cam_default = {
	{ 22 << FIX_SHIFT, 0x0200, 12 << FIX_SHIFT }, /* pos */
	0x0, /* theta */
	0x0, /* phi */
	{1 << FIX_SHIFT, 0, 0}, /* u */
	{0, -1 << FIX_SHIFT, 0}, /* v */
	{0, 0, 1 << FIX_SHIFT}  /* w */
};

/* prototypes */

void init_main();

void input_game();
void camera_update();

/* implementations */

const int blocks[32][32] = {
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,2,2,0,0,0,0,0,0,2,2,0,0,0,0,0,0,2,2,0,0,0,0,0,0,2,2,0,0,1},
	{1,0,0,2,2,0,0,0,0,0,0,2,2,0,0,0,0,0,0,2,2,0,0,0,0,0,0,2,2,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,2,2,0,0,0,0,0,0,2,2,0,0,0,0,0,0,2,2,0,0,0,0,0,0,2,2,0,0,1},
	{1,0,0,2,2,0,0,0,0,0,0,2,2,0,0,0,0,0,0,2,2,0,0,0,0,0,0,2,2,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,2,2,0,0,0,0,0,0,2,2,0,0,0,0,0,0,2,2,0,0,0,0,0,0,2,2,0,0,1},
	{1,0,0,2,2,0,0,0,0,0,0,2,2,0,0,0,0,0,0,2,2,0,0,0,0,0,0,2,2,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,2,2,0,0,0,0,0,0,2,2,0,0,0,0,0,0,2,2,0,0,0,0,0,0,2,2,0,0,1},
	{1,0,0,2,2,0,0,0,0,0,0,2,2,0,0,0,0,0,0,2,2,0,0,0,0,0,0,2,2,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,2,2,0,0,0,0,0,0,2,2,0,0,0,0,0,0,2,2,0,0,0,0,0,0,2,2,0,0,1},
	{1,0,0,2,2,0,0,0,0,0,0,2,2,0,0,0,0,0,0,2,2,0,0,0,0,0,0,2,2,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
};

void init_map() {
	/* layout level */
	m7_level.blocks = (int**)&blocks;

	/* init mode 7 */
	m7_init(&m7_level, &m7_cam, m7_aff_arrs, m7_winh,
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
	REG_DISPCNT = DCNT_MODE1 | DCNT_BG0 | DCNT_BG2 | DCNT_OBJ | DCNT_OBJ_1D | DCNT_WIN0;
	REG_WININ = WININ_BUILD(WIN_BG2, 0);
	REG_WINOUT = WINOUT_BUILD(WIN_BG0, 0);
}

void input_game(VECTOR *dir) {
	const FIXED VEL_H = 0x1 << 5;
	const FIXED VEL_Y = 0x1 << 5;

	key_poll();

	/* strafe */
	dir->x = VEL_H * key_tri_horz();

	/* forwards / backwards */
	dir->z = VEL_Y * key_tri_vert();
}

void camera_update(VECTOR *dir) {
	/* update camera rotation */
	m7_rotate(m7_level.camera, m7_level.camera->phi, m7_level.camera->theta);

	/* update camera position */
	m7_translate(&m7_level, dir);

	/* don't sink through the ground */
	if(m7_level.camera->pos.y < (2 << 8)) {
		m7_level.camera->pos.y = 2 << 8;
	}
}

int main() {
	init_map();
	m7_level.horizon = 0;

	/* hud */
	tte_init_chr4c_b4_default(0, BG_CBB(TTE_CBB) | BG_SBB(TTE_SBB));
	tte_init_con();
	tte_set_margins(8, 8, 232, 40);

	/* irqs */
	irq_init(NULL);
	irq_add(II_HBLANK, (fnptr)m7_hbl);
	irq_add(II_VBLANK, NULL);

	while(1) {
		VBlankIntrWait();

		/* update camera based on input */
		VECTOR dir = {0, 0, 0};
		input_game(&dir);
		camera_update(&dir);

		/* update affine matrices */
		m7_prep_affines(&m7_level);

		/* update hud */
		/*
		int offs = 5;
		tte_printf("#{es;P}%d %d,%d %d %d,%d %d %d,%d",
			1 + offs, m7_level.bgaff[1 + offs].dx, m7_level.bgaff[1 + offs].dy,
			80 + offs, m7_level.bgaff[80 + offs].dx, m7_level.bgaff[80 + offs].dy,
			100 + offs, m7_level.bgaff[100 + offs].dx, m7_level.bgaff[100 + offs].dy);
			*/
	}

	return 0;

}
