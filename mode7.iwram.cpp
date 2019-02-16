#include "lut.hpp" // precompiled
constexpr fp<2> focalLength{lut::focalLength};

#include <tonc.h>

#include "reg.hpp"
Reg volatile reg;

/* M7Level affine / window calculation implementations */

IWRAM_CODE
void m7_hbl() {
	fanLevel.applyAffine(REG_VCOUNT);
}

IWRAM_CODE void
M7::Level::applyAffine(int const vc) {
	/* apply floor (primary) affine */
	auto const& bga = layer.bgaff[vc + 1];
	*reg.bgAffine2 = bga;

	/* apply shading */
	u32 ey = bga.pb.to_rep() >> 7;
	if (ey > 16) { ey = 16; }
	REG_BLDY = BLDY_BUILD(ey);

	REG_WIN0H = layer.winh[vc + 1];
}

IWRAM_CODE void
M7::Level::prepAffines() {
	for (int h = 0; h < k::screenHeight; h++) {
		layer.bgaff[h].pa = lut::ytoviewangle(fp0{h});
		layer.bgaff[h].pd = lut::tantoangle(k::Slope{h} / fp0{3});

		layer.bgaff[h].dx = 0;
		layer.bgaff[h].dy = h;
	}

	layer.bgaff[k::screenHeight] = layer.bgaff[0];
}
