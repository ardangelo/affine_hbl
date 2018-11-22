#include <tonc.h>

#include "mode7.h"

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

/* M7Level affine / window calculation implementations */

IWRAM_CODE
void m7_hbl() {
	fanLevel.applyAffines(REG_VCOUNT);
}

inline IWRAM_CODE void
M7Level::applyAffines(int const vc) {
	/* apply floor (primary) affine */
	auto const bga = &map->bgaff[vc + 1];
	REG_BG_AFFINE[2] = *bga;

	/* apply shading */
	u32 ey = bga->pb >> 7;
	if (ey > 16) { ey = 16; }
	REG_BLDY = BLDY_BUILD(ey);

	REG_WIN0H = map->winh[vc + 1];
}

IWRAM_CODE void
M7Level::prepAffines() {
	for (int h = 0; h < SCREEN_HEIGHT; h++) {

		FIXED lambda = int2fx(1);

		/* set affine matrix for scanline */
		map->bgaff[h].pa = lambda;
		map->bgaff[h].pb = 0;
		map->bgaff[h].pc = 0;
		map->bgaff[h].pd = lambda;

		map->bgaff[h].dx = 0;
		map->bgaff[h].dy = int2fx(h);
	}
}
