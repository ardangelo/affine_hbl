#pragma once

#include <tonc.h>
#undef FIXED
#undef POINT
#undef VECTOR

#include <array>

#include "Math.hpp"
#include "Reg.hpp"

auto static inline luCos(uint32_t const theta) {
	return cnl::from_rep<FPi32<12>, std::int32_t>{}(lu_cos(theta & 0xFFFF));
}
auto static inline luSin(uint32_t const theta) {
	return cnl::from_rep<FPi32<12>, std::int32_t>{}(lu_sin(theta & 0xFFFF));
}

auto static inline make_rot(uint32_t const theta, uint32_t const phi) {
	auto const sin_phi   = luSin(phi);
	auto const sin_theta = luSin(theta);
	auto const cos_phi   = luCos(phi);
	auto const cos_theta = luCos(theta);
	return Matrix<-decltype(luCos(theta))::exponent>
	#if 0
		{ .a =  cos_phi
		, .b =  sin_phi * sin_theta
		, .c = -sin_phi * cos_theta
		, .d =  0
		, .e =  cos_theta
		, .f =  sin_theta
		, .g =  sin_phi
		, .h = -cos_phi * sin_theta
		, .i =  cos_phi * cos_theta
		};
	#else
		{ .a =  1
		, .b =  0
		, .c =  0
		, .d =  0
		, .e =  cos_theta
		, .f =  sin_theta
		, .g =  0
		, .h = -sin_theta
		, .i =  cos_theta
		};
	#endif
}

namespace M7 {

	/* mode 7 constants */
	namespace k {
		size_t static constexpr screenHeight = SCREEN_HEIGHT;
		size_t static constexpr screenWidth  = SCREEN_WIDTH;
		int static constexpr focalLength   =  160;
		int static constexpr focalShift    =  8;
		int static constexpr renormShift   =  2;
		int static constexpr viewLeft      = -120;
		int static constexpr viewRight     =  120;
		int static constexpr viewTop       =  80;
		int static constexpr viewBottom    = -80;
		int static constexpr nearPlane     =  24;
		int static constexpr objFarPlane   =  512;
		int static constexpr floorFarPlane =  768;
	}

	/* mode 7 classes */
	class Camera {
	public:
		Vector<0> pos;
		/* rotation angles */
		int theta; /* polar angle */
		int phi; /* azimuth angle */
		/* space basis */
		Vector<12> u; /* local x-axis */
		Vector<12> v; /* local y-axis */
		Vector<12> w; /* local z-axis */
		/* rendering */
		FPi32<8> fov;

		Camera(FPi32<8> const& fov);
		void translate(Vector<0> const& dPos);
		void rotate(int32_t const dTheta);
	};

	class Layer {
	public: // types
		template <typename T>
		using ScreenArray = std::array<T, k::screenHeight + 1>;

	public: // variables
		ScreenArray<u16> winh; /* window 0 widths */
		ScreenArray<affine::ParamSet> bgaff; /* affine parameter array */
		u16 bgcnt; /* BGxCNT for floor */

		Layer(
			size_t cbb, const unsigned int tiles[],
			size_t sbb, const unsigned short map[],
			size_t mapSize, size_t prio);
	};

	class Level {
	public:
		Camera const& cam;
		Layer& layer;

		Level(Camera const& cam, Layer& layer);
		void translateLocal(Vector<0> const& dir);

		IWRAM_CODE void prepAffines();
		IWRAM_CODE void applyAffine(int vc);
	};
}

/* accessible both from main and iwram */
extern M7::Level fanLevel;
extern Reg volatile reg;
IWRAM_CODE void m7_hbl();

Vector<8> extern t_ul_cam, t_ur_cam, t_bl_cam, t_br_cam;
Vector<4> extern t_ul_screen, t_ur_screen, t_bl_screen, t_br_screen;