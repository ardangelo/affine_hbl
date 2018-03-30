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

	for(int h = start_line; h < SCREEN_HEIGHT; h++) {
		bg_aff_ptr->pa = 1 << 8; // 8f
		bg_aff_ptr->pd = 1 << 8; // 8f

		bg_aff_ptr->dx = 0 << 8; // 8f
		bg_aff_ptr->dy = h << 8; // 8f

		bg_aff_ptr++;
	}
	level->bgaff[SCREEN_HEIGHT] = level->bgaff[0];
}
