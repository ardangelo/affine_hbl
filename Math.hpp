#pragma once

#include <cnl/fixed_point.h>

template <size_t FractDigits>
using FPi32 = cnl::fixed_point<std::int32_t, -FractDigits>;

template <size_t N>
struct Point {
	FPi32<N> const x;
	FPi32<N> const y;

	template <size_t M>
	auto inline operator+ (Point<M> const& rhs) const {
		return Point<-decltype(x + rhs.x)::exponent>
			{ .x = x + rhs.x
			, .y = y + rhs.y
			};
	}

	template <size_t M>
	auto inline operator- (Point<M> const& rhs) const {
		return Point<-decltype(x - rhs.x)::exponent>
			{ .x = x - rhs.x
			, .y = y - rhs.y
			};
	}

	template <size_t M>
	auto inline operator* (FPi32<M> const& n) const {
		return Point<-decltype(n * x)::exponent>
			{ .x = n * x
			, .y = n * y
			};
	}

	template <size_t M>
	inline operator Point<M> () const {
		return Point<M>
			{ .x = FPi32<M>{x}
			, .y = FPi32<M>{y}
			};
	}
};

struct Vector {
	FPi32<8> x, y, z;
};

template <size_t N>
struct Matrix {
	FPi32<N> const a;
	FPi32<N> const b;
	FPi32<N> const c;
	FPi32<N> const d;

	template <size_t M>
	auto inline operator+ (Matrix<M> const& rhs) const {
		return Matrix<-decltype(a + rhs.a)::exponent>
			{ .a = a + rhs.a
			, .b = b + rhs.b
			, .c = c + rhs.c
			, .d = d + rhs.d
			};
	}

	template <size_t M>
	auto inline operator* (Point<M> const& rhs) const {
		return Point<-decltype(a * rhs.x)::exponent>
			{ .x = a * rhs.x + b * rhs.y
			, .y = c * rhs.x + d * rhs.y
			};
	}
};