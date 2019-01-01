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
auto constexpr static t_u_world = v0{0, 0, 0};
auto constexpr static t_b_world = v0{0, 0, M7::k::screenHeight};

int constexpr t_theta = 0;

IWRAM_CODE void
M7::Level::prepAffines() {

	auto const camBasis_screen = Matrix<0>
		{ .a =  1, .b =  0, .c =  0
		, .d =  0, .e =  1, .f =  0
		, .g =  0, .h =  0, .i = -1
		};
	auto const theta_cam = cam.theta + t_theta;
	auto const texBasis_cam = make_rot<8>(theta_cam, 0);
	auto const texBasis_screen = texBasis_cam * camBasis_screen;

	for (int h = 0; h < k::screenHeight; h++) {

		auto const screenOrigin_cam = v0
			{ M7::k::viewLeft
			, h - M7::k::viewTop
			, M7::k::focalLength
		};
		auto const b = texBasis_screen * screenOrigin_cam;
		auto const lam = Div<12>(fp20{cam.pos.y}, b.y);

		layer.bgaff[h].pa = lam;

		layer.bgaff[h].dx = fp8{cam.pos.x} + fp8{fp8{lam} * fp0{b.x}};
		layer.bgaff[h].dy = fp8{cam.pos.z} + fp8{fp8{lam} * fp8{b.z}};
	}
	layer.bgaff[M7::k::screenHeight] = layer.bgaff[0];
}
