#pragma once

#include <tonc.h>
#undef FIXED
#undef POINT
#undef VECTOR

#include <array>

#include "Math.hpp"
#include "Reg.hpp"

namespace M7 {

	/* mode 7 constants */
	namespace k {
		int static constexpr screenHeight = SCREEN_HEIGHT;
		int static constexpr screenWidth  = SCREEN_WIDTH;
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
		v0 pos;
		/* rotation angles */
		int theta; /* polar angle */
		int phi; /* azimuth angle */
		/* rendering */
		fp8 fov;

		Camera(fp8 const& fov);
		void translate(v0 const& dPos);
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

		IWRAM_CODE void prepAffines();
		IWRAM_CODE void applyAffine(int vc);
	};
}

/* accessible both from main and iwram */
extern M7::Level fanLevel;
extern Reg volatile reg;
IWRAM_CODE void m7_hbl();