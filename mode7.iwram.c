#include <tonc.h>

#include "mode7.h"

#define PIX_PER_BLOCK 16

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
	BG_AFFINE *wa = &m7_level.wallaff[vc + 1];
	REG_BG_AFFINE[2] = *wa;
	BG_AFFINE *bga = &m7_level.bgaff[vc + 1];
	REG_BG_AFFINE[3] = *bga;

	/* shading */
	u32 ey = bga->pb >> 7;
	if (ey > 16) { ey = 16; }
	REG_BLDY = BLDY_BUILD(ey);
	REG_WIN0H = m7_level.winh[vc + 1];
	REG_WIN0V = SCREEN_HEIGHT;
}

IWRAM_CODE void m7_prep_affines(m7_level_t *level) {
	m7_cam_t *cam = level->camera;

	/* location vector */
	FIXED a_x = cam->pos.x; // 8f
	FIXED a_y = cam->pos.y; // 8f
	FIXED a_z = cam->pos.z; // 8f

	/* sines and cosines of pitch */
	FIXED cos_theta = cam->v.y; // 8f
	FIXED sin_theta = cam->w.y; // 8f

	BG_AFFINE *bg_aff_ptr = &level->bgaff[0];
	BG_AFFINE *wall_aff_ptr = &level->wallaff[0];
	u16 *winh_ptr = &level->winh[0];

	for (int h = 0; h < SCREEN_HEIGHT; h++) {
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
		FIXED lambda = fxdiv(perp_wall_dist, cam->fov) / PIX_PER_BLOCK;

		/* scaling */
		bg_aff_ptr->pa = lambda;

		/* camera x-position */
		bg_aff_ptr->dx = fxadd(fxmul(lambda, int2fx(M7_LEFT)), a_x * PIX_PER_BLOCK);

		/* move side to correct texture source */
		if (side == 1) {
			bg_aff_ptr->dx += int2fx(level->texture_width);
		}

		/* calculate angle corrections (angles are .12f) */
		FIXED correction;
		if (side == 0) {
			correction = fxadd(fxmul(perp_wall_dist, ray_z), a_z);
		} else {
			correction = fxadd(fxmul(perp_wall_dist, ray_y), a_y);
		}
		correction *= PIX_PER_BLOCK;

		/* wrap texture for ceiling */
		if (((side == 0) && (ray_y > 0)) ||
			((side == 1) && (ray_z < 0))) {
			correction = fxsub(int2fx(level->texture_height), correction);
		}

		bg_aff_ptr->dy = correction;

		/* calculate windowing */
		int line_height = fx2int(fxmul(fxdiv(int2fx(level->texture_width * 2), lambda), cam->fov));
		int a_x_offs = fx2int(fxdiv((a_x - (level->a_x_range / 2)), lambda) * PIX_PER_BLOCK);

		int draw_start = -line_height / 2 + M7_RIGHT - a_x_offs;
		draw_start = CLAMP(draw_start, 0, M7_RIGHT);
		int draw_end = line_height / 2 + M7_RIGHT - a_x_offs;
		draw_end = CLAMP(draw_end, M7_RIGHT, SCREEN_WIDTH + 1);

		/* pb and pd aren't used (q_y is implicitly zero) */
		bg_aff_ptr->pb = lambda;
		bg_aff_ptr->pd = 0;

		/* calculate wall affine */
		static int wall_enabled = 1;
		if (a_z < int2fx(17)) {
			if (!wall_enabled) {
				REG_DISPCNT ^= DCNT_BG2;
				wall_enabled = 1;
			}

			FIXED yb = (h - M7_TOP) * sin_theta - M7_D * cos_theta;
			if (yb == 0) { yb = 1; }
			FIXED lam = fxdiv((a_z - float2fx(16.5)) * PIX_PER_BLOCK, yb);

			wall_aff_ptr->pa = lam;

			FIXED zb = (h - M7_TOP) * cos_theta + M7_D * sin_theta;
			wall_aff_ptr->dx = fxadd(a_x * PIX_PER_BLOCK, lam * M7_LEFT);
			wall_aff_ptr->dy = fxadd(a_y * PIX_PER_BLOCK, fxmul(lam, zb));

			/* update windowing */
			if ((in_range(wall_aff_ptr->dy, int2fx(0), int2fx(40)) ||
				 in_range(wall_aff_ptr->dy, int2fx(128), int2fx(256)))) {
				line_height = fx2int(fxmul(fxdiv(int2fx(128 * 2), lam), cam->fov));
				a_x_offs = fx2int(fxdiv((a_x - (3 * level->a_x_range / 4)), lam) * PIX_PER_BLOCK);

				int draw_start_p = -line_height / 2 + M7_RIGHT - a_x_offs;
				draw_start_p = CLAMP(draw_start_p, 0, M7_RIGHT);
				draw_start = MIN(draw_start, draw_start_p);
				int draw_end_p = line_height / 2 + M7_RIGHT - a_x_offs;
				draw_end_p = CLAMP(draw_end_p, M7_RIGHT + 1, SCREEN_WIDTH + 1);
				draw_end = MAX(draw_end, draw_end_p);
			}
		} else {
			if (wall_enabled) {
				REG_DISPCNT ^= DCNT_BG2;
				wall_enabled = 0;
			}
		}

		/* apply windowing */
		*winh_ptr = WIN_BUILD((u8)draw_end, (u8)draw_start);

		bg_aff_ptr++;
		wall_aff_ptr++;
		winh_ptr++;
	}
	level->bgaff[SCREEN_HEIGHT] = level->bgaff[0];
	level->wallaff[SCREEN_HEIGHT] = level->wallaff[0];
	level->winh[SCREEN_HEIGHT] = level->winh[0];
}
