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
	VECTOR pos;
	FIXED dist_y_0, dist_z_0;
	FIXED delta_map_y, delta_map_z;
	FIXED delta_dist_y, delta_dist_z;
	FIXED ray_y, ray_z, inv_ray_y, inv_ray_z;
	int map_y_0, map_z_0;
} raycast_input_t;


enum sides {N_SIDE, S_SIDE, E_SIDE, W_SIDE};
typedef struct {
	enum sides side;
	FIXED perp_wall_dist;
	FIXED dist_y, dist_z;
	int map_y, map_z;
} raycast_output_t;

IWRAM_CODE static void init_raycast(const M7Camera *cam, int h, raycast_input_t *rin_ptr);
IWRAM_CODE static int raycast(const M7Map *map, const raycast_input_t *rin, raycast_output_t *rout_ptr);
IWRAM_CODE static void compute_affines(const M7Map *map, const raycast_input_t *rin, const raycast_output_t *rout, FIXED lambda, BG_AFFINE *bg_aff_ptr);
IWRAM_CODE static void compute_windows(const M7Map *map, const raycast_input_t *rin, const raycast_output_t *rout, FIXED lambda, u16 *winh_ptr);

/* M7Level affine / window calculation implementations */

IWRAM_CODE
void m7_hbl() {
	fanLevel.HBlank();
}

inline IWRAM_CODE void
M7Level::HBlank() {
	int vc = REG_VCOUNT;

	/* apply wall (secondary) affine */
	BG_AFFINE *bga;
	REG_BG_AFFINE[3] = maps[1]->bgaff[vc + 1];

	/* hide the wall (bg2) if applicable by flipping to mode 1 */
	bga = &maps[1]->bgaff[(vc + 2) > SCREEN_HEIGHT ? 0 : vc + 2];
	static int wall_hidden = 0;
	if (!wall_hidden && (bga->pa == 0)) {
		REG_DISPCNT = (REG_DISPCNT & ~DCNT_MODE2) | DCNT_MODE1;
		wall_hidden = 1;
	} else if (wall_hidden && (bga->pa != 0)) {
		REG_DISPCNT = (REG_DISPCNT & ~DCNT_MODE1) | DCNT_MODE2;
		wall_hidden = 0;
	}

	/* apply floor (primary) affine */
	bga = &maps[0]->bgaff[vc + 1];
	REG_BG_AFFINE[2] = *bga;

	/* apply shading */
	u32 ey = bga->pb >> 7;
	if (ey > 16) { ey = 16; }
	REG_BLDY = BLDY_BUILD(ey);

	if (!wall_hidden) {
		/* todo: use win0 and win1 instead of just combining into win0 */
		u8 draw_start = MIN(maps[0]->winh[vc + 1] >> 8, maps[1]->winh[vc + 1] >> 8);
		u8 draw_end  = MAX(maps[0]->winh[vc + 1] & 0xFF, maps[1]->winh[vc + 1] & 0xFF);
		REG_WIN0H = WIN_BUILD(draw_end, draw_start);
	} else {
		REG_WIN0H = maps[0]->winh[vc + 1];
	}
}

IWRAM_CODE void
M7Level::prepAffines() {
	raycast_input_t rin;
	raycast_output_t routs[2];

	for (int h = 0; h < SCREEN_HEIGHT; h += RAYCAST_FREQ) {
		init_raycast(cam, h, &rin);

		for (int bg = 0; bg < 2; bg++) {
			/* compute the affines / windows only if raycast finds a renderable wall */
			if (raycast(maps[bg], &rin, &routs[bg])) {
				FIXED lambda = fxmul(routs[bg].perp_wall_dist, pre.inv_fov_x_ppb);

				compute_affines(maps[bg], &rin, &routs[bg], lambda, &maps[bg]->bgaff[h]);

				/* extent will correctly size window (texture can be transparent) */
				compute_windows(maps[bg], &rin, &routs[bg], lambda, &maps[bg]->winh[h]);

				/* for shading. pb and pd aren't used (q_y is implicitly zero) */
				maps[bg]->bgaff[h].pb = lambda;
			} else {
				maps[bg]->bgaff[h].pa = 0;
				maps[bg]->winh[h]     = WIN_BUILD(M7_RIGHT, M7_RIGHT);
			}

			/* duplicate affine matrices if rendering low-res */
			for (int i = 1; i < RAYCAST_FREQ; i++) {
				maps[bg]->bgaff[h + i] = maps[bg]->bgaff[h];
				maps[bg]->winh[h + i]  = maps[bg]->winh[h];
			}
		}
	}

	/* needed to correctly scale last scanline */
	for (int bg = 0; bg < 2; bg++) {
		maps[bg]->bgaff[SCREEN_HEIGHT] = maps[bg]->bgaff[0];
		maps[bg]->winh[SCREEN_HEIGHT]  = maps[bg]->winh[0];
	}
}

/* raycasting implementations */

IWRAM_CODE static void
init_raycast(const M7Camera *cam, int h, raycast_input_t *rin_ptr) {
	raycast_input_t rin;

	/* camera position */
	rin.pos = cam->pos;

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

IWRAM_CODE static int
raycast(const M7Map *map, const raycast_input_t *rin, raycast_output_t *rout_ptr) {
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

		if ((hit = map->blocks[rout.map_y * map->blocksDepth + rout.map_z])) {
			/* defined raycast map value 1 to be "end, no texture" */
			if (hit == 1) {
				return 0;
			}
		}
	}

	/* calculate wall distance */
	if ((rout.side == N_SIDE) || (rout.side == S_SIDE)) {
		rout.perp_wall_dist = fxmul(
			fxadd(
				fxsub(int2fx(rout.map_y), rin->pos.y),
				int2fx(1 - rin->delta_map_y) / 2),
			rin->inv_ray_y);
	} else {
		rout.perp_wall_dist = fxmul(
			fxadd(
				fxsub(int2fx(rout.map_z), rin->pos.z),
				int2fx(1 - rin->delta_map_z) / 2),
			rin->inv_ray_z);
	}
	if (rout.perp_wall_dist == 0) { rout.perp_wall_dist = 1; }

	/* apply raytrace result */
	*rout_ptr = rout;

	return 1;
}

IWRAM_CODE static void
compute_affines(const M7Map *map, const raycast_input_t *rin, const raycast_output_t *rout, FIXED lambda, BG_AFFINE *bg_aff_ptr) {

	/* scaling */
	bg_aff_ptr->pa = lambda;

	/* camera x-position */
	bg_aff_ptr->dx = fxadd(fxmul(lambda, int2fx(M7_LEFT)), rin->pos.x * PIX_PER_BLOCK);

	/* move side to correct texture source */
	if (((rout->side == E_SIDE) || (rout->side == W_SIDE))) {
		bg_aff_ptr->dx += int2fx(map->textureWidth);
	} else if (rout->side == N_SIDE) {
		bg_aff_ptr->dx += int2fx(2 * map->textureWidth);
	}

	/* calculate angle corrections (angles are .12f) */
	FIXED correction;
	if ((rout->side == N_SIDE) || (rout->side == S_SIDE)) {
		correction = fxadd(fxmul(rout->perp_wall_dist, rin->ray_z), rin->pos.z);
	} else {
		correction = fxadd(fxmul(rout->perp_wall_dist, rin->ray_y), rin->pos.y);
	}
	correction *= PIX_PER_BLOCK;

	/* apply correction */
	bg_aff_ptr->dy = correction;

	/* wrap texture for ceiling */
	if ((((rout->side == N_SIDE) || (rout->side == S_SIDE)) && (rin->ray_y > 0)) ||
		(((rout->side == E_SIDE) || (rout->side == W_SIDE)) && (rin->ray_z < 0))) {
		bg_aff_ptr->dy = fxsub(int2fx(map->textureDepth), bg_aff_ptr->dy);
	}

	/* offset down for bg2 */
	if (map->bgcnt & BG_PRIO(1)) {
		bg_aff_ptr->dy += int2fx(map->textureDepth);
	}
}

IWRAM_CODE static void
compute_windows(const M7Map *map, const raycast_input_t *rin, const raycast_output_t *rout, FIXED lambda, u16 *winh_ptr) {
	FIXED a_x_offs = fxsub(
		map->extentOffs[rout->map_y], // origin relative to center of map
		rin->pos.x // adjust by camera position
	) * PIX_PER_BLOCK; // scale up to block size

	FIXED inv_lambda = fxdiv(int2fx(1), lambda);

	int draw_start = 1 + M7_RIGHT + fx2int(
		fxmul(
			fxsub(a_x_offs, map->extentWidths[rout->map_y]),
			inv_lambda));
	int draw_end = M7_RIGHT + fx2int(
		fxmul(
			fxadd(a_x_offs, map->extentWidths[rout->map_y]),
			inv_lambda));

	/* clamp to screen size */
	draw_start = CLAMP(draw_start, 0, SCREEN_WIDTH);
	draw_end = CLAMP(draw_end, 0, SCREEN_WIDTH + 1);

	*winh_ptr = WIN_BUILD((u8)draw_end, (u8)draw_start);
}