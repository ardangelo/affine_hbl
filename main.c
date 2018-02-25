#include <limits.h>
#include <stdio.h>

#include <tonc.h>

#include "mode7.h"

#include "gfx/bc1floor.h"
#include "gfx/bc1sky.h"
#include "gfx/bgpal.h"

/* globals */

OBJ_ATTR *obj_cross = &oam_mem[0];

static const VECTOR cam_pos_default= { 256<<8, 32<<8, 256<<8 };
VECTOR cam_pos;
u16 cam_phi = 0;

FIXED g_cosf = 1 << 8;
FIXED g_sinf = 0;

/* prototypes */

/* overlay functions */
void init_cross();
void win_textbox(int bg_num, int left, int top, int right, int bottom, int blend_y);

void input_game();

/* implementations */

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

void win_textbox(int bg_num, int left, int top, int right, int bottom, int blend_y) {
	REG_WIN0H = (left << 8) | right;
	REG_WIN0V = (top  << 8) | bottom;

	REG_WIN0CNT = WIN_ALL | WIN_BLD;
	REG_WINOUTCNT = WIN_ALL;

	REG_BLDCNT = (BLD_ALL & ~BIT(bg_num)) | BLD_BLACK;
	REG_BLDY = blend_y;

	REG_DISPCNT |= DCNT_WIN0;

	tte_set_margins(left, top, right, bottom);
}

void init_map() {
	// Setup main bg
	LZ77UnCompVram(bgPal, pal_bg_mem);
	LZ77UnCompVram(bc1floorTiles, tile_mem[0]);
	LZ77UnCompVram(bc1floorMap, se_mem[8]);

	// Registers
	REG_BG2CNT = BG_CBB(0) | BG_SBB(8) | BG_AFF_128x128;
}

void input_game() {
	const FIXED speed = 2;
	const FIXED DY = 64;
	VECTOR dir;

	key_poll();

	/* left/right : strafe */
	dir.x = speed * key_tri_horz();
	/* B/A : rise/sink */
	dir.y = DY * key_tri_fire();
	/* up/down : forward/back */
	dir.z = speed * key_tri_vert();

	/* update camera position */
	cam_pos.x += (dir.x * g_cosf) - (dir.z * g_sinf);
	cam_pos.y += dir.y;
	cam_pos.z += (dir.x * g_sinf) + (dir.z * g_cosf);

	/* keep above ground */
	if (cam_pos.y < 0) { cam_pos.y = 0; }

	/* L/R : rotate */
	cam_phi += 256 * key_tri_shoulder();

	/* start : reset */
	if (key_hit(KEY_START)) {
		cam_pos = cam_pos_default;
		cam_phi = 0;
	}

	/* update cos / sin globals for isr */
	g_cosf = lu_cos(cam_phi) >> 4;
	g_sinf = lu_sin(cam_phi) >> 4;
}

int main() {
	init_map();
	init_cross();

	/* registers */
	REG_DISPCNT = DCNT_MODE1 | DCNT_BG0 | DCNT_BG2 | DCNT_OBJ;

	/* hud system */
	tte_init_chr4c_b4_default(0, BG_CBB(2)|BG_SBB(28));
	tte_init_con();
	win_textbox(0, 8, 120, 232, 156, 8);

	/* camera */
	cam_pos = cam_pos_default;

	/* irqs */
	irq_init(NULL);
	irq_add(II_HBLANK, (fnptr)m7_hbl);
	irq_add(II_VBLANK, NULL);

	while(1) {
		VBlankIntrWait();
		input_game();

		/* update cross position  */
		obj_set_pos(obj_cross, 0, 0);

		/* update hud */
		tte_printf("#{es;P}cam\t: (%d, %d, %d) phi %d\n",
			cam_pos.x, cam_pos.y, cam_pos.z, cam_phi);
	}

	return 0;

}
