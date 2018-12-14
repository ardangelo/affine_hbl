#include <tonc.h>

#include "Reg.hpp"
Reg volatile reg;

#include "mode7.h"

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
	u32 ey = cnl::to_rep<decltype(bga.pb)>{}(bga.pb >> 7);
	if (ey > 16) { ey = 16; }
	REG_BLDY = BLDY_BUILD(ey);

	REG_WIN0H = layer.winh[vc + 1];
}

IWRAM_CODE void
M7::Level::prepAffines() {

	// world -> cam space
	AffineSpace camSpace_world = 
		{ .basis    = make_rot(cam.theta)
		, .basisInv = make_rot(-cam.theta)
		, .origin   = Point<0>{cam.pos.x, cam.pos.z}
		};

	// transform world -> cam space
	auto origin_cam  = camSpace_world.transform(Point<0>{0, 0});

	// project onto screen space
	auto origin_y   = origin_cam.y  * M7::k::focalLength / origin_cam.x;

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
