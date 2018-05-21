#include <utility>

#include <tonc.h>

#include "mode7.h"

#define RAYCAST_FREQ 1
#define TRIG_ANGLE_MAX 0xFFFF

M7Precompute pre;

static IWRAM_CODE compute_affine(const M7Map& map, const Ray& rin, const Ray::Casted& rout, const FixedPixel& lambda, int h);
static IWRAM_CODE compute_window(const M7Map& map, const Ray& rin, const Ray::Casted& rout, const FixedPixel& lambda, int h);

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
	for (int h = 0; h < SCREEN_HEIGHT; h += RAYCAST_FREQ) {
		const Ray rin(cam.pos, h);

		for (int bg = 0; bg < 2; bg++) {
			/* compute the affines / windows only if raycast finds a renderable wall */
			const std::optional<Ray::Casted>& rout = rin.cast(*maps[bg]);
			if (rout) {
				const FixedPixel lambda = rout->perp_wall_dist * pre.inv_fov_x_ppb;

				compute_affines(*maps[bg], rin, rout, lambda, h);

				/* extent will correctly size window (texture can be transparent) */
				compute_windows(*maps[bg], rin, rout, lambda, h);

				/* for shading. pb and pd aren't used (q_y is implicitly zero) */
				maps[bg]->bgaff[h].pb = lambda.get();
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

IWRAM_CODE
Ray(const Vector<FixedPixel>& origin_, const FixedPixel& fov, int h) : origin(origin_) {

	/* sines and cosines of pitch */
	const Point<FixedPixel> sin_cos_theta = {cam->v.y, cam->w.y};
	const Point<FixedPixel> neg_cos_sin_theta = {-cam->v.y, cam->w.y};

	/* precomputed: x_c ray intersect in camera plane */

	/* ray components in world space */
	ray = sin_cos_theta + (fov * neg_cos_sin_theta) * pre.x_cs[h];

	/* map coordinates */
	map_0 = Point<Block>(origin_);

	/* ray lengths to next x / z side */
	inv_ray = ABS(FixedPixel(1) / ray);

	/* initialize map / distance steps */
	Block deltaMapY, deltaMapZ;
	FixedPixel distY, distZ;
	if (ray.a < 0) {
		deltaMapY = Block(-1);
		distY = (origin_.a - FixedPixel(int2fx(map_0.a))) * deltaDist.a;
		rin.dist_y_0 = fxmul(
			fxsub(cam->pos.y, int2fx(rin.map_y_0)),
			rin.delta_dist_y);
	} else {
		deltaMapY = Block(1);
		rin.dist_y_0 = fxmul(
			fxsub(int2fx(rin.map_y_0 + 1), cam->pos.y),
			rin.delta_dist_y);
	}
	if (rin.ray_z < 0) {
		deltaMapZ = Block(-1);
		rin.dist_z_0 = fxmul(
			fxsub(cam->pos.z, int2fx(rin.map_z_0)),
			rin.delta_dist_z);
	} else {
		deltaMapZ = Block(1);
		rin.dist_z_0 = fxmul(
			fxsub(int2fx(rin.map_z_0 + 1), cam->pos.z),
			rin.delta_dist_z);
	}
	deltaMap = {deltaMapY, deltaMapZ};

	/* apply raytrace preparation */
	*rin_ptr = rin;
}

IWRAM_CODE const std::optional<Ray::Casted>&
Ray::cast(const M7Map& map) {
	Ray::Casted rout;

	rout.dist = rin.dist_0;
	rout.map  = rout.map_0;

	/* for casting, vector comp 'a' is y, 'b' is z */
	int hit = 0;
	while (!hit) {
		if (rout.dist.a < rout.dist.b) {
			rout.dist.a += delta_dist.a;
			rout.map.a  += delta_map.a;
			rout.side    = (delta_map.a < 0) ? Ray::sides::N : Ray::sides::S;
		} else {
			rout.dist.b += delta_dist.b;
			rout.map.b  += delta_map.b;
			rout.side    = (delta_map.b < 0) ? Ray::sides::W : Ray::sides::E;
		}

		if ((hit = map.blocks[rout.map.b * map->blocks.z + rout.map.b])) {
			/* defined raycast map value 1 to be "end, no texture" */
			if (hit == 1) {
				return std::nullopt;
			}
		}
	}

	/* calculate wall distance */
	if ((rout.side == Ray::sides::N) || (rout.side == Ray::sides::S)) {
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
compute_affines(const M7Map& map, const Ray& rin, const Ray::Casted& rout, const FixedPixel& lambda, int h) {

	/* scaling */
	bg_aff_ptr->pa = lambda;

	/* camera x-position */
	bg_aff_ptr->dx = fxadd(fxmul(lambda, int2fx(M7_LEFT)), rin->pos.x * PIX_PER_BLOCK);

	/* move side to correct texture source */
	if (((rout->side == Ray::sides::E) || (rout->side == Ray::sides::W))) {
		bg_aff_ptr->dx += int2fx(map->textureWidth);
	} else if (rout->side == Ray::sides::N) {
		bg_aff_ptr->dx += int2fx(2 * map->textureWidth);
	}

	/* calculate angle corrections (angles are .12f) */
	FIXED correction;
	if ((rout->side == Ray::sides::N) || (rout->side == Ray::sides::S)) {
		correction = fxadd(fxmul(rout->perp_wall_dist, rin->ray_z), rin->pos.z);
	} else {
		correction = fxadd(fxmul(rout->perp_wall_dist, rin->ray_y), rin->pos.y);
	}
	correction *= PIX_PER_BLOCK;

	/* apply correction */
	bg_aff_ptr->dy = correction;

	/* wrap texture for ceiling */
	if ((((rout->side == Ray::sides::N) ||
		  (rout->side == Ray::sides::S)) && (rin->ray_y > 0)) ||
		(((rout->side == Ray::sides::E) ||
		  (rout->side == Ray::sides::W)) && (rin->ray_z < 0))) {
		bg_aff_ptr->dy = fxsub(int2fx(map->textureDepth), bg_aff_ptr->dy);
	}

	/* offset down for bg2 */
	if (map->bgcnt & BG_PRIO(1)) {
		bg_aff_ptr->dy += int2fx(map->textureDepth);
	}
}

IWRAM_CODE static void
compute_windows(const M7Map& map, const Ray& rin, const Ray::Casted& rout, const FixedPixel& lambda, int h) {
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