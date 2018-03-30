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
	CAM_NORMAL, /* camera state */
	0x0, /* theta */
	0x0, /* phi */
	1, /* focal factor */
	{256, 0, 0}, /* u */
	{0, 256, 0}, /* v */
	{0, 0, 256}  /* w */
};

/* prototypes */

void init_main();
void init_cross();

void input_game();
void apply_camera_effects();
void camera_update();

/* implementations */
static FIXED heightmap_input, heightmap_output;
FIXED level_heightmap(FIXED z) {
	static const FIXED depth  = 0x400; // 0f

	static const FIXED level0 = 0x000 << 12; // 12f
	static const FIXED end0   = 0x100; // 0f
	static const FIXED start1 = 0x200; // 0f
	static const FIXED level1 = 0x001 << 12; // 12f

	heightmap_input = z;
	heightmap_output = (heightmap_input > (depth / 2)) ? level1 : level0;
	return heightmap_output;

	FIXED y = 0;
	if (z > end0) {
		y = level0;
	} else if (z < start1) {
		y = level1;
	} else {
		FIXED slope = (level1 - level0) / (end0 - start1); // 12f
		y = slope * (z + end0); // 12f
	}

	heightmap_output = y;
	return y;
}

void init_map() {
	/* layout level */

	m7_level.heightmap = &level_heightmap;

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

	obj_hide(obj_cross);
}

void input_game(VECTOR *dir) {
	const FIXED VEL_H = 0x200;
	const FIXED VEL_Y = 0x80;
	const FIXED OMEGA = 0x40;
	int aim_speed_x = 3;
	int aim_speed_y = 3;

	key_poll();

	switch (g_control_state) {
		case CTRL_NORMAL:
		/* strafe */
		dir->x = VEL_H * key_tri_horz();
		/* back/forward */
		dir->z = VEL_H * key_tri_vert();

		/* scope in */
		if (key_hit(KEY_R)) {
			g_control_state = CTRL_ZOOMED;
			m7_level.camera->state = CAM_ZOOMIN;

			/* set up crosshair */
			pt_crosshair.x = SCREEN_WIDTH / 2 - 4;
			pt_crosshair.y = SCREEN_HEIGHT / 2 - 4;
			obj_unhide(obj_cross, 0);
		}
		break;

		case CTRL_ZOOMED:
		/* adjust the crosshair */
		pt_crosshair.x += aim_speed_x * key_tri_horz(); /* move crosshair left/right */
		pt_crosshair.x = CLAMP(pt_crosshair.x,
			CROSS_SCROLL_WIDTH_X,
			SCREEN_WIDTH - (CROSS_WIDTH / 2) - CROSS_SCROLL_WIDTH_X);
		pt_crosshair.y += aim_speed_y * key_tri_vert(); /* move crosshair up.down */
		pt_crosshair.y = CLAMP(pt_crosshair.y,
			CROSS_SCROLL_WIDTH_Y,
			SCREEN_HEIGHT - (CROSS_HEIGHT / 2) - CROSS_SCROLL_WIDTH_Y);
		/* rotate camera to track crosshair */
		m7_level.camera->phi += OMEGA * key_tri_horz();
		m7_level.camera->theta += OMEGA * key_tri_vert();

		/* scope out */
		if (key_released(KEY_R)) {
			g_control_state = CTRL_NORMAL;
			m7_level.camera->state = CAM_ZOOMOUT;

			obj_hide(obj_cross);
		}
		break;
	}
}

#define ZOOM_FRAME_CT 8
#define ZOOM_FOCAL_MULT 2
static const int zoom_anim[ZOOM_FRAME_CT] = {
	0x40, 0x40, 0x38, 0x30, 0x20, 0x10, 0x10, 0x08
};
static const int sum_anim = (0x40 + 0x40 + 0x38 + 0x30 + 0x20 + 0x10 + 0x10 + 0x08);
void apply_camera_effects(VECTOR *dir) {
	static u8 zoom_frame = 0;
	static int orig_phi = 0, orig_theta = 0;

	switch (m7_level.camera->state) {
		case CAM_ZOOMIN:
		if (zoom_frame == 0) {
			/* save phi, theta */
			orig_phi = m7_level.camera->phi;
			orig_theta = m7_level.camera->theta;
		} else if (zoom_frame == (ZOOM_FRAME_CT - 1)) {
			m7_level.camera->state = CAM_ZOOMED;
			break;
		}

		zoom_frame++;

		dir->z -= zoom_anim[zoom_frame];
		m7_level.camera->focal_offs += zoom_anim[zoom_frame] >> ZOOM_FOCAL_MULT;
		break;

		case CAM_ZOOMOUT:
		zoom_frame--;

		/* apply zoom / rotate effects based on frame */
		dir->z += zoom_anim[zoom_frame];
		m7_level.camera->focal_offs -= zoom_anim[zoom_frame] >> ZOOM_FOCAL_MULT;
		m7_level.camera->phi -= (m7_level.camera->phi - orig_phi) >> 3;
		m7_level.camera->theta -= (m7_level.camera->theta - orig_theta) >> 3;

		if (zoom_frame == 0) {
			/* reset back to unzoomed */
			m7_level.camera->state = CAM_NORMAL;
			m7_level.camera->focal_offs = 0;

			/* restore phi, theta */
			m7_level.camera->phi = orig_phi;
			m7_level.camera->theta = orig_theta;
		}
		break;

		default:
		break;
	}
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

	/* controls */
	g_control_state = CTRL_NORMAL;

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
		apply_camera_effects(&dir);
		camera_update(&dir);

		/* update cross position  */
		obj_set_pos(obj_cross, pt_crosshair.x, pt_crosshair.y);

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

		tte_printf("#{es;P}z %u, f(z) %u", heightmap_input, heightmap_output);
	}

	return 0;

}
