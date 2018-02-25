#include <limits.h>
#include <stdio.h>

#include <tonc.h>

#include "gfx/bc1floor.h"
#include "gfx/bgpal.h"

/* globals */

OBJ_ATTR *obj_cross = &oam_mem[0];
OBJ_ATTR *obj_disp = &oam_mem[1];

BG_AFFINE bgaff;
AFF_SRC_EX asx = {
	32<<8, 64<<8,           // Map coords.
	120, 80,                // Screen coords.
	0x0100, 0x0100, 0       // Scales and angle.
};

const int DX = 256;
FIXED ss = 0x0100;

/* prototypes */

void init_main();
void input_game();
void win_textbox(int bgnr, int left, int top, int right, int bottom, int bldy);
void init_cross();
void init_map();

/* implementations */

void win_textbox(int bgnr, int left, int top, int right, int bottom, int bldy) {
	REG_WIN0H = left<<8 | right;
	REG_WIN0V =  top<<8 | bottom;
	REG_WIN0CNT = WIN_ALL | WIN_BLD;
	REG_WINOUTCNT = WIN_ALL;

	REG_BLDCNT = (BLD_ALL&~BIT(bgnr)) | BLD_BLACK;
	REG_BLDY = bldy;

	REG_DISPCNT |= DCNT_WIN0;

	tte_set_margins(left, top, right, bottom);
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
	obj_disp->attr2 = 0x1001;
}

void init_map() {
	// Setup main bg
	LZ77UnCompVram(bgPal, pal_bg_mem);
	LZ77UnCompVram(bc1floorTiles, tile_mem[0]);
	LZ77UnCompVram(bc1floorMap, se_mem[8]);

	// Registers
	REG_BG2CNT = BG_CBB(0) | BG_SBB(8) | BG_AFF_128x128;
	bgaff = bg_aff_default;
}

void input_game() {
	key_poll();

	if(key_is_down(KEY_A)) {
		/* dir + A : move map in screen coords */
		asx.scr_x += key_tri_horz();
		asx.scr_y += key_tri_vert();
	} else {
		/* dir : move map in map coords */
		asx.tex_x -= DX*key_tri_horz();
		asx.tex_y -= DX*key_tri_vert();
	}

	/* L / R : rotate */
	asx.alpha -= 128*key_tri_shoulder();

	/* B: scale up ; B+Se : scale down */
	if(key_is_down(KEY_B))
		ss += (key_is_down(KEY_SELECT) ? -1 : 1);

	if(key_hit(KEY_START)) {
		if(!key_is_down(KEY_SELECT)) {
			/* St : toggle wrapping flag. */
			REG_BG2CNT ^= BG_WRAP;
		} else {
			/* St+Se : reset */
			asx.tex_x = asx.tex_y= 0;
			asx.scr_x = asx.scr_y= 0;
			asx.alpha = 0;
			ss = 1<<8;
		}
	}

	/* apply fixed point transformation */
	asx.sx = asx.sy = (1<<16)/ss;
}

int main() {
	init_map();
	init_cross();

	/* registers */
	REG_DISPCNT = DCNT_MODE1 | DCNT_BG0 | DCNT_BG2 | DCNT_OBJ;

	/* text system */
	tte_init_chr4c_b4_default(0, BG_CBB(2)|BG_SBB(28));
	tte_init_con();
	win_textbox(0, 8, 120, 232, 156, 8);

	/* irqs */
	irq_init(NULL);
	irq_add(II_HBLANK, NULL);
	irq_add(II_VBLANK, NULL);

	while(1) {
		VBlankIntrWait();
		input_game();

		/* apply affine matrix to bg */
		bg_rotscale_ex(&bgaff, &asx);
		REG_BG_AFFINE[2] = bgaff;

		/* update cross position (indicates rotation point)
		   (== p in map-space; q in screen-space) */
		obj_set_pos(obj_cross, asx.scr_x-3, (asx.scr_y-3));
		obj_set_pos(obj_disp, (bgaff.dx>>8)-3, (bgaff.dy>>8)-3);

		/* update hud */
		tte_printf("#{es;P}p0\t: (%d, %d)\nq0\t: (%d, %d)\ndx\t: (%d, %d)", 
		    asx.tex_x>>8, asx.tex_y>>8, asx.scr_x, asx.scr_y, 
		    bgaff.dx>>8, bgaff.dy>>8);
	}

	return 0;

}
