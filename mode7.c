#include <limits.h>
#include <tonc.h>

#include "mode7.h"

void m7_init(m7_level_t *level, m7_cam_t *cam, BG_AFFINE bgaff[], u16 winh_arr[], u16 floorcnt) {
	level->camera = cam;
	level->bgaff = bgaff;
	level->winh = winh_arr;
	level->bgcnt_floor = floorcnt;

	REG_BG2CNT = floorcnt;
	REG_BG_AFFINE[2] = bg_aff_default;
}

void m7_rotate(m7_cam_t *cam, int theta) {
	/* limited to fixpoint range */
	theta &= 0xFFFF;

	cam->theta = theta;

	FIXED ct = lu_cos(theta) >> 4;
	FIXED st = lu_sin(theta) >> 4;

	/* camera x-axis */
	cam->u.x = 1 << 8;
	cam->u.y = 0;
	cam->u.z = 0;

	/* camera y-axis */
	cam->v.x = 0;
	cam->v.y = ct;
	cam->v.z = -1 * st;

	/* camera z-axis */
	cam->w.x = 0;;
	cam->w.y = st;
	cam->w.z = ct;
}

void m7_translate_local(m7_level_t *level, const VECTOR *dir) {
	m7_cam_t *cam = level->camera;

	VECTOR pos = cam->pos;
	pos.x += (cam->u.x * dir->x + cam->v.x * dir->y + cam->w.x * dir->z) >> 8;
	pos.y += ( 0                + cam->v.y * dir->y + cam->w.y * dir->z) >> 8;
	pos.z += (cam->u.z * dir->x + cam->v.z * dir->y + cam->w.z * dir->z) >> 8;

	/* update x */
	if ((int2fx(-3) <= pos.x) && (pos.x <= int2fx(3))) {
		cam->pos.x = pos.x;
	}

	/* check y / z wall collision independently */
	if ((pos.y >= 0) && (fx2int(pos.y) < level->blocks_height)) {
		if (level->blocks[(fx2int(pos.y) * m7_level.blocks_width) + fx2int(cam->pos.z)] == 0) {
			cam->pos.y = pos.y;
		}
	}
	if ((pos.z >= 0) && (fx2int(pos.z) < level->blocks_width)) {
		if (level->blocks[(fx2int(cam->pos.y) * m7_level.blocks_width) + fx2int(pos.z)] == 0) {
			cam->pos.z = pos.z;
		}
	}
}

void m7_translate_level(m7_level_t *level, const VECTOR *dir) {
	m7_cam_t *cam = level->camera;

	cam->pos.x += ((cam->u.x * dir->x) - (cam->u.z * dir->z)) >> 8;
	cam->pos.y += dir->y;
	cam->pos.z += ((cam->u.z * dir->x) + (cam->u.x * dir->z)) >> 8;
}