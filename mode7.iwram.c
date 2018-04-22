#include <tonc.h>

#include "mode7.h"

IWRAM_CODE void m7_hbl() {
	int vc = REG_VCOUNT;
	int horz = m7_level.horizon;

	if (!IN_RANGE(vc, horz, SCREEN_HEIGHT)) {
		return;
	}

	if (vc == horz) {
		BFN_SET(REG_DISPCNT, DCNT_MODE1, DCNT_MODE);
		REG_BG2CNT = m7_level.bgcnt_floor;
	}
	
	BG_AFFINE *bga = &m7_level.bgaff[vc + 1];
	REG_BG_AFFINE[2] = *bga;

	REG_WIN0H = m7_level.winh[vc + 1];
	REG_WIN0V = 0 << 8 | SCREEN_HEIGHT;
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

	FIXED plane_x = 0 << 8;
	FIXED plane_z = 1 << 8;
	for (int h = start_line; h < SCREEN_HEIGHT; h++) {
		FIXED x_c = 2 * (h << 8) / SCREEN_HEIGHT - (1 << 8);

		FIXED dir_x = cos_theta + ((plane_x * x_c) >> 8);
		if (dir_x == 0) {
			dir_x = 1;
		}
		FIXED dir_z = sin_theta + ((plane_z * x_c) >> 8);
		if (dir_z == 0) {
			dir_z = 1;
		}

		int map_x = a_x >> 8;
		int map_z = a_z >> 8;

		FIXED delta_dist_x = ABS((1 << 16) / dir_x);
		FIXED delta_dist_z = ABS((1 << 16) / dir_z);

		int step_x = 0;
		int step_z = 0;
		FIXED side_dist_x = 0;
		FIXED side_dist_z = 0;

		if (dir_x < 0) {
			step_x = -1;
			side_dist_x = ((a_x - (map_x << 8)) * delta_dist_x) >> 8;
		} else {
			step_x = 1;
			side_dist_x = (((map_x << 8) + (1 << 8) - a_x) * delta_dist_x) >> 8;
		}

		if (dir_z < 0) {
			step_z = -1;
			side_dist_z = ((a_z - (map_z << 8)) * delta_dist_z) >> 8;
		} else {
			step_z = 1;
			side_dist_z = (((map_z << 8) + (1 << 8) - a_z) * delta_dist_z) >> 8;
		}

		int hit = 0;
		int side = 0;
		while (!hit) {
			if (side_dist_x < side_dist_z) {
				side_dist_x += delta_dist_x;
				map_x += step_x;
				side = 0;
			} else {
				side_dist_z += delta_dist_z;
				map_z += step_z;
				side = 1;
			}

			if (m7_level.blocks[map_x][map_z] > 0) {
				hit = 1;
			}
		}

		FIXED perp_wall_dist = 0;
		if (side == 0) {
			perp_wall_dist = (((map_x << 8) - a_x + ((1 - step_x) << 8) / 2) * delta_dist_x) >> 8;
		} else {
			perp_wall_dist = (((map_z << 8) - a_z + ((1 - step_z) << 8) / 2) * delta_dist_z) >> 8;
		}
		if (perp_wall_dist == 0) {
			perp_wall_dist = 1;
		}

		int line_height = ((SCREEN_WIDTH << 16) / perp_wall_dist) >> 8;
		int draw_start = -line_height / 2 + SCREEN_WIDTH / 2;
		if (draw_start < 0) { draw_start = 0; }
		int draw_end = line_height / 2 + SCREEN_WIDTH / 2;
		if (draw_end >= SCREEN_WIDTH) { draw_end = SCREEN_WIDTH; }

		/* apply windowing */
		*winh_ptr = draw_start << 8 | draw_end;
		winh_ptr++;

		/* build affine matrices */
		bg_aff_ptr->pa = perp_wall_dist; // 8f
		bg_aff_ptr->pd = perp_wall_dist; // 8f

		bg_aff_ptr->dx = 0;
		bg_aff_ptr->dy = (h << 8);

		bg_aff_ptr++;
	}
	level->bgaff[SCREEN_HEIGHT] = level->bgaff[0];
	level->winh[SCREEN_HEIGHT] = level->winh[0];
}
