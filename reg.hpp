#pragma once

#include <memory>
#include <type_traits>

#include <tonc.h>

#include "math.hpp"

namespace affine {
	using PParam = math::fixed_point<8, std::int16_t>;
	using DParam = math::fixed_point<8, std::int32_t>;

	struct ParamSet {
		PParam pa, pb, pc, pd;
		DParam dx, dy;
	};
}

struct Reg {
	affine::ParamSet *bgAffine2;

	Reg()
		: bgAffine2{new (&REG_BG_AFFINE[2]) affine::ParamSet} {}
};
