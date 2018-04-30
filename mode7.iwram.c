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

/* raycasting prototypes */

typedef struct {
	FIXED perp_wall_dist;
	FIXED dist_y_0, dist_z_0;
	FIXED delta_map_y, delta_map_z;
	FIXED delta_dist_y, delta_dist_z;
	FIXED ray_y, ray_z;
	int map_y_0, map_z_0;
} raycast_input_t;

typedef struct {
	int side;
	FIXED perp_wall_dist;
	FIXED dist_y, dist_z;
	int map_y, map_z;
} raycast_output_t;

IWRAM_CODE static void init_raycast(const m7_cam_t *cam, int h, raycast_input_t *rin_ptr);
IWRAM_CODE static void raycast(const m7_level_t *level, const raycast_input_t *rin, raycast_output_t *rout_ptr);
IWRAM_CODE static void compute_affines(const m7_level_t *level, const raycast_input_t *rin, const raycast_output_t *rout, FIXED lambda, BG_AFFINE *bg_aff_ptr);
IWRAM_CODE static void compute_windows(const m7_level_t *level, FIXED lambda, u16 *winh_ptr);

/* public function implementations */

IWRAM_CODE void
m7_hbl() {
	int vc = REG_VCOUNT;

	/* apply affine */
	REG_BG_AFFINE[2] = wall_level.bgaff[vc + 1];

	BG_AFFINE *bga   = &floor_level.bgaff[vc + 1];
	REG_BG_AFFINE[3] = *bga;

	/* shading */
	u32 ey = bga->pb >> 7;
	if (ey > 16) { ey = 16; }
	REG_BLDY = BLDY_BUILD(ey);

	/* windowing */
	REG_WIN0H = floor_level.winh[vc + 1];
	REG_WIN0V = SCREEN_HEIGHT;
}

IWRAM_CODE void
m7_prep_affines(m7_level_t *level) {
	raycast_input_t rin;
	raycast_output_t rout;

	int h;
	BG_AFFINE *bg_aff_ptr;
	u16 *winh_ptr;
	for (h = 0, bg_aff_ptr = &level->bgaff[0], winh_ptr = &level->winh[0];
		h < SCREEN_HEIGHT;
		h++, bg_aff_ptr++, winh_ptr++) {

		init_raycast(level->camera, h, &rin);
		raycast(level, &rin, &rout);

		FIXED lambda = fxdiv(rout.perp_wall_dist, level->camera->fov) / PIX_PER_BLOCK;

		compute_affines(level, &rin, &rout, lambda, bg_aff_ptr);
		compute_windows(level, lambda, winh_ptr);

		/* for shading. pb and pd aren't used (q_y is implicitly zero) */
		bg_aff_ptr->pb = lambda;
	}

	/* needed to correctly scale last scanline */
	level->bgaff[SCREEN_HEIGHT] = level->bgaff[0];
	level->winh[SCREEN_HEIGHT]  = level->winh[0];
}

/* raycasting implementations */

IWRAM_CODE static void init_raycast(const m7_cam_t *cam, int h, raycast_input_t *rin_ptr) {
	raycast_input_t rin;

	/* sines and cosines of pitch */
	FIXED cos_theta = cam->v.y; // 8f
	FIXED sin_theta = cam->w.y; // 8f

	/* ray intersect in camera plane */
	FIXED x_c = fxsub(2 * fxdiv(int2fx(h), int2fx(SCREEN_HEIGHT)), int2fx(1));

	/* ray components in world space */
	rin.ray_y = fxadd(sin_theta, fxmul(fxmul(cam->fov, cos_theta), x_c));
	if (rin.ray_y == 0) { rin.ray_y = 1; }
	rin.ray_z = fxsub(cos_theta, fxmul(fxmul(cam->fov, sin_theta), x_c));
	if (rin.ray_z == 0) { rin.ray_z = 1; }

	/* map coordinates */
	rin.map_y_0 = fx2int(cam->pos.y);
	rin.map_z_0 = fx2int(cam->pos.z);

	/* ray lengths to next x / z side */
	rin.delta_dist_y = ABS(fxdiv(int2fx(1), rin.ray_y));
	rin.delta_dist_z = ABS(fxdiv(int2fx(1), rin.ray_z));

	/* initialize map / distance steps */
	if (rin.ray_y < 0) {
		rin.delta_map_y = -1;
		rin.dist_y_0 = fxmul(fxsub(cam->pos.y, int2fx(rin.map_y_0)), rin.delta_dist_y);
	} else {
		rin.delta_map_y = 1;
		rin.dist_y_0 = fxmul(fxsub(int2fx(rin.map_y_0 + 1), cam->pos.y), rin.delta_dist_y);
	}
	if (rin.ray_z < 0) {
		rin.delta_map_z = -1;
		rin.dist_z_0 = fxmul(fxsub(cam->pos.z, int2fx(rin.map_z_0)), rin.delta_dist_z);
	} else {
		rin.delta_map_z = 1;
		rin.dist_z_0 = fxmul(fxsub(int2fx(rin.map_z_0 + 1), cam->pos.z), rin.delta_dist_z);
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
			rout.side    = 0;
		} else {
			rout.dist_z += rin->delta_dist_z;
			rout.map_z  += rin->delta_map_z;
			rout.side    = 1;
		}

		if (level->blocks[rout.map_y * level->blocks_width + rout.map_z] > 0) {
			hit = 1;
		}
	}

	/* calculate wall distance */
	if (rout.side == 0) {
		rout.perp_wall_dist = fxdiv(fxadd(fxsub(int2fx(rout.map_y), level->camera->pos.y), fxdiv(int2fx(1 - rin->delta_map_y), int2fx(2))), rin->ray_y);
	} else {
		rout.perp_wall_dist = fxdiv(fxadd(fxsub(int2fx(rout.map_z), level->camera->pos.z), fxdiv(int2fx(1 - rin->delta_map_z), int2fx(2))), rin->ray_z);
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
	if (rout->side == 1) {
		bg_aff_ptr->dx += int2fx(level->texture_width);
	}

	/* calculate angle corrections (angles are .12f) */
	FIXED correction;
	if (rout->side == 0) {
		correction = fxadd(fxmul(rout->perp_wall_dist, rin->ray_z), a_z);
	} else {
		correction = fxadd(fxmul(rout->perp_wall_dist, rin->ray_y), a_y);
	}
	correction *= PIX_PER_BLOCK;

	/* wrap texture for ceiling */
	if (((rout->side == 0) && (rin->ray_y > 0)) ||
		((rout->side == 1) && (rin->ray_z < 0))) {
		correction = fxsub(int2fx(level->texture_height), correction);
	}

	/* apply correction */
	bg_aff_ptr->dy = correction;
}

IWRAM_CODE static void
compute_windows(const m7_level_t *level, FIXED lambda, u16 *winh_ptr) {
	int line_height = fx2int(fxmul(fxdiv(int2fx(level->texture_width * 2), lambda), level->camera->fov));
	int a_x_offs = fx2int(fxdiv((level->camera->pos.x - (level->a_x_range / 2)), lambda) * PIX_PER_BLOCK);

	int draw_start = -line_height / 2 + M7_RIGHT - a_x_offs;
	draw_start = CLAMP(draw_start, 0, M7_RIGHT);

	int draw_end = line_height / 2 + M7_RIGHT - a_x_offs;
	draw_end = CLAMP(draw_end, M7_RIGHT, SCREEN_WIDTH + 1);

	*winh_ptr = WIN_BUILD((u8)draw_end, (u8)draw_start);
}