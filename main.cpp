#include <limits.h>
#include <stdio.h>

#include <tonc.h>

#include <utility>

#include "mode7.h"

#include "gfx/fanroom.h"
#include "gfx/bgpal.h"

#include "maps/fanroom.h"

#ifdef DEBUG
#define DEBUGFMT(fmt, ...)
#else
#include <stdlib.h>
char *dbg_str;
#define DEBUG(str) (nocash_puts(str))
#define DEBUGFMT(fmt, ...) do {	  \
		asprintf(&dbg_str, fmt, __VA_ARGS__); \
		nocash_puts(dbg_str); \
		free(dbg_str); \
	} while (0)
#endif

/* block mappings */
#define M7_CBB 0
#define FLOOR_SBB 24
#define FLOOR_PRIO 2
#define WALL_PRIO 3

/* m7 globals */

M7::Camera cam(fxdiv(int2fx(M7::k::viewTop), int2fx(M7::k::focalLength)));
M7::Layer floorLayer(
	M7_CBB,    fanroomTiles,
	FLOOR_SBB, fanroomMap,
	BG_AFF_128x128, FLOOR_PRIO,
	cam.fov);
M7::Level fanLevel(cam, floorLayer);

/* implementations */

void init_map() {
	#if 0
	/* extract main bg */
	LZ77UnCompVram(bgPal, pal_bg_mem);

	/* setup shadow fade */
	REG_BLDCNT = BLD_BUILD(BLD_BG2, BLD_BACKDROP, 3);
	REG_WININ  = WININ_BUILD(WIN_BG2 | WIN_BLD, 0);
	REG_WIN0V  = M7::k::screenHeight;
	pal_bg_mem[0] = CLR_GRAY / 2;

	/* registers */
	REG_DISPCNT = DCNT_MODE2 | DCNT_BG2 | DCNT_OBJ | DCNT_OBJ_1D;
	#else
	REG_DISPCNT = DCNT_MODE3 | DCNT_BG2;
	#endif
}

FPi32<4> static constexpr OMEGA =   16;
FPi32<8> static constexpr VEL_X =   8;
FPi32<8> static constexpr VEL_Z =  -8;
auto input_game(M7::Camera const& cam) {
	key_poll();

	Vector dir
		{ .x = VEL_X * key_tri_shoulder() /* strafe */
		, .y = 0
		, .z = VEL_Z * key_tri_vert() /* forwards / backwards */
	};

	/* rotate */
	auto theta = OMEGA * key_tri_horz();

	return std::make_pair(dir, theta);
}

namespace draw {
	int const static SCALE = 0x3;
	void inline static plot(Point<0> const& p, COLOR const color) {
		m3_plot(int(p.x >> SCALE), int(p.y >> SCALE), color);
	}

	void inline static line(Point<0> const& from, Point<0> const& to, COLOR const color) {
		m3_line(int(from.x >> SCALE), int(from.y >> SCALE), int(to.x >> SCALE), int(to.y >> SCALE), color);
	}
}

struct Quad {
	Point<0> const o;
	Point<0> const x;
};

Point<0> constexpr static origin {SCREEN_WIDTH / 2 << draw::SCALE, SCREEN_HEIGHT / 2 << draw::SCALE};
Quad     constexpr static wall1 {{50, 50}, {50,-50}};

void render_world() {
	// Fill screen with grey color
	m3_fill(RGB15(12, 12, 14));

	// origin
	draw::plot(origin, CLR_WHITE);

	// camera point
	auto c_o = origin - Point<0>{cam.pos.x, cam.pos.z};
	draw::plot(c_o, CLR_BLUE);
	// camera fov
	auto c_d = c_o - point_rot(Point<0>{M7::k::focalLength, 0}, cam.theta);
	draw::line(c_o, c_d, CLR_BLUE);

	auto c_x = c_d - point_rot(Point<0>{0, M7::k::viewRight}, cam.theta);
	draw::line(c_d, c_x, CLR_BLUE);
	auto c_xp = c_d - point_rot(Point<0>{0, M7::k::viewLeft}, cam.theta);
	draw::line(c_d, c_xp, CLR_SKYBLUE);

	auto r   = c_o - point_rot(Point<0>{4 * M7::k::focalLength, 4 * M7::k::viewRight}, cam.theta);
	draw::line(c_o, r, CLR_BLUE);
	auto rp  = c_o - point_rot(Point<0>{4 * M7::k::focalLength, 4 * M7::k::viewLeft }, cam.theta);
	draw::line(c_o, rp, CLR_SKYBLUE);

	// wall1
	auto w_o = origin - wall1.o;
	auto w_x = w_o - wall1.x;
	draw::line(w_o, w_x, CLR_RED);
	draw::plot(w_o, CLR_YELLOW);
}

int main() {
	init_map();

	/* irqs */
	irq_init(nullptr);
	#if 0
	irq_add(II_HBLANK, (fnptr)m7_hbl);
	#endif
	irq_add(II_VBLANK, nullptr);

	while(1) {
		VBlankIntrWait();

		/* update camera based on input */
		auto [dPos, dTheta] = input_game(cam);
		cam.translate(dPos);
		cam.rotate(dTheta);
		#if 0
		fanLevel.translateLocal(dPos);

		/* update affine matrices */
		fanLevel.prepAffines();
		#else
		render_world();
		#endif
	}

	return 0;

}
