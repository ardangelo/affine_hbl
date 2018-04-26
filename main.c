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
#define TTE_CBB 1
#define TTE_SBB 20
#define M7_PRIO 3

/* game globals */
typedef enum { CTRL_NORMAL, CTRL_ZOOMED } control_state_t;
control_state_t g_control_state;

/* obj globals */
POINT pt_crosshair;
#define CROSS_HEIGHT 8
#define CROSS_SCROLL_WIDTH_X 24
#define CROSS_WIDTH 8
#define CROSS_SCROLL_WIDTH_Y 24
OBJ_ATTR *obj_cross = &oam_mem[0];

/* m7 globals */
m7_cam_t m7_cam;
BG_AFFINE m7_aff_arrs[SCREEN_HEIGHT+1];
m7_level_t m7_level;

static const m7_cam_t m7_cam_default = {
	{ 0x00, 0x2000, 0x00 }, /* pos */
	0x0, /* theta */
	0x0, /* phi */
	{256, 0, 0}, /* u */
	{0, 256, 0}, /* v */
	{0, 0, 256}  /* w */
};

/* prototypes */

void init_main();
void init_cross();

void input_game();
void camera_update();

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
	REG_DISPCNT = DCNT_MODE1 | DCNT_BG0 | DCNT_BG2 | DCNT_OBJ | DCNT_OBJ_1D;
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

#ifdef NO
	obj_hide(obj_cross);
#endif
}

void input_game(VECTOR *dir) {
	const FIXED VEL_H= 0x200, VEL_Y= 0x80, OMEGA= 0x140;

	key_poll();

	/* strafe */
	dir->x = VEL_H * key_tri_shoulder();

	if(key_is_down(KEY_SELECT)) {
		dir->y = VEL_Y * key_tri_fire(); // B/A : sink/rise
	} else {
		dir->z = -VEL_H * key_tri_fire(); // B/A : back/forward
	}

	/* change camera orientation */
	m7_cam.phi   += OMEGA * key_tri_horz();
	m7_cam.theta -= OMEGA * key_tri_vert();
}

void camera_update(VECTOR *dir) {
	/* update camera rotation */
	m7_rotate(m7_level.camera, m7_level.camera->phi, m7_level.camera->theta);

	/* update camera position */
	m7_translate(m7_level.camera, dir);

	/* don't sink through the ground */
	if(m7_level.camera->pos.y < (2 << 8)) {
		m7_level.camera->pos.y = 2 << 8;
	}
}

int main() {
	init_map();

	/* hud */
	init_cross();
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

		/* update cross position  */
		obj_set_pos(obj_cross, pt_crosshair.x - 4, pt_crosshair.y - 4);

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

		/* update hud */
		FIXED z = (m7_cam.pos.z >> 8) + pt_crosshair.y;
		tte_printf("#{es;P}z %u", z);
	}

	return 0;

}
