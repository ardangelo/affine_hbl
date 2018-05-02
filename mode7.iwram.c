#include <tonc.h>

#include "mode7.h"

#define RAYCAST_FREQ 1

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

m7_precompute pre;

/* raycasting prototypes */

typedef struct {
	FIXED perp_wall_dist;
	FIXED dist_y_0, dist_z_0;
	FIXED delta_map_y, delta_map_z;
	FIXED delta_dist_y, delta_dist_z;
	FIXED ray_y, ray_z, inv_ray_y, inv_ray_z;
	int map_y_0, map_z_0;
} raycast_input_t;

typedef struct {
	int enabled;
	enum {N_SIDE, S_SIDE, E_SIDE, W_SIDE} side;
	FIXED perp_wall_dist;
	FIXED dist_y, dist_z;
	int map_y, map_z;
} raycast_output_t;

IWRAM_CODE static void init_raycast(const m7_cam_t *cam, int h, raycast_input_t *rin_ptr);
IWRAM_CODE static void raycast(const m7_level_t *level, const raycast_input_t *rin, raycast_output_t *rout_ptr);
IWRAM_CODE static void compute_affines(const m7_level_t *level, const raycast_input_t *rin, const raycast_output_t *rout, FIXED lambda, BG_AFFINE *bg_aff_ptr);
IWRAM_CODE static void compute_windows(const m7_level_t *level, const int *extent, FIXED lambda, u16 *winh_ptr);

/* public function implementations */

IWRAM_CODE void
m7_hbl() {
	int vc = REG_VCOUNT;

	/* apply affine */
	BG_AFFINE *bga;

	REG_BG_AFFINE[3] = wall_level.bgaff[vc + 1];

	/* hide the wall if applicable */
	bga = &wall_level.bgaff[(vc + 2) % SCREEN_HEIGHT];
	static int wall_hidden = 0;
	if (!wall_hidden && (bga->pa == 0)) {
		REG_DISPCNT = (REG_DISPCNT & ~DCNT_MODE2) | DCNT_MODE1;
		wall_hidden = 1;
	} else if (wall_hidden && (bga->pa != 0)) {
		REG_DISPCNT = (REG_DISPCNT & ~DCNT_MODE1) | DCNT_MODE2;
		wall_hidden = 0;
	}

	bga = &floor_level.bgaff[vc + 1];
	REG_BG_AFFINE[2] = *bga;

	/* shading */
	u32 ey = bga->pb >> 7;
	if (ey > 16) { ey = 16; }
	REG_BLDY = BLDY_BUILD(ey);

	/* windowing */
	if (!wall_hidden) {
		u8 draw_start = MIN(floor_level.winh[vc + 1] >> 8, wall_level.winh[vc + 1] >> 8);
		u8 draw_end  = MAX(floor_level.winh[vc + 1] & 0xFF, wall_level.winh[vc + 1] & 0xFF);
		REG_WIN0H = WIN_BUILD(draw_end, draw_start);
	} else {
		REG_WIN0H = floor_level.winh[vc + 1];
	}
	REG_WIN0V = SCREEN_HEIGHT;
}

IWRAM_CODE void
m7_prep_affines(m7_level_t *level_2, m7_level_t *level_3) {
	raycast_input_t rin;
	raycast_output_t routs[2];
	m7_level_t *levels[2] = {level_2, level_3};

	m7_cam_t *cam = level_2->camera;

	for (int h = 0; h < SCREEN_HEIGHT; h += RAYCAST_FREQ) {
		init_raycast(cam, h, &rin);

		FIXED lambda = 0;
		for (int bg = 0; bg < 2; bg++) {
			raycast(levels[bg], &rin, &routs[bg]);

			/* compute the affines / windows only if raycast finds a renderable wall */
			if (!routs[bg].enabled) {
				levels[bg]->bgaff[h].pa = 0;
				levels[bg]->winh[h]     = WIN_BUILD(M7_RIGHT, M7_RIGHT);
			} else {
				lambda = fxmul(routs[bg].perp_wall_dist, pre.inv_fov_x_ppb);

				compute_affines(levels[bg], &rin, &routs[bg], lambda, &levels[bg]->bgaff[h]);

				/* extent will correctly size window (texture can be transparent) */
				int *extent = &levels[bg]->window_extents[routs[bg].map_y * 2];
				compute_windows(levels[bg], extent, lambda, &levels[bg]->winh[h]);
			}

			for (int i = 1; i < RAYCAST_FREQ; i++) {
				levels[bg]->bgaff[h + i] = levels[bg]->bgaff[h];
				levels[bg]->winh[h + i]  = levels[bg]->winh[h];
			}
		}

		/* for shading. pb and pd aren't used (q_y is implicitly zero) */
		level_3->bgaff[h].pb = lambda;
	}

	/* needed to correctly scale last scanline */
	level_2->bgaff[SCREEN_HEIGHT] = level_2->bgaff[0];
	level_2->winh[SCREEN_HEIGHT]  = level_2->winh[0];
	level_3->bgaff[SCREEN_HEIGHT] = level_3->bgaff[0];
	level_3->winh[SCREEN_HEIGHT]  = level_3->winh[0];
}

/* raycasting implementations */

IWRAM_CODE static void init_raycast(const m7_cam_t *cam, int h, raycast_input_t *rin_ptr) {
	raycast_input_t rin;

	/* sines and cosines of pitch */
	FIXED cos_theta = cam->v.y; // 8f
	FIXED sin_theta = cam->w.y; // 8f

	/* precomputed: x_c ray intersect in camera plane */

	/* ray components in world space */
	rin.ray_y = fxadd(sin_theta, fxmul(fxmul(cam->fov, cos_theta), pre.x_cs[h]));
	if (rin.ray_y == 0) { rin.ray_y = 1; }
	rin.ray_z = fxsub(cos_theta, fxmul(fxmul(cam->fov, sin_theta), pre.x_cs[h]));
	if (rin.ray_z == 0) { rin.ray_z = 1; }

	/* map coordinates */
	rin.map_y_0 = fx2int(cam->pos.y);
	rin.map_z_0 = fx2int(cam->pos.z);

	/* ray lengths to next x / z side */
	rin.inv_ray_y = fxdiv(int2fx(1), rin.ray_y);
	rin.delta_dist_y = ABS(rin.inv_ray_y);
	rin.inv_ray_z = fxdiv(int2fx(1), rin.ray_z);
	rin.delta_dist_z = ABS(rin.inv_ray_z);

	/* initialize map / distance steps */
	if (rin.ray_y < 0) {
		rin.delta_map_y = -1;
		rin.dist_y_0 = fxmul(
			fxsub(cam->pos.y, int2fx(rin.map_y_0)),
			rin.delta_dist_y);
	} else {
		rin.delta_map_y = 1;
		rin.dist_y_0 = fxmul(
			fxsub(int2fx(rin.map_y_0 + 1), cam->pos.y),
			rin.delta_dist_y);
	}
	if (rin.ray_z < 0) {
		rin.delta_map_z = -1;
		rin.dist_z_0 = fxmul(
			fxsub(cam->pos.z, int2fx(rin.map_z_0)),
			rin.delta_dist_z);
	} else {
		rin.delta_map_z = 1;
		rin.dist_z_0 = fxmul(
			fxsub(int2fx(rin.map_z_0 + 1), cam->pos.z),
			rin.delta_dist_z);
	}

	/* apply raytrace preparation */
	*rin_ptr = rin;
}

IWRAM_CODE static void
raycast(const m7_level_t *level, const raycast_input_t *rin, raycast_output_t *rout_ptr) {
	raycast_output_t rout;

	rout.dist_y = rin->dist_y_0;
	rout.dist_z = rin->dist_z_0;
	rout.map_y  = rin->map_y_0;
	rout.map_z  = rin->map_z_0;
	int hit = 0;

	while (!hit) {
		if (rout.dist_y < rout.dist_z) {
			rout.dist_y += rin->delta_dist_y;
			rout.map_y  += rin->delta_map_y;
			rout.side    = (rin->delta_map_y < 0) ? N_SIDE : S_SIDE;
		} else {
			rout.dist_z += rin->delta_dist_z;
			rout.map_z  += rin->delta_map_z;
			rout.side    = (rin->delta_map_z < 0) ? W_SIDE : E_SIDE;
		}

		if ((hit = level->blocks[rout.map_y * level->blocks_width + rout.map_z])) {
			if (hit == 1) {
				rout_ptr->enabled = 0;
				return;
			} else {
				rout.enabled = 1;
			}
		}
	}

	/* calculate wall distance */
	if ((rout.side == N_SIDE) || (rout.side == S_SIDE)) {
		rout.perp_wall_dist = fxmul(
			fxadd(
				fxsub(int2fx(rout.map_y), level->camera->pos.y),
				int2fx(1 - rin->delta_map_y) / 2),
			rin->inv_ray_y);
	} else {
		rout.perp_wall_dist = fxmul(
			fxadd(
				fxsub(int2fx(rout.map_z), level->camera->pos.z),
				int2fx(1 - rin->delta_map_z) / 2),
			rin->inv_ray_z);
	}
	if (rout.perp_wall_dist == 0) { rout.perp_wall_dist = 1; }

	/* apply raytrace result */
	*rout_ptr = rout;
}

IWRAM_CODE static void
compute_affines(const m7_level_t *level, const raycast_input_t *rin, const raycast_output_t *rout, FIXED lambda, BG_AFFINE *bg_aff_ptr) {

	/* location vector */
	FIXED a_x = level->camera->pos.x; // 8f
	FIXED a_y = level->camera->pos.y; // 8f
	FIXED a_z = level->camera->pos.z; // 8f

	/* scaling */
	bg_aff_ptr->pa = lambda;

	/* camera x-position */
	bg_aff_ptr->dx = fxadd(fxmul(lambda, int2fx(M7_LEFT)), a_x * PIX_PER_BLOCK);

	/* move side to correct texture source */
	if (((rout->side == E_SIDE) || (rout->side == W_SIDE))) {
		bg_aff_ptr->dx += int2fx(level->texture_width);
	}

	/* calculate angle corrections (angles are .12f) */
	FIXED correction;
	if ((rout->side == N_SIDE) || (rout->side == S_SIDE)) {
		correction = fxadd(fxmul(rout->perp_wall_dist, rin->ray_z), a_z);
	} else {
		correction = fxadd(fxmul(rout->perp_wall_dist, rin->ray_y), a_y);
	}
	correction *= PIX_PER_BLOCK;

	/* apply correction */
	bg_aff_ptr->dy = correction;

	/* wrap texture for ceiling */
	if ((((rout->side == N_SIDE) || (rout->side == S_SIDE)) && (rin->ray_y > 0)) ||
		(((rout->side == E_SIDE) || (rout->side == W_SIDE)) && (rin->ray_z < 0))) {
		bg_aff_ptr->dy = fxsub(int2fx(level->texture_height), bg_aff_ptr->dy);
	}

	if (rout->side == N_SIDE) {
		bg_aff_ptr->dx += int2fx(level->texture_height);
	}
	else if (rout->side == W_SIDE) {
		bg_aff_ptr->dy += int2fx(level->texture_width);
	}

	/* offset down for bg2 */
	if (level->bgcnt & BG_PRIO(1)) {
		bg_aff_ptr->dy += int2fx(level->texture_height);
	}
}

IWRAM_CODE static void
compute_windows(const m7_level_t *level, const int *extent, FIXED lambda, u16 *winh_ptr) {
	FIXED extent_width = fxmul(
		int2fx((extent[1] - extent[0]) * PIX_PER_BLOCK), // normal extent width
		level->camera->fov); // adjust for fov

	FIXED a_x_offs = fxsub(
		(level->a_x_range + int2fx(extent[0])) / 2, // origin relative to center of level
		level->camera->pos.x // adjust by camera position
	) * PIX_PER_BLOCK; // scale up to block size

	FIXED inv_lambda = fxdiv(int2fx(1), lambda);

	int draw_start = 1 + M7_RIGHT + fx2int(
		fxmul(
			fxsub(a_x_offs, extent_width),
			inv_lambda));
	int draw_end = M7_RIGHT + fx2int(
		fxmul(
			fxadd(a_x_offs, extent_width),
			inv_lambda));

	/* clamp to screen size */
	draw_start = CLAMP(draw_start, 0, SCREEN_WIDTH);
	draw_end = CLAMP(draw_end, 0, SCREEN_WIDTH + 1);

	*winh_ptr = WIN_BUILD((u8)draw_end, (u8)draw_start);
}