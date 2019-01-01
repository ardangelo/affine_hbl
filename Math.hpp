#pragma once

#include <cnl/fixed_point.h>

template <size_t FractDigits>
using FPi32 = cnl::fixed_point<std::int32_t, -FractDigits>;

/* wrapped tonc functions */

template <size_t Frac, typename T, typename U>
auto static inline Div(T const& num, U const& denom) {
	return cnl::from_rep<FPi32<Frac>, std::int32_t>{}(DivSafe(
		cnl::to_rep<T>{}(num),
		cnl::to_rep<U>{}(denom)));
}

auto static inline luCos(uint32_t const theta) {
	return cnl::from_rep<FPi32<12>, std::int32_t>{}(lu_cos(theta & 0xFFFF));
}
auto static inline luSin(uint32_t const theta) {
	return cnl::from_rep<FPi32<12>, std::int32_t>{}(lu_sin(theta & 0xFFFF));
}

using fp0  = FPi32<0>;
using fp4  = FPi32<4>;
using fp8  = FPi32<8>;
using fp12 = FPi32<12>;
using fp16 = FPi32<16>;
using fp20 = FPi32<20>;

/* linear algebra */

template <size_t N>
struct Vector {
	FPi32<N> x, y, z;

	template <size_t M>
	auto inline operator+ (Vector<M> const& rhs) const {
		return Vector<-decltype(x + rhs.x)::exponent>
			{ .x = x + rhs.x
			, .y = y + rhs.y
			, .z = z + rhs.z
			};
	}

	template <size_t M>
	auto inline operator+= (Vector<M> const& rhs) {
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
	}

	template <size_t M>
	auto inline operator- (Vector<M> const& rhs) const {
		return Vector<-decltype(x - rhs.x)::exponent>
			{ .x = x - rhs.x
			, .y = y - rhs.y
			, .z = z - rhs.z
			};
	}

	template <size_t M>
	auto inline operator* (FPi32<M> const& n) const {
		return Vector<-decltype(n * x)::exponent>
			{ .x = n * x
			, .y = n * y
			, .z = n * z
			};
	}

	template <size_t M>
	inline operator Vector<M> () const {
		return Vector<M>
			{ .x = FPi32<M>{x}
			, .y = FPi32<M>{y}
			, .z = FPi32<M>{z}
			};
	}
};

using  v0 = Vector<0>;
using  v8 = Vector<8>;
using v12 = Vector<12>;

template <size_t N>
struct Matrix {
	FPi32<N> a, b, c
	       , d, e, f
	       , g, h, i;

	template <size_t M>
	auto inline operator+ (Matrix<M> const& rhs) const {
		return Matrix<-decltype(a + rhs.a)::exponent>
			{ .a = a + rhs.a
			, .b = b + rhs.b
			, .c = c + rhs.c
			, .d = d + rhs.d
			, .e = e + rhs.e
			, .f = f + rhs.f
			, .g = g + rhs.g
			, .h = h + rhs.h
			, .i = i + rhs.i
			};
	}

	template <size_t M>
	auto inline operator* (Matrix<M> const& rhs) const {
		return Matrix<-decltype(a * rhs.a)::exponent>
			{ .a = a * rhs.a + b * rhs.d + c * rhs.g
			, .b = a * rhs.b + b * rhs.e + c * rhs.h
			, .c = a * rhs.c + b * rhs.f + c * rhs.i
			, .d = d * rhs.a + e * rhs.d + f * rhs.g
			, .e = d * rhs.b + e * rhs.e + f * rhs.h
			, .f = d * rhs.c + e * rhs.g + f * rhs.i
			, .g = g * rhs.a + h * rhs.d + i * rhs.g
			, .h = g * rhs.b + h * rhs.e + i * rhs.h
			, .i = g * rhs.c + h * rhs.f + i * rhs.i
			};
	}

	template <size_t M>
	auto inline operator* (Vector<M> const& rhs) const {
		return Vector<-decltype(a * rhs.x)::exponent>
			{ .x = a * rhs.x + b * rhs.y + c * rhs.z
			, .y = d * rhs.x + e * rhs.y + f * rhs.z
			, .z = g * rhs.x + h * rhs.y + i * rhs.z
			};
	}

	template <size_t M>
	inline operator Matrix<M> () const {
		return Matrix<M>
			{ .a = FPi32<M>{a}
			, .b = FPi32<M>{b}
			, .c = FPi32<M>{c}
			, .d = FPi32<M>{d}
			, .e = FPi32<M>{e}
			, .f = FPi32<M>{f}
			, .g = FPi32<M>{g}
			, .h = FPi32<M>{h}
			, .i = FPi32<M>{i}
			};
	}
};

template <size_t N>
auto inline make_basis(Vector<N> const& u, Vector<N> const& v, Vector<N> const& w) {
	return Matrix<N>
		{ .a = u.x, .b = v.x, .c = w.x
		, .d = u.y, .e = v.y, .f = w.y
		, .g = u.z, .h = v.z, .i = w.z
		};
}

template <size_t N = 12>
auto static inline make_rot(uint32_t const theta, uint32_t const phi) {
	auto const sin_theta = luSin(theta);
	auto const cos_theta = luCos(theta);
	#if 0
	auto const sin_phi   = luSin(phi);
	auto const cos_phi   = luCos(phi);
	return Matrix<N>
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
	return Matrix<N>
		{ .a =  1
		, .b =  0
		, .c =  0
		, .d =  0
		, .e =  cos_theta
		, .f = -sin_theta
		, .g =  0
		, .h =  sin_theta
		, .i =  cos_theta
		};
	#endif
}

template <size_t N>
struct AffineSpace {
	Matrix<N> basis    = {1, 0, 0, 0, 1, 0, 0, 0, 1};
	Matrix<N> basisInv = {1, 0, 0, 0, 1, 0, 0, 0, 1};
	Vector<0> origin;

	template <size_t M>
	Vector<N> inline transform(Vector<M> const& fromPt) const {
		return basisInv * (fromPt - origin);
	}
};