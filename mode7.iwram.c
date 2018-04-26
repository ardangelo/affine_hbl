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
		FIXED yb = (h - M7_TOP) * cos_theta + M7_D * sin_theta; // 12f
		FIXED lambda = DivSafe(a_y << 12, yb); // 12f

		FIXED lambda_cos_phi = (lambda * cos_phi) >> 8; // 12f
		FIXED lambda_sin_phi = (lambda * sin_phi) >> 8; // 12f

		/* build affine matrices */
		bg_aff_ptr->pa = lambda_cos_phi >> 4; // 8f
		bg_aff_ptr->pc = lambda_sin_phi >> 4; // 8f

		FIXED zb = (h - M7_TOP) * sin_theta - M7_D * cos_theta; // 12f
		bg_aff_ptr->dx = a_x + (lambda_cos_phi >> 4) * M7_LEFT - ((lambda_sin_phi * zb) >> 12); // 8f
		bg_aff_ptr->dy = a_z + (lambda_sin_phi >> 4) * M7_LEFT + ((lambda_cos_phi * zb) >> 12); // 8f

		bg_aff_ptr++;
	}
	level->bgaff[SCREEN_HEIGHT] = level->bgaff[0];
}
