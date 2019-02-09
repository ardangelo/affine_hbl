#pragma once

#include <type_traits>
#include <climits>

#include <cmath>
constexpr static auto M_PI = double{4.0 * atan(1.0)};

#include "mode7.hpp"

namespace lut {
	/* partial compression of monotonic luts:
		encode as bitset with bit 1 meaning increase value by 1 and store
		good for e.g. 0 0 0 0 1 1 1 2 2 3 4 ...
	 */
	namespace {
		constexpr auto byte2bits(std::size_t const bytes) {
			return std::size_t{(bytes + CHAR_BIT - 1) / CHAR_BIT};
		}
		template <std::size_t compr_range, std::size_t full_size, typename T>
		struct partial_compressed_lut {
			T firstElem;
			std::array<T, byte2bits(compr_range)>  compr_portion   = {0x0};
			std::array<T, full_size - compr_range> decompr_portion = {0x0};

			auto decompress() const {
				std::array<T, full_size> result = {0x0};
				auto lastElem = firstElem;
				for (auto i = std::size_t{0}; i < compr_range; i++) {
					if (compr_portion[i / CHAR_BIT] & (1 << (i % CHAR_BIT))) {
						lastElem++;
					}
					result[i] = lastElem;
				}
				for (auto i = compr_range; i < full_size; i++) {
					result[i] = decompr_portion[i - compr_range];
				}
				return result;
			}
		};
		template <std::size_t compr_range, std::size_t full_size, typename T>
		struct fully_compressed_lut {
			T firstElem;
			std::array<T, byte2bits(compr_range)>  compr_portion = {0x0};

			auto decompress() const {
				std::array<T, full_size> result = {0x0};
				auto lastElem = firstElem;
				for (auto i = std::size_t{0}; i < compr_range; i++) {
					if (compr_portion[i / CHAR_BIT] & (1 << (i % CHAR_BIT))) {
						lastElem++;
					}
					result[i] = lastElem;
				}
				return result;
			}
		};
		template <typename Array>
		constexpr auto calc_compr_range(Array const& arr) {
			auto lastElem = arr[0];
			auto result = std::size_t{0};
			for (auto i = std::size_t{0}; i < arr.size(); i++) {
				if (arr[i] - lastElem > 1) {
					return result;
				}
				lastElem = arr[i];
				result++;
			}
			return result;
		}

		// get array size from type
		template <typename Array>
		struct ArraySize {
			constexpr static auto value() {
				return Array{}.size();
			}
		};
		template <std::size_t compr_range, typename Array,
			typename std::enable_if<(compr_range == (ArraySize<Array>::value()))>::type* = nullptr>
		constexpr auto make_compressed_lut(Array const& arr) {
			using T = typename std::remove_cv<typename std::remove_reference<decltype( arr[0] )>::type>::type;
			fully_compressed_lut<compr_range, arr.size(), T> result = {};
			result.firstElem = arr[0];

			auto lastElem = arr[0];
			for (auto i = std::size_t{0}; i < compr_range; i++) {
				if (lastElem < arr[i]) {
					result.compr_portion[i / CHAR_BIT] |= (1 << (i % CHAR_BIT));
				}
				lastElem = arr[i];
			}
			return result;
		}
		template <std::size_t compr_range, typename Array,
			typename std::enable_if<(compr_range < (ArraySize<Array>::value()))>::type* = nullptr>
		constexpr auto make_compressed_lut(Array const& arr) {
			using T = typename std::remove_cv<typename std::remove_reference<decltype( arr[0] )>::type>::type;
			partial_compressed_lut<compr_range, arr.size(), T> result = {};
			result.firstElem = arr[0];

			auto lastElem = arr[0];
			for (auto i = std::size_t{0}; i < compr_range; i++) {
				if (lastElem < arr[i]) {
					result.compr_portion[i / CHAR_BIT] |= (1 << (i % CHAR_BIT));
				}
				lastElem = arr[i];
			}
			for (auto i = compr_range; i < arr.size(); i++) {
				result.decompr_portion[i - compr_range] = arr[i];
			}
			return result;
		}
	}

	using namespace M7::k;
	using namespace math;

	/* precalculate cos */
	namespace {
		constexpr auto fp_cos(BAM const angle) {
			return fp<16>{double(cos(static_cast<double>(angle) / double{PI} * M_PI))};
		}
		constexpr auto make_finecos() {
			std::array<fp<16>, PI / 2> result = {};
			for (auto i = BAM{0}; i < PI / 2; i++) {
				result[i] = fp_cos(i);
			}
			return result;
		}
		constexpr auto finecos_arr = make_finecos();
	}
	constexpr auto finecos(BAM i) {
		i &= FINEMASK;
		if ((0 <= i) && (i < PI / 2)) {
			return finecos_arr[i];
		} else if ((PI / 2 <= i) && (i < PI)) {
			return finecos_arr[PI - i];
		} else if ((i <= PI) && (i < 3 * PI / 2)) {
			return -finecos_arr[i - PI];
		} else if ((3 * PI / 2 <= i) && (i < 2 * PI)) {
			return -finecos_arr[PI - (i - PI)];
		}
	}
	constexpr auto finesin(BAM const i) {
		return finecos(i + (PI / 2));
	}

	/* precalculate tan */
	namespace {
		constexpr auto fp_tan(BAM angle) {
			angle &= FINEMASK;
			if (angle == (PI / 2) || (angle == (3 * PI / 2))) {
				return fp<16>::max();
			}
			return fp<16>{double(tan(static_cast<double>(angle) / double{PI} * M_PI))};
		}
		constexpr auto make_finetan() {
			std::array<fp<16>, PI / 2> result = {};
			for (auto i = BAM{0}; i < PI / 2; i++) {
				result[i] = fp_tan(i);
			}
			return result;
		}
		constexpr auto finetan_arr = make_finetan();
	}
	constexpr auto finetan(BAM i) {
		i &= FINEMASK;
		if ((0 <= i) && (i < PI / 2)) {
			return finetan_arr[i];
		} else if ((PI / 2 <= i) && (i < PI)) {
			return -finetan_arr[(PI / 2) - i];
		} else if ((i <= PI) && (i < 3 * PI / 2)) {
			return finetan_arr[i - PI];
		} else if ((3 * PI / 2 <= i) && (i < 2 * PI)) {
			return -finetan_arr[(PI / 2) - (i - PI)];
		}
	}

	/* precalculate view angle -> screenspace y-coord */
	namespace _viewangletoy {
		constexpr auto make_viewangletoy(fp<2> const focalLength) {
			using Rep = decltype(focalLength.to_rep());
			std::array<Rep, PI / 2> result = {};
			for (auto i = BAM{0}; i < PI / 2; i++) {
				auto const tani = finetan(i);
				if (tani > fp<16>{fp<18>::max() / focalLength}) {
					result[i] = fp0::max().to_rep();
				} else {
					result[i] = fp0{tani * focalLength}.to_rep();
				}
			}
			return result;
		}
		constexpr auto focalLength = fp<2>{fp<18>{viewTop} / fp<16>{finetan(FOV / 2)}};
		constexpr auto viewangletoy_arr = make_viewangletoy(focalLength);

		/* stored */
		constexpr auto compr_viewangletoy_range = calc_compr_range(viewangletoy_arr);
		constexpr auto compressed_viewangletoy = make_compressed_lut<compr_viewangletoy_range>(viewangletoy_arr);
	}
	template <typename Array>
	constexpr auto viewangletoy_impl(Array const& viewangletoy_arr, BAM const i) {
		if (abs(i) > PI / 2) {
			return fp0::max();
		}
		if (i < 0) {
			return fp0{viewangletoy_arr[-i]} + viewTop;
		} else {
			return -fp0{viewangletoy_arr[i]} + viewTop;
		}
	}
	auto const static decompressed_viewangletoy_arr = _viewangletoy::compressed_viewangletoy.decompress();
	auto viewangletoy(BAM const i) {
		return viewangletoy_impl(decompressed_viewangletoy_arr, i);
	}

	/* precalculate screenspace y-coord -> view angle */
	namespace {
		template <int viewTop>
		constexpr auto make_ytoviewangle() {
			std::array<BAM, viewTop> result = {};
			for (auto h = int{0}; h < viewTop; h++) {
				auto i = BAM{0};
				while (viewangletoy_impl(_viewangletoy::viewangletoy_arr, i) > fp0{h}) {
					i++;
				}
				result[h] = i;
			}
			return result;
		}
		constexpr auto ytoviewangle_arr = make_ytoviewangle<viewTop>();
	}
	constexpr auto ytoviewangle(fp0 const h) {
		if (h < viewTop) {
			return ytoviewangle_arr[h.to_rep()];
		} else {
			return -ytoviewangle_arr[abs(((fp0{2} * fp0{viewTop}) - h).to_rep())];
		}
	}

	/* precalculate arctan */
	namespace _tantoangle {
		constexpr auto fp_atan(Slope const m) {
			return ((fp<13>{double{atan(double{m}) / M_PI}}.to_rep() * /*.13*/PI) >> 13);
		}
		constexpr auto tantoangle_size = SLOPERANGE + 1;
		constexpr auto make_tantoangle() {
			std::array<BAM, tantoangle_size> result = {}; // add extra for ratio 1
			for (auto m = Slope{0}; m <= Slope{1}; m += Slope::unit()) {
				result[m.to_rep()] = fp_atan(m);
			}
			return result;
		}
		constexpr auto tantoangle_arr = make_tantoangle();

		/* stored */
		constexpr auto compr_tantoangle_range = calc_compr_range(tantoangle_arr);
		constexpr auto compressed_tantoangle = make_compressed_lut<compr_tantoangle_range>(tantoangle_arr);
	}
	auto const static decompressed_tantoangle_arr = _tantoangle::compressed_tantoangle.decompress();
	auto tantoangle(Slope const m) {
		return decompressed_tantoangle_arr[m.to_rep()];
	}

	/* precalculate camspace point -> view angle */
	namespace {
		constexpr auto slope_div(fp0 const num, fp0 const denom) {
			if (denom.to_rep()) {
				return Slope{num} / denom;
			} else {
				return Slope::max();
			}
		}
	}
	auto pointtoangle(fp0 const x, fp0 const y) {
		if (!x.to_rep() && !y.to_rep()) {
			return BAM{0};
		}
		#if 0
		if ((0 <= x.to_rep()) && (0 <= y.to_rep())) {
			if (x > y) { // 0*pi/4 <= angle < 1*pi/4
				return (0 * PI / 2) + tantoangle(slope_div(y, x));
			} else {      // 1*pi/4 <= angle < 2*pi/4
				return (1 * PI / 2) - tantoangle(slope_div(x, y));
			}
		} else if ((0 > x.to_rep()) && (0 <= y.to_rep())) {
			auto const abs_x = -x;
			if (abs_x < y) {// 2*pi/4 <= angle < 3*pi/4
				return (1 * PI / 2) + tantoangle(slope_div(abs_x, y));
			} else {      // 3*pi/4 <= angle < 4*pi/4
				return (2 * PI / 2) - tantoangle(slope_div(y, abs_x));
			}
		} else if ((0 > x.to_rep()) && (0 > y.to_rep())) {
			auto const abs_x = -x;
			auto const abs_y = -y;
			if (abs_x > abs_y) { // 4*pi/4 <= angle < 5*pi/4
				return (2 * PI / 2) + tantoangle(slope_div(abs_y, abs_x));
			} else {      // 5*pi/4 <= angle < 6*pi/4
				return (3 * PI / 2) - tantoangle(slope_div(abs_x, abs_y));
			}
		} else {
			auto const abs_y = -y;
			if (x < abs_y) { // 6*pi/4 <= angle < 7*pi/4
				return (3 * PI / 2) + tantoangle(slope_div(x, abs_y));
			} else {      // 7*pi/4 <= angle < 8*pi/4
				return (4 * PI / 2) - tantoangle(slope_div(abs_y, x));
			}
		}
		#else
		auto abs_x = fp0{};
		auto abs_y = fp0{};
		auto quadrant_offs = BAM{};
		auto first_half_of_quad = bool{};
		if ((0 <= x.to_rep()) && (0 <= y.to_rep())) {
			abs_x =  x; abs_y =  y;
			quadrant_offs = 0 * PI / 2;
			first_half_of_quad = abs_y < abs_x;
		} else if ((0 > x.to_rep()) && (0 <= y.to_rep())) {
			abs_x = -x; abs_y =  y;
			quadrant_offs = 1 * PI / 2;
			first_half_of_quad = abs_y > abs_x;
		} else if ((0 > x.to_rep()) && (0 > y.to_rep())) {
			abs_x = -x; abs_y = -y;
			quadrant_offs = 2 * PI / 2;
			first_half_of_quad = abs_y < abs_x;
		} else {
			abs_x =  x; abs_y =  -y;
			quadrant_offs = 3 * PI / 2;
			first_half_of_quad = abs_y > abs_x;
		}
		auto const slope = (abs_x < abs_y) ? slope_div(abs_x, abs_y) : slope_div(abs_y, abs_x);
		if (first_half_of_quad) {
			return          quadrant_offs + tantoangle(slope);
		} else {
			return PI / 2 + quadrant_offs - tantoangle(slope);
		}

		#endif
	}
}