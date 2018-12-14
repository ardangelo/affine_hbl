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

// make a ramp that always appears in same place in front of camera
Vector<0> static const t_o_cam =
	{ .x = .75 * M7::k::viewLeft
	, .y = .75 * M7::k::viewTop
	, .z = - (M7::k::focalLength + 100)
	};
Vector<0> static const t_w_cam =
	{ .x = t_o_cam.x + M7::k::viewRight
	, .y = t_o_cam.y + 0
	, .z = t_o_cam.z + 0
	};
Vector static const t_h_cam = t_o_cam + Vector<0>
	{ .x = t_o_cam.x + 0
	, .y = t_o_cam.y + -M7::k::viewTop
	, .z = t_o_cam.z + 75
	};

IWRAM_CODE void
M7::Level::prepAffines() {

	// world -> cam space
	AffineSpace camSpace_world = 
		{ .basis    = make_rot( cam.theta, cam.phi)
		, .basisInv = make_rot(-cam.theta, cam.phi)
		, .origin   = cam.pos
		};

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
