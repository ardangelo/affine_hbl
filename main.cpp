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

const FIXED OMEGA =  0x400;
const FIXED VEL_X =  0x1 << 8;
const FIXED VEL_Z = -0x1 << 8;
auto input_game(M7::Camera const& cam) {
	key_poll();

	VECTOR dir
		{ .x = VEL_X * key_tri_shoulder() /* strafe */
		, .y = 0
		, .z = VEL_Z * key_tri_vert() /* forwards / backwards */
	};

	/* rotate */
	FIXED theta = OMEGA * key_tri_horz();

	return std::make_pair(dir, theta);
}

struct Point {
	FIXED const x;
	FIXED const y;
	Point operator+ (Point const& rhs) const {
		return Point{x + rhs.x, y + rhs.y};
	}
	Point operator- (Point const& rhs) const {
		return Point{x - rhs.x, y - rhs.y};
	}
	Point operator>>(size_t rhs) const {
		return Point{x >> rhs, y >> rhs};
	}
	Point rot(FIXED theta) const;
};

struct Matr {
	FIXED const a;
	FIXED const b;
	FIXED const c;
	FIXED const d;

	Point operator* (Point const& rhs) const {
		return Point{
			(a * rhs.x + b * rhs.y),
			(c * rhs.x + d * rhs.y)
		};
	}
};

Point Point::rot(FIXED theta) const {
	return Matr{
		lu_cos(theta), -lu_sin(theta),
		lu_sin(theta),  lu_cos(theta)
	} * (*this) >> 12;
}

struct Quad {
	Point const o;
	Point const x;
};

FIXED const static SCALE = 0x3;
void inline static plot(Point const& p, COLOR const color) {
	m3_plot(p.x >> SCALE, p.y >> SCALE, color);
}

void inline static line(Point const& from, Point const& to, COLOR const color) {
	m3_line(from.x >> SCALE, from.y >> SCALE, to.x >> SCALE, to.y >> SCALE, color);
}

Point constexpr static origin {SCREEN_WIDTH / 2 << SCALE, SCREEN_HEIGHT / 2 << SCALE};
Quad  constexpr static wall1 {Point{50, 50}, Point{50,-50}};

void render_world() {
	// Fill screen with grey color
	m3_fill(RGB15(12, 12, 14));

	plot(origin, CLR_WHITE);

	auto c_o = origin - Point{fx2int(cam.pos.x), fx2int(cam.pos.z)};
	plot(c_o, CLR_BLUE);

	auto c_d = c_o - Point{M7::k::focalLength, 0}.rot(cam.theta);
	line(c_o, c_d, CLR_BLUE);

	auto w_o = origin - wall1.o;
	auto w_x = w_o - wall1.x;
	line(w_o, w_x, CLR_RED);
	plot(w_o, CLR_YELLOW);
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
		fanLevel.translateLocal(dir);

		/* update affine matrices */
		fanLevel.prepAffines();
		#else
		render_world();
		#endif
	}

	return 0;

}
