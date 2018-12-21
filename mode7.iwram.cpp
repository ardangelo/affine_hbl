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

using fp0  = FPi32<0>;
using fp4  = FPi32<4>;
using fp8  = FPi32<8>;
using fp12 = FPi32<12>;
using fp16 = FPi32<16>;

using v0 = Vector<0>;
using v8 = Vector<8>;

// texture flat like Mode 7
auto const static t_ul_world = v0{0, 0, 0};
auto const static t_ur_world = v0{M7::k::screenWidth, 0, 0};
auto const static t_bl_world = v0{0, 0, M7::k::screenHeight};
auto const static t_br_world = v0{M7::k::screenWidth, 0, M7::k::screenHeight};

IWRAM_CODE void
M7::Level::prepAffines() {

	/* world -> cam space */
	auto const camSpace_world = AffineSpace<12>
		{ .basis    = make_rot( cam.theta, cam.phi)
		, .basisInv = make_rot(-cam.theta, cam.phi)
		, .origin   = cam.pos
		};

	/* find texture reference points in cam space */
	t_ul_cam = camSpace_world.transform(t_ul_world);
	t_ur_cam = camSpace_world.transform(t_ur_world);
	t_bl_cam = camSpace_world.transform(t_bl_world);
	t_br_cam = camSpace_world.transform(t_br_world);

	/* cam -> screen space */
	auto const screenSpace_cam = AffineSpace<4>
		{ .basis    = {1, 0, 0, 0, -1, 0, 0, 0, 1}
		, .basisInv = {1, 0, 0, 0, -1, 0, 0, 0, 1}
		, .origin  = {-M7::k::screenWidth / 2, M7::k::screenHeight / 2, -M7::k::focalLength}
		};

	/* find where reference points in cam space intersect screen plane */
	auto screenIntersect = [](v8 const& vec_cam) {
		if (fp8{vec_cam.z} == 0) {
			return v8{0xFFFFFFFF, 0xFFFFFFFF, fp8{-M7::k::focalLength}};
		}
		return v8
			{ .x = fp16{vec_cam.x} / fp8{vec_cam.z} * fp0{-M7::k::focalLength}
			, .y = fp16{vec_cam.y} / fp8{vec_cam.z} * fp0{-M7::k::focalLength}
			, .z = fp8{-M7::k::focalLength}
			};
	};

	t_ul_screen = screenSpace_cam.transform(screenIntersect(t_ul_cam));
	t_ur_screen = screenSpace_cam.transform(screenIntersect(t_ur_cam));
	t_bl_screen = screenSpace_cam.transform(screenIntersect(t_bl_cam));
	t_br_screen = screenSpace_cam.transform(screenIntersect(t_br_cam));

	/* - cleanup needed below - */

	auto const ultx = fp8{t_ul_world.x}; auto const ulty = fp8{t_ul_world.y};
	auto const urtx = fp8{t_ur_world.x}; auto const urty = fp8{t_ur_world.y};
	auto const bltx = fp8{t_bl_world.x}; auto const blty = fp8{t_bl_world.y};
	auto const brtx = fp8{t_br_world.x}; auto const brty = fp8{t_br_world.y};

	auto const ulsx = fp8{t_ul_screen.x}; auto const ulsy = fp8{t_ul_screen.y};
	auto const ursx = fp8{t_ur_screen.x}; auto const ursy = fp8{t_ur_screen.y};
	auto const blsx = fp8{t_bl_screen.x}; auto const blsy = fp8{t_bl_screen.y};
	auto const brsx = fp8{t_br_screen.x}; auto const brsy = fp8{t_br_screen.y};

	/* texture starting and ending scanline */
	auto const hu   = fp0{ulsy};
	auto const hb   = fp0{blsy};
	auto const dh_inv = [&]() {
		if (auto const dh = fp8{blsy - ulsy}) {
			return fp16{1} / dh;
		}
		return fp8{0xFFFFFFFF};
	}();

	/* upper scanline texture midpoint */
	auto const tmxu = fp8{urtx + ultx} / 2;
	auto const tmyu = fp8{urty + ulty} / 2;

	/* upper scanline screenspace midpoint */
	auto const smxu = fp8{ursx + ulsx} / 2;
	auto const smyu = fp8{ursy + ulsy} / 2;

	/* lower scanline texture midpoint */
	auto const tmxb = fp8{brtx + bltx} / 2;
	auto const tmyb = fp8{brty + blty} / 2;

	/* lower scanline screenspace midpoint */
	auto const smxb = fp8{brsx + blsx} / 2;
	auto const smyb = fp8{brsy + blsy} / 2;

	/* upper and lower scanline texture width */
	auto const twu    = urtx - tmxu;
	auto const twb    = brtx - tmxb;

	/* upper and lower scanline projected width */
	auto const swu    = ursx - smxu;
	auto const swb    = brsx - smxb;

	/* texture and screenspace midpoints are linear... */
	auto const dtmx_dh = fp4{tmxb - tmxu} * dh_inv;
	auto const dtmy_dh = fp4{tmyb - tmyu} * dh_inv;
	auto const dsmx_dh = fp4{smxb - smxu} * dh_inv;
	auto const dsmy_dh = fp4{smyb - smyu} * dh_inv;

	/* texture & projected widths also linear... */
	auto const dtw_dh = fp4{twb - twu} * dh_inv;
	auto const dsw_dh = fp4{swb - swu} * dh_inv;

	/* so we can calculate midpoints and widths for each scanline using
	   top and bottom scanline information instead of transforming every edge world point */
	auto tmx = [&](fp0 const& h) {
		return dtmx_dh * (h - hu) + tmxu;
	};
	auto smx = [&](fp0 const& h) {
		return dsmx_dh * (h - hu) + smxu;
	};
	auto tw = [&](fp0 const& h) {
		return dtw_dh * (h - hu) + twu;
	};
	auto sw = [&](fp0 const& h) {
		return dsw_dh * (h - hu) + swu;
	};
	auto tmy = [&](fp0 const& h) {
		return dtmy_dh * (h - hu) + tmyu;
	};
	auto smy = [&](fp0 const& h) {
		return dsmy_dh * (h - hu) + smyu;
	};

	/* lam = t_width / s_width */
	auto lam = [&](fp0 const& h) {
		if (auto const swh = fp4{sw(h)}) {
			return fp16{tw(h)} / swh;
		}
		return fp12{0xFFFFFFFF};
	};

	for (size_t h = 0; h < k::screenHeight; h++) {

		/* P = { lam, 0, 0, lam } */
		layer.bgaff[h].pa = lam(h);
		layer.bgaff[h].pb = 0;
		layer.bgaff[h].pc = 0;
		layer.bgaff[h].pd = lam(h);

		/* dx = t_mid - lam * s_mid; */
		layer.bgaff[h].dx = tmx(h) - fp8{lam(h) * smx(h)};

		/* Tonc: "When writing to REG_BGxY inside an HBlank interrupt: the current scanline is
		   perceived as the screen's origin null-line." Figure that out for this */
		layer.bgaff[h].dy = tmy(h) - fp8{lam(h) * smy(h)} + fp8{h};
	}
}
