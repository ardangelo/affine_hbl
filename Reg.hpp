#pragma once

#include <memory>
#include <type_traits>

#include <tonc.h>

#include <cnl/fixed_point.h>

namespace affine {
	using PParam = cnl::fixed_point<std::int16_t, -8>;
	using DParam = cnl::fixed_point<std::int32_t, -8>;

	struct ParamSet {
		PParam pa, pb, pc, pd;
		DParam dx, dy;

		ParamSet& operator=(ParamSet const& rhs) {
			auto srcbytes = reinterpret_cast<const std::int32_t*>(&rhs.pa);
			auto dstbytes = reinterpret_cast<std::int32_t*>(&pa);
			for (size_t i = 0; i < 4; i++) {
				dstbytes[i] = srcbytes[i];
			}
			return *this;
		}
	};
}

struct Reg {
	affine::ParamSet *bgAffine2;

	Reg()
		: bgAffine2{new (&REG_BG_AFFINE[2]) affine::ParamSet} {}
};
