#include <limits.h>
#include <stdio.h>

#include <tonc.h>

#include "mode7.h"

#include "gfx/fanroom.h"
#include "gfx/bgpal.h"

// #define TTE_ENABLED

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
#define WALL_PRIO 1

#define TTE_CBB 2
#define TTE_SBB 18

/* m7 globals */
m7_cam_t m7_cam;

u16 floor_winh[SCREEN_HEIGHT + 1], wall_winh[SCREEN_HEIGHT + 1];
BG_AFFINE floor_bgaff_arr[SCREEN_HEIGHT+1], wall_bgaff_arr[SCREEN_HEIGHT+1];
m7_level_t floor_level, wall_level;

m7_obj_t m7_obj_arr[M7_OBJ_COUNT];

static const m7_cam_t m7_cam_default = {
	{ 8 << FIX_SHIFT, 2 << FIX_SHIFT, 2 << FIX_SHIFT }, /* pos */
	0x0000, /* theta */
	0x0, /* phi */
	{1 << FIX_SHIFT, 0, 0}, /* u */
	{0, 1 << FIX_SHIFT, 0}, /* v */
	{0, 0, 1 << FIX_SHIFT}  /* w */
};

/* prototypes */

void init_main();

void input_game();
void camera_update();

/* implementations */

const int fanroom_floor_blocks[16 * 32] = {
  2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,
  2,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,2,
  2,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,2,
  2,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,2,
  2,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,2,
  2,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,2,
  2,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,2,
  2,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,2,
  2,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,3, 3,3,3,3,3,3,3,2,
  2,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,3,0, 0,0,0,0,0,0,0,2,
  2,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,3,0,0, 0,0,0,0,0,0,0,2,
  2,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,3,0,0,0, 0,0,0,0,0,0,0,2,
  2,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,3,0,0,0,0, 0,0,0,0,0,0,0,2,
  2,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,3,0,0,0,0,0, 0,0,0,0,0,0,0,2,
  2,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,3,0,0,0,0,0,0, 0,0,0,0,0,0,0,2,
  2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2
};

const int floor_extents[16 * 2] = {
	0,16, 0,16, 0,16, 0,16, 0,16, 0,16, 0,16, 0,16,
	0,16, 0,16, 0,16, 0,16, 0,16, 0,16, 0,16, 0,16,
};
FIXED floor_extent_widths[16];
FIXED floor_extent_offs[16];

const int fanroom_wall_blocks[16 * 32] = {
  1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
  1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 3,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 3,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,1,
  1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 3,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 3,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 3,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 3,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 3,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 3,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,1,
  1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1
};

const int wall_extents[16 * 2] = {
	0,16, 8,16, 8,16, 8,16, 0,16, 0,16, 0,16, 0,16,
	8,16, 8,16, 8,16, 8,16, 8,16, 8,16, 8,16, 8,16
};
FIXED wall_extent_widths[16];
FIXED wall_extent_offs[16];

void init_map() {
	/* layout level */
	floor_level.blocks = (int*)fanroom_floor_blocks;
	wall_level.blocks = (int*)fanroom_wall_blocks;

	floor_level.blocks_width = 32; floor_level.blocks_height = 16;
	wall_level.blocks_width = 32; wall_level.blocks_height = 16;

	floor_level.texture_width = 256; floor_level.texture_height = 512;
	wall_level.texture_width = 256; wall_level.texture_height = 512;

	floor_level.a_x_range = int2fx(floor_level.texture_width / floor_level.blocks_height);
	wall_level.a_x_range = floor_level.a_x_range;

	floor_level.extent_widths = floor_extent_widths;
	floor_level.extent_offs = floor_extent_offs;
	wall_level.extent_widths = wall_extent_widths;
	wall_level.extent_offs = wall_extent_offs;

	/* init mode 7 */
	m7_init(&floor_level, &m7_cam, floor_bgaff_arr, floor_winh,
		BG_CBB(M7_CBB) | BG_SBB(FLOOR_SBB) | BG_AFF_128x128 | BG_PRIO(FLOOR_PRIO), 2);
	m7_init(&wall_level, &m7_cam, wall_bgaff_arr, wall_winh,
		BG_CBB(M7_CBB) | BG_SBB(FLOOR_SBB) | BG_AFF_128x128 | BG_PRIO(WALL_PRIO), 3);
	m7_cam = m7_cam_default;
	m7_cam.fov = fxdiv(int2fx(M7_TOP), int2fx(M7_D));

	/* extract main bg */
	LZ77UnCompVram(bgPal, pal_bg_mem);
	LZ77UnCompVram(fanroomTiles, tile_mem[M7_CBB]);
	LZ77UnCompVram(fanroomMap, se_mem[FLOOR_SBB]);

	/* precompute for mode 7 */
	pre.inv_fov = fxdiv(int2fx(1), m7_cam.fov);
	pre.inv_fov_x_ppb = fxdiv(int2fx(1), m7_cam.fov * PIX_PER_BLOCK);
	for (int h = 0; h < SCREEN_HEIGHT; h++) {
		pre.x_cs[h] = fxsub(2 * fxdiv(int2fx(h), int2fx(SCREEN_HEIGHT)), int2fx(1));
	}

	/* precompute window extent widths */
	for (int i = 0; i < 16; i++) {
		floor_extent_widths[i] = fxmul(
			int2fx(
				(floor_extents[i * 2 + 1] - floor_extents[i * 2 + 0])
				* PIX_PER_BLOCK), // normal extent width
			m7_cam.fov); // adjust for fov
		floor_extent_offs[i] = (floor_level.a_x_range + int2fx(floor_extents[i * 2])) / 2;

		wall_extent_widths[i] = fxmul(
			int2fx(
				(wall_extents[i * 2 + 1] - wall_extents[i * 2 + 0])
				* PIX_PER_BLOCK), // normal extent width
			m7_cam.fov); // adjust for fov
		wall_extent_offs[i] = (wall_level.a_x_range + int2fx(wall_extents[i * 2])) / 2;
	}

	/* setup shadow fade */
	REG_BLDCNT = BLD_BUILD(BLD_BG2 | BLD_BG3, BLD_BACKDROP, 3);
	REG_WININ = WININ_BUILD(WIN_BG2 | WIN_BG3 | WIN_BLD, 0);
	REG_WIN0V = SCREEN_HEIGHT;
	pal_bg_mem[0] = CLR_GRAY / 2;

	/* registers */
	REG_DISPCNT = DCNT_MODE2 | DCNT_BG2 | DCNT_BG3 | DCNT_OBJ | DCNT_OBJ_1D | DCNT_WIN0;
#ifdef TTE_ENABLED
	REG_DISPCNT |= DCNT_BG0;
#endif
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
	m7_cam.theta -= OMEGA * key_tri_horz();
}

void camera_update(VECTOR *dir) {
	/* update camera rotation */
	m7_rotate(&m7_cam, m7_cam.theta);

	/* update camera position */
	m7_translate_local(&floor_level, dir);

	/* don't sink through the ground */
	if(m7_cam.pos.y < (2 << 8)) {
		m7_cam.pos.y = 2 << 8;
	}
}

int main() {
	init_map();

	/* hud */
#ifdef TTE_ENABLED
	tte_init_chr4c_b4_default(0, BG_CBB(TTE_CBB) | BG_SBB(TTE_SBB));
	tte_init_con();
	tte_set_margins(8, 8, 232, 40);
#endif

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
		m7_prep_affines(&wall_level, &floor_level);

		/* update objects */
		m7_update_objects(&floor_level);

		/* update hud */
#ifdef TTE_ENABLED
		tte_printf("#{es;P}x %x fov %x",
			m7_cam.pos.x, m7_cam.fov);
#endif
	}

	return 0;

}
