#pragma once

#include <cstdint>
#include <type_traits>
#include <limits>

#include <array>

namespace math {

namespace {
	struct FromRaw {};
}

template <std::size_t N, typename Rep>
struct fixed_point {
	Rep x;

	constexpr explicit fixed_point(Rep const x_, FromRaw) : x{x_} {
		static_assert(N <= std::numeric_limits<Rep>::digits, "too many fractional bits for representation type");
	}

	template<class Num, typename = typename std::enable_if<std::is_arithmetic<Num>::value>::type>
	constexpr fixed_point(Num const n_) : fixed_point{static_cast<Rep>(n_ * (1 << N)), FromRaw{}} {}

	constexpr fixed_point() : fixed_point{Rep{}} {}

	static constexpr auto max() {
		return fixed_point{std::numeric_limits<Rep>::max(), FromRaw{}};
	}
	static constexpr auto unit() {
		return fixed_point{0x1, FromRaw{}};
	}

	constexpr auto to_rep() const {
		return x;
	}
	constexpr explicit operator Rep() const {
		return to_rep();
	}

	constexpr explicit operator double() const {
		return static_cast<double>(x) / (1 << N);
	}

	template <std::size_t M, typename RepP>
	constexpr explicit operator fixed_point<M, RepP>() const {
		if constexpr (M > N) {
			return fixed_point<M, RepP>{static_cast<RepP>(x << (M - N)), FromRaw{}};
		} else {
			return fixed_point<M, RepP>{static_cast<RepP>(x >> (N - M)), FromRaw{}};
		}
	}

	constexpr bool operator< (fixed_point const rhs) const {
		return x < rhs.x;
	}

	constexpr bool operator> (fixed_point const rhs) const {
		return x > rhs.x;
	}

	constexpr bool operator== (fixed_point const rhs) const {
		return x == rhs.x;
	}

	constexpr bool operator<= (fixed_point const rhs) const {
		return (x < rhs.x) || (x == rhs.x);
	}

	template <std::size_t M, typename RepP>
	constexpr auto& operator= (fixed_point<M, RepP> const rhs) {
		x = fixed_point{rhs}.x;
		return *this;
	}

	constexpr auto operator- () const {
		return fixed_point{-x, FromRaw{}};
	}

	constexpr auto operator+ (fixed_point const rhs) const {
		return fixed_point{x + rhs.x, FromRaw{}};
	}

	constexpr auto& operator+= (fixed_point const rhs) {
		x += rhs.x;
		return *this;
	}

	constexpr auto& operator-= (fixed_point const rhs) {
		x -= rhs.x;
		return *this;
	}

	constexpr auto operator- (fixed_point const rhs) const {
		return fixed_point{x - rhs.x, FromRaw{}};
	}

	template <std::size_t M>
	constexpr auto operator* (fixed_point<M, Rep> const rhs) const {
		return fixed_point<N + M, Rep>{x * rhs.x, FromRaw{}};
	}

	template <std::size_t M>
	constexpr auto operator/ (fixed_point<M, Rep> const rhs) const {
		if (rhs.x == 0) {
			return fixed_point<N - M, Rep>::max();
		}
		return fixed_point<N - M, Rep>{x / rhs.x, FromRaw{}};
	}
};

}