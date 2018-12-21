#include <tonc.h>

#include "Reg.hpp"
Reg volatile reg;

#include "mode7.h"
Vector<8> t_ul_cam, t_ur_cam, t_bl_cam, t_br_cam;
Vector<4> t_ul_screen, t_ur_screen, t_bl_screen, t_br_screen;

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

// texture flat like Mode 7
auto const static t_ul_world = v0{0, 0, 0};
auto const static t_ur_world = v0{M7::k::screenWidth, 0, 0};
auto const static t_bl_world = v0{0, 0, M7::k::screenHeight};
auto const static t_br_world = v0{M7::k::screenWidth, 0, M7::k::screenHeight};

IWRAM_CODE void
M7::Level::prepAffines() {

	auto const cos_theta = fp8{cam.v.y};
	auto const sin_theta = fp8{cam.w.y};

	for (size_t h = 0; h < k::screenHeight; h++) {

		auto const yb = fp0{h - M7::k::viewTop} * cos_theta + fp0{M7::k::focalLength} * sin_theta;
		auto const lam = Div<12>(fp20{cam.pos.y}, yb);

		layer.bgaff[h].pa = lam;

		auto const zb = fp0{h - M7::k::viewTop} * sin_theta - fp0{M7::k::focalLength} * cos_theta;
		layer.bgaff[h].dx = fp8{cam.pos.x} + fp8{lam} * fp0{M7::k::viewLeft};
		layer.bgaff[h].dy = fp8{cam.pos.z} + fp8{fp8{lam} * zb};
	}
	layer.bgaff[M7::k::screenHeight] = layer.bgaff[0];
}
