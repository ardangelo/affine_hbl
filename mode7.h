#pragma once

#include <tonc.h>

#include <array>

namespace M7 {

	/* mode 7 constants */
	namespace k {
		size_t static const screenHeight = SCREEN_HEIGHT;
		FIXED static const focalLength   =  160;
		FIXED static const focalShift    =  8;
		FIXED static const renormShift   =  2;
		FIXED static const viewLeft      = -120;
		FIXED static const viewRight     =  120;
		FIXED static const viewTop       =  80;
		FIXED static const viewBottom    = -80;
		FIXED static const nearPlane     =  24;
		FIXED static const objFarPlane   =  512;
		FIXED static const floorFarPlane = 768;
	}

	/* mode 7 classes */
	class Camera {
	public:
		VECTOR pos;
		/* rotation angles */
		int theta; /* polar angle */
		int phi; /* azimuth angle */
		/* space basis */
		VECTOR u; /* local x-axis */
		VECTOR v; /* local y-axis */
		VECTOR w; /* local z-axis */
		/* rendering */
		FIXED fov;

		Camera(FIXED f);
		void rotate(FIXED th);
	};

	class Layer {
	public: // types
		template <typename T>
		using ScreenArray = std::array<T, k::screenHeight + 1>;

	public: // variables
		ScreenArray<u16> winh; /* window 0 widths */
		ScreenArray<BG_AFFINE> bgaff; /* affine parameter array */
		u16 bgcnt; /* BGxCNT for floor */

		Layer(
			size_t cbb, const unsigned int tiles[],
			size_t sbb, const unsigned short map[],
			size_t mapSize, size_t prio,
			FIXED fov);
	};

	class Level {
	public:
		Camera const& cam;
		Layer& layer;

		Level(Camera const& cam, Layer& layer);
		void translateLocal(VECTOR const& dir);

		IWRAM_CODE void prepAffines();
		IWRAM_CODE void applyAffines(int vc);
	};
}

/* accessible both from main and iwram */
extern M7::Level fanLevel;
IWRAM_CODE void m7_hbl();