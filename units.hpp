#ifndef UNITS_HPP_
#define UNITS_HPP_

#include <optional>

#include "strongtypes.hpp"

template <typename T>
struct Point {
	T a, b;

	constexpr Point operator+(const Point& p) {
		return {a + p.a, p + p.b};
	}
};

template <typename T>
struct Vector {
	T x, y, z;

	constexpr Vector operator+(const Vector& v) {
		return {x + v.x, y + v.y, z + v.z};
	}

	constexpr Vector operator*(const Vector& v) {
		return {x * v.x, y * v.y, z * v.z};
	}
	constexpr Vector operator*(int i) {
		return {x * i, y * i, z * i};
	}
};

using FixedPixel = NamedInt<struct FixedPixelParameter>;
using FixedAngle = NamedInt<struct FixedAngleParameter>;
using Block = NamedInt<struct BlockParameter>;


constexpr Point<Block> operator Point<Block>(const Point<FixedPixel>& p) {
	return {Block(fx2int(int(p.a))), Block(fx2int(int(p.b)))};
}

#endif