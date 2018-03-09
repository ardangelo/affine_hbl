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

	/* from tonc:
	   A distance fogging with high marks for hack-value
	   Remember that we used pb to store the scale in, 
	   so the blend is basically lambda/64  = distance * 2
	*/
	u32 ey = (bga->pb * 6) >> 12;
	if (ey > 16) {
		ey = 16;
	}

	REG_BLDALPHA = BLDA_BUILD(16 - ey, ey);
}

IWRAM_CODE void m7_prep_affines(m7_level_t *level) {
	if (level->horizon >= SCREEN_HEIGHT) {
		return;
	}

	int start_line = (level->horizon >= 0) ? level->horizon : 0;

	m7_cam_t *cam = level->camera;
	FIXED a_x = cam->pos.x;
	FIXED a_y = cam->pos.y;
	FIXED a_z = cam->pos.z;

	BG_AFFINE *bg_aff_ptr = &level->bgaff[start_line];

	/* sines and cosines */
	FIXED cf = cam->u.x;
	FIXED sf = cam->u.z;
	FIXED ct = cam->v.y;
	FIXED st = cam->w.y;

	/* b'  = Rx(theta) *  (L, ys, -D) */
	/* scale and scaled (co)sine(phi) */
	for(int i = start_line; i < SCREEN_HEIGHT; i++) {
		FIXED yb = ((i - M7_TOP) * ct) + ((m7_level.camera->focal_offs + M7_D) * st);
		FIXED lambda = DivSafe(a_y << 12, yb);

		FIXED lcf = (lambda * cf) >> 8;
		FIXED lsf = (lambda * sf) >> 8;

		bg_aff_ptr->pa = lcf >> 4;
		bg_aff_ptr->pc = lsf >> 4;

		/* lambda·Rx·b */
		FIXED zb = ((i - M7_TOP) * st) - ((m7_level.camera->focal_offs + M7_D) * ct);
		bg_aff_ptr->dx = a_x + (lcf >> 4) * M7_LEFT - ((lsf * zb) >> 12);
		bg_aff_ptr->dy = a_z + (lsf >> 4) * M7_LEFT + ((lcf * zb) >> 12);

		/* from tonc:
		   hack that I need for fog. pb and pd are unused anyway
		   */
		bg_aff_ptr->pb = lambda;
		bg_aff_ptr++;
	}
	level->bgaff[SCREEN_HEIGHT] = level->bgaff[0];
}
