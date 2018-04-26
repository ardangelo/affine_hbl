#include <tonc.h>

#include "mode7.h"

#define TRIG_ANGLE_MAX 0xFFFF

IWRAM_CODE void m7_hbl() {
	int vc = REG_VCOUNT;
	int horz = m7_level.horizon;

	if (!IN_RANGE(vc, horz, SCREEN_HEIGHT)) {
		return;
	}
	
	/* apply affine */
	BG_AFFINE *bga = &m7_level.bgaff[vc + 1];
	REG_BG_AFFINE[2] = *bga;

	/* apply window */
	REG_WIN0H = m7_level.winh[vc + 1];
	REG_WIN0V = WIN_BUILD(SCREEN_HEIGHT, 0);

	/* apply shading */
	static int last_side = -1;
	if (last_side != bga->pb) {
		if (bga->pb == 1) {
			BFN_SET(REG_DISPCNT, DCNT_MODE0, DCNT_MODE);
			REG_BG2CNT = m7_level.bgcnt_sky;
		} else {
			BFN_SET(REG_DISPCNT, DCNT_MODE1, DCNT_MODE);
			REG_BG2CNT = m7_level.bgcnt_floor;
		}
	}
	last_side = bga->pb;
}

IWRAM_CODE void m7_prep_affines(m7_level_t *level) {
	if (level->horizon >= SCREEN_HEIGHT) {
		return;
	}

	int start_line = (level->horizon >= 0) ? level->horizon : 0;

	m7_cam_t *cam = level->camera;

	/* location vector */
	FIXED a_x = cam->pos.x; // 8f
	FIXED a_y = cam->pos.y; // 8f
	FIXED a_z = cam->pos.z; // 8f

	/* sines and cosines of yaw, pitch */
	FIXED cos_phi   = cam->u.x; // 8f
	FIXED sin_phi   = cam->u.z; // 8f
	FIXED cos_theta = cam->v.y; // 8f
	FIXED sin_theta = cam->w.y; // 8f

	BG_AFFINE *bg_aff_ptr = &level->bgaff[start_line];
	u16 *winh_ptr = &level->winh[start_line];

	const FIXED fov = fxdiv(int2fx(M7_TOP), int2fx(M7_D));
	const FIXED pixels_per_block = fxdiv(int2fx(level->texture_height), int2fx(level->blocks_height));
	for (int h = start_line; h < SCREEN_HEIGHT; h++) {
		/* ray intersect in camera plane */
		FIXED x_c = fxsub(2 * fxdiv(int2fx(h), int2fx(SCREEN_HEIGHT)), int2fx(1));

		/* ray components in world space */
		FIXED ray_y = fxadd(sin_theta, fxmul(fxmul(fov, cos_theta), x_c));
		if (ray_y == 0) { ray_y = 1; }
		FIXED ray_z = fxsub(cos_theta, fxmul(fxmul(fov, sin_theta), x_c));
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

		int line_height = fx2int(fxdiv(int2fx(SCREEN_WIDTH), perp_wall_dist));
		int draw_start = -line_height / 2 + M7_RIGHT;
		if (draw_start < 0) { draw_start = 0; }
		int draw_end = line_height / 2 + M7_RIGHT;
		if (draw_end >= SCREEN_WIDTH) { draw_end = SCREEN_WIDTH; }

		/* apply windowing */
		*winh_ptr = WIN_BUILD((u8)draw_end, (u8)draw_start);
		winh_ptr++;

		/* build affine matrices */
		FIXED lambda = perp_wall_dist;

		/* scaling */
		bg_aff_ptr->pa = lambda;

		bg_aff_ptr->dx = fxmul(lambda, int2fx(M7_LEFT));

		/* calculate angle corrections (angles are .12f) */
		FIXED angle_correction = fxdiv(level->texture_height * (level->camera->theta >> 4), TRIG_ANGLE_MAX >> 4);

		/* calculate displacement from position */
		FIXED position_correction;
		if (side == 0) {
			position_correction = fxadd(a_z, fxmul(perp_wall_dist, ray_z));
		} else {
			position_correction = fxadd(a_y, fxmul(perp_wall_dist, ray_y));
		}
		position_correction = fxmul(position_correction, pixels_per_block);

		/* wrap texture for ceiling */
		if (((side == 0) && (ray_y > 0)) ||
			((side == 1) && (ray_z < 0))) {
			position_correction = fxsub(int2fx(level->texture_height - 1), position_correction);
		}

		bg_aff_ptr->dy = fxadd(fxadd(int2fx(h), angle_correction), position_correction);

		/* pb and pd aren't used (q_y is implicitly zero) */
		bg_aff_ptr->pb = side;
		bg_aff_ptr->pd = alpha_correction;

		bg_aff_ptr++;
	}
	level->bgaff[SCREEN_HEIGHT] = level->bgaff[0];
	level->winh[SCREEN_HEIGHT] = level->winh[0];
}
