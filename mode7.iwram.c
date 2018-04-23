#include <tonc.h>

#include "mode7.h"

static const int worldMap[24][24]= {
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,2,2,2,2,2,0,0,0,0,3,0,3,0,3,0,0,0,1},
  {1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,3,0,0,0,3,0,0,0,1},
  {1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,2,2,0,2,2,0,0,0,0,3,0,3,0,3,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,4,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,0,0,0,5,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,4,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

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
	if (bga->pb == 1) {	
		BFN_SET(REG_DISPCNT, DCNT_MODE0, DCNT_MODE);
		REG_BG2CNT = m7_level.bgcnt_sky;
	} else {
		BFN_SET(REG_DISPCNT, DCNT_MODE1, DCNT_MODE);
		REG_BG2CNT = m7_level.bgcnt_floor;
	}
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

	FIXED plane_y = float2fx(0.0);
	FIXED plane_z = float2fx(0.66);
	for (int h = start_line; h < SCREEN_HEIGHT; h++) {
		/* ray intersect in camera plane */
		FIXED x_c = fxsub(2 * fxdiv(int2fx(h), int2fx(SCREEN_HEIGHT)), int2fx(1));

		/* ray components in world space */
		FIXED ray_y = fxadd(sin_theta, fxmul(plane_y, x_c));
		if (ray_y == 0) { ray_y = 1; }
		FIXED ray_z = fxadd(cos_theta, fxmul(plane_z, x_c));
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
			dist_y = fxmul(fxadd(int2fx(map_y + 1), a_y), delta_dist_y);
		}
		if (ray_z < 0) {
			delta_map_z = -1;
			dist_z = fxmul(fxsub(a_z, int2fx(map_z)), delta_dist_z);
		} else {
			delta_map_z = 1;
			dist_z = fxmul(fxadd(int2fx(map_z + 1), a_z), delta_dist_z);
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

			if (worldMap[map_y][map_z] > 0) {
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
		if (perp_wall_dist == 0) {
			perp_wall_dist = 1;
		}

		int line_height = fx2int(fxdiv(int2fx(SCREEN_WIDTH), perp_wall_dist));
		int draw_start = -line_height / 2 + SCREEN_WIDTH / 2;
		if (draw_start < 0) { draw_start = 0; }
		int draw_end = line_height / 2 + SCREEN_WIDTH / 2;
		if (draw_end >= SCREEN_WIDTH) { draw_end = SCREEN_WIDTH; }

		/* apply windowing */
		*winh_ptr = WIN_BUILD((u8)draw_end, (u8)draw_start);
		winh_ptr++;

		/* build affine matrices */
		FIXED lambda = perp_wall_dist;

		bg_aff_ptr->pa = lambda;
		bg_aff_ptr->pd = lambda;

		bg_aff_ptr->pb = side;

		bg_aff_ptr->dx = 0;
		bg_aff_ptr->dy = int2fx(h);

		bg_aff_ptr++;
	}
	level->bgaff[SCREEN_HEIGHT] = level->bgaff[0];
	level->winh[SCREEN_HEIGHT] = level->winh[0];
}
