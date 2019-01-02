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

// texture flat like Mode 7
struct Seg {
	v0 start, end;
	int theta;
};
auto constexpr static floorSeg = Seg
	{ .start = {0, 0, 0}
	, .end   = {0, 0, M7::k::screenHeight}
	, .theta = 0 // arctan((e.y - s.y) / (e.z - s.z))
};

IWRAM_CODE void
M7::Level::prepAffines() {

	// camera to screen change-of-basis (reversed Z-axis)
	auto const camBasis_screen = Matrix<0>
		{ .a =  1, .b =  0, .c =  0
		, .d =  0, .e =  1, .f =  0
		, .g =  0, .h =  0, .i = -1
		};

	/* helper to render visible parts of drawseg */
	auto renderSeg = [&](Seg const& seg) {
		/* calculate texture to screen COB */
		auto const theta_cam = cam.theta + seg.theta;
		auto const texBasis_cam = make_rot<8>(theta_cam, 0);
		auto const texBasis_screen = texBasis_cam * camBasis_screen;

		/* determine which scanline a world vector intersects with */
		auto scanlineIntersect = [&](v0 const& vec_world) {
			auto const vec_screen = (texBasis_cam * (vec_world - cam.pos));
			return int{Div<0>(
				fp8{vec_screen.y} * fp0{k::focalLength},
				fp8{vec_screen.z})
			+ k::viewTop};
		};

		/* calculate begin and end scanlines of drawseg */
		auto const h_start = scanlineIntersect(seg.start);
		auto const h_end   = scanlineIntersect(seg.end);

		/* calculate affine parameters for visible part of drawseg */
		for (int h = std::max(0, h_start); h < std::min(h_end, k::screenHeight); h++) {
			/* screen origin adjusts per scanline */
			auto const topscanOrigin_cam = v0
				{ k::viewLeft
				, h - M7::k::viewTop
				, k::focalLength
			};
			auto const b = texBasis_screen * topscanOrigin_cam;
			auto const lam = Div<12>(fp20{cam.pos.y}, b.y);

			layer.bgaff[h].pa = lam;

			layer.bgaff[h].dx = fp8{cam.pos.x} + fp8{fp8{lam} * fp0{b.x}};
			layer.bgaff[h].dy = fp8{cam.pos.z} + fp8{fp8{lam} * fp8{b.z}};
		}
	};

	renderSeg(floorSeg);

	layer.bgaff[k::screenHeight] = layer.bgaff[0];
}
