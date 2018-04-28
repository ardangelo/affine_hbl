#include <tonc.h>

#include "mode7.h"

#define FSH 8
#define FSC (1 << FSH)
#define FSC_F ((float)FSC)

#define fx2int(fx) (fx/FSC)
#define fx2float(fx) (fx/FSC_F)
#define fxadd(fa,fb) (fa + fb)
#define fxsub(fa, fb) (fa - fb)
#define fxmul(fa, fb) ((fa*fb)>>FSH)
#define fxdiv(fa, fb) (((fa)*FSC)/(fb))

#define TRIG_ANGLE_MAX 0xFFFF

IWRAM_CODE void m7_hbl() {
	int vc = REG_VCOUNT;

	/* apply affine */
	BG_AFFINE *bga = &m7_level.bgaff[vc + 1];
	REG_BG_AFFINE[2] = *bga;

	/* apply window */
	REG_WIN0H = m7_level.winh[vc + 1];
	REG_WIN0V = WIN_BUILD(SCREEN_HEIGHT, 0);

	/* apply shading */
	static int last_side = -1;
	/*
	if (last_side != bga->pb) {
		if (bga->pb == 1) {
			BFN_SET(REG_DISPCNT, DCNT_MODE0, DCNT_MODE);
			REG_BG2CNT = m7_level.bgcnt_sky;
		} else {
			BFN_SET(REG_DISPCNT, DCNT_MODE1, DCNT_MODE);
			REG_BG2CNT = m7_level.bgcnt_floor;
		}
	}
	*/
	last_side = bga->pb;
}

static const int start_line = 0;
IWRAM_CODE void m7_prep_affines(m7_level_t *level) {
	m7_cam_t *cam = level->camera;

	/* location vector */
	FIXED a_x = cam->pos.x; // 8f
	FIXED a_y = cam->pos.y; // 8f
	FIXED a_z = cam->pos.z; // 8f

	/* sines and cosines of pitch */
	FIXED cos_theta = cam->v.y; // 8f
	FIXED sin_theta = cam->w.y; // 8f

	BG_AFFINE *bg_aff_ptr = &level->bgaff[start_line];
	u16 *winh_ptr = &level->winh[start_line];

	for (int h = start_line; h < SCREEN_HEIGHT; h++) {
		/* ray intersect in camera plane */
		FIXED x_c = fxsub(2 * fxdiv(int2fx(h), int2fx(SCREEN_HEIGHT)), int2fx(1));

		/* ray components in world space */
		FIXED ray_y = fxadd(sin_theta, fxmul(fxmul(cam->fov, cos_theta), x_c));
		if (ray_y == 0) { ray_y = 1; }
		FIXED ray_z = fxsub(cos_theta, fxmul(fxmul(cam->fov, sin_theta), x_c));
		if (ray_z == 0) { ray_z = 1; }

		/* map coordinates */
		int map_y = fx2int(a_y);
		int map_z = fx2int(a_z);

		/* ray lengths to next x / z side */
		FIXED delta_dist_y = ABS(fxdiv(int2fx(1), ray_y));
		FIXED delta_dist_z = ABS(fxdiv(int2fx(1), ray_z));

		/* initialize map / distance steps */
		int delta_map_y, delta_map_z;
		FIXED dist_y, dist_z;
		if (ray_y < 0) {
			delta_map_y = -1;
			dist_y = fxmul(fxsub(a_y, int2fx(map_y)), delta_dist_y);
		} else {
			delta_map_y = 1;
			dist_y = fxmul(fxsub(int2fx(map_y + 1), a_y), delta_dist_y);
		}
		if (ray_z < 0) {
			delta_map_z = -1;
			dist_z = fxmul(fxsub(a_z, int2fx(map_z)), delta_dist_z);
		} else {
			delta_map_z = 1;
			dist_z = fxmul(fxsub(int2fx(map_z + 1), a_z), delta_dist_z);
		}

		/* perform raycast */
		int hit = 0;
		int side;
		while (!hit) {
			if (dist_y < dist_z) {
				dist_y += delta_dist_y;
				map_y += delta_map_y;
				side = 0;
			} else {
				dist_z += delta_dist_z;
				map_z += delta_map_z;
				side = 1;
			}

			if (m7_level.blocks[map_y * m7_level.blocks_width + map_z] > 0) {
				hit = 1;
			}
		}

		/* calculate wall distance */
		FIXED perp_wall_dist;
		if (side == 0) {
			perp_wall_dist = fxdiv(fxadd(fxsub(int2fx(map_y), a_y), fxdiv(int2fx(1 - delta_map_y), int2fx(2))), ray_y);
		} else {
			perp_wall_dist = fxdiv(fxadd(fxsub(int2fx(map_z), a_z), fxdiv(int2fx(1 - delta_map_z), int2fx(2))), ray_z);
		}
		if (perp_wall_dist == 0) { perp_wall_dist = 1; }

		/* build affine matrices */
		FIXED lambda = fxmul(perp_wall_dist, cam->fov);

		/* scaling */
		bg_aff_ptr->pa = lambda;

		bg_aff_ptr->dx = fxadd(fxmul(lambda, int2fx(M7_LEFT)), fxmul(a_x, level->pixels_per_block));

		/* calculate angle corrections (angles are .12f) */
		FIXED correction;
		if (side == 0) {
			correction = fxadd(fxmul(perp_wall_dist, ray_z), a_z);
		} else {
			correction = fxadd(fxmul(perp_wall_dist, ray_y), a_y);
		}
		correction = fxmul(correction, level->pixels_per_block * 4);

		/* wrap texture for ceiling */
		if (((side == 0) && (ray_y > 0)) ||
			((side == 1) && (ray_z < 0))) {
			correction = fxsub(int2fx(level->texture_width - 1), correction);
		}

		bg_aff_ptr->dy = correction;

		/* calculate windowing */
		int line_height = fx2int(fxdiv(int2fx(SCREEN_WIDTH), lambda));
		int a_x_offs = fx2int(fxmul(fxdiv(a_x, lambda), level->pixels_per_block));

		int draw_start = -line_height / 2 + M7_RIGHT - a_x_offs;
		draw_start = CLAMP(draw_start, 0, M7_RIGHT);
		int draw_end = line_height / 2 + M7_RIGHT - a_x_offs;
		draw_end = CLAMP(draw_end, M7_RIGHT, SCREEN_WIDTH + 1);

		/* apply windowing */
		*winh_ptr = WIN_BUILD((u8)draw_end, (u8)draw_start);
		winh_ptr++;

		/* pb and pd aren't used (q_y is implicitly zero) */
		bg_aff_ptr->pb = side;
		bg_aff_ptr->pd = correction;

		bg_aff_ptr++;
	}
	level->bgaff[SCREEN_HEIGHT] = level->bgaff[0];
	level->winh[SCREEN_HEIGHT] = level->winh[0];
}
