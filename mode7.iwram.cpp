#include <tonc.h>

#include "Reg.hpp"
Reg reg;

#include "mode7.h"

#define TRIG_ANGLE_MAX 0xFFFF

/* M7Level affine / window calculation implementations */

IWRAM_CODE
void m7_hbl() {
	fanLevel.applyAffines(REG_VCOUNT);
}

inline IWRAM_CODE void
M7::Level::applyAffines(int const vc) {
	/* apply floor (primary) affine */
	auto const& bga = layer.bgaff[vc + 1];
	*reg.bgAffine2 = bga;

	/* apply shading */
	u32 ey = cnl::to_rep<decltype(bga.pb)>{}(bga.pb >> 7);
	if (ey > 16) { ey = 16; }
	REG_BLDY = BLDY_BUILD(ey);

	REG_WIN0H = layer.winh[vc + 1];
}

IWRAM_CODE void
M7::Level::prepAffines() {
	for (size_t h = 0; h < k::screenHeight; h++) {

		/* set affine matrix for scanline */
		layer.bgaff[h].pa = 1;
		layer.bgaff[h].pb = 0;
		layer.bgaff[h].pc = 0;
		layer.bgaff[h].pd = 1;

		layer.bgaff[h].dx = 0;
		layer.bgaff[h].dy = h;
	}
}
