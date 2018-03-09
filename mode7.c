#include <limits.h>
#include <tonc.h>

#include "mode7.h"

INLINE int m7_horizon_line(const m7_level_t *level) {
	return clamp(level->horizon, 0, 228);
}

void m7_init(m7_level_t *level, m7_cam_t *cam, BG_AFFINE bgaff[], u16 skycnt, u16 floorcnt) {
	level->camera = cam;
	level->bgaff = bgaff;
	level->bgcnt_sky = skycnt;
	level->bgcnt_floor = floorcnt;
	level->horizon = 80;

	REG_BG2CNT = floorcnt;
	REG_BG_AFFINE[2] = bg_aff_default;
}


void m7_prep_horizon(m7_level_t *level) {
	m7_cam_t *cam = level->camera;

	int horz = 0;
	if (cam->v.y != 0) {
		horz = (M7_FAR_BG * cam->w.y) - cam->pos.y;
		horz = M7_TOP - Div(horz * (M7_D + m7_level.camera->focal_offs), M7_FAR_BG * cam->v.y);
		/* horz = M7_TOP - Div(cam->w.y*M7_D, cam->v.y); */
	} else {
		/* looking straight down */
		horz = (cam->w.y > 0) ? INT_MIN : INT_MAX;
	}
	
	level->horizon = horz;
}

void m7_rotate(m7_cam_t *cam, int phi, int theta) {
	theta = clamp(theta, -0x3FFF, 0x4001);
	cam->phi = phi;
	cam->theta = theta;

	FIXED cf, sf, ct, st;
	cf = lu_cos(phi) >> 4;
	sf = lu_sin(phi) >> 4;
	ct = lu_cos(theta) >> 4;
	st = lu_sin(theta) >> 4;

	/* camera x-axis */
	cam->u.x = cf;
	cam->u.y = 0;
	cam->u.z = sf;

	/* camera y-axis */
	cam->v.x = (sf * st) >> 8;
	cam->v.y = ct;
	cam->v.z = (-cf * st) >> 8;

	/* camera z-axis */
	cam->w.x = (-sf * ct) >> 8;
	cam->w.y = st;
	cam->w.z = (cf * ct) >> 8;
}

void m7_translate(m7_cam_t *cam, const VECTOR *dir) {
	cam->pos.x += ((cam->u.x * dir->x) - (cam->u.z * dir->z)) >> 8;
	cam->pos.y += dir->y;
	cam->pos.z += ((cam->u.z * dir->x) + (cam->u.x * dir->z)) >> 8;
}

void m7_update_sky(const m7_level_t *level) {
	REG_BG2HOFS = (level->camera->phi >> 6) + M7_LEFT;
	REG_BG2VOFS = (-m7_horizon_line(level)) - 1;
}
