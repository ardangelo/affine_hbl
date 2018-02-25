#include <limits.h>
#include <stdio.h>

#include <tonc.h>

#include "gfx/bc1floor.h"
#include "gfx/bgpal.h"

/* globals */

OBJ_ATTR *obj_cross= &oam_mem[0];
OBJ_ATTR *obj_disp= &oam_mem[1];

BG_AFFINE bgaff;

/* prototypes */

void init_main();
void input_game();
void win_textbox(int bgnr, int left, int top, int right, int bottom, int bldy);
void init_cross();
void init_map();

/* implementations */

void win_textbox(int bgnr, int left, int top, int right, int bottom, int bldy) {
	REG_WIN0H= left<<8 | right;
	REG_WIN0V=  top<<8 | bottom;
	REG_WIN0CNT= WIN_ALL | WIN_BLD;
	REG_WINOUTCNT= WIN_ALL;

	REG_BLDCNT= (BLD_ALL&~BIT(bgnr)) | BLD_BLACK;
	REG_BLDY= bldy;

	REG_DISPCNT |= DCNT_WIN0;

	tte_set_margins(left, top, right, bottom);
}

void init_cross() {
	TILE cross= {{
		0x00011100, 0x00100010, 0x01022201, 0x01021201, 
		0x01022201, 0x00100010, 0x00011100, 0x00000000,
	}};
	tile_mem[4][1]= cross;

	pal_obj_mem[0x01]= pal_obj_mem[0x12]= CLR_WHITE;
	pal_obj_mem[0x02]= pal_obj_mem[0x11]= CLR_BLACK;

	obj_cross->attr2= 0x0001;
	obj_disp->attr2= 0x1001;
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
}

int main() {
	init_map();
	init_cross();

	/* registers */
	REG_DISPCNT= DCNT_MODE1 | DCNT_BG0 | DCNT_BG2 | DCNT_OBJ;

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
	}

	return 0;

}
