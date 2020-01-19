#pragma once

#include <array>
#include <limits>

#include "math.hpp"

namespace bsp
{

using Color = uint16_t;

using coord_fp = float;
using angle_fp = float;

static constexpr auto
pointToAngleEx2(coord_fp v1_y, coord_fp v1_z, coord_fp v2_y, coord_fp v2_z)
{
	return angle_fp{};
}

struct Vertex
{
	coord_fp y, z;
};

struct Sector;

struct Sidedef
{
	Sector const& sector;

	constexpr Sidedef(Sector const& sector_)
		: sector{sector_}
	{}
};

struct Linedef
{
	Vertex const& v1;
	Vertex const& v2;
	Sidedef const* left;
	Sidedef const* right;

	constexpr Linedef(Vertex const& v1_, Vertex const& v2_,
		Sidedef const* left_, Sidedef const* right_)
		: v1{v1_}
		, v2{v2_}
		, left{left_}
		, right{right_}
	{}
};

struct Seg
{
	using id = int32_t;
	static constexpr auto maxSegs = 24;

	Vertex const& v1;
	Vertex const& v2;
	coord_fp offset;
	angle_fp angle;
	Linedef const& linedef;
	Sector const* frontSector;
	Sector const* backSector;
	angle_fp rw_angle1; // set by addline, used by render

	constexpr Seg(Vertex const& v1_, Vertex const& v2_, Linedef const& linedef_)
		: v1{v1_}
		, v2{v2_}
		, offset{}
		, angle{pointToAngleEx2(v1.y, v1.z, v2.y, v2.z)}
		, linedef{linedef_}
		, frontSector{&linedef.right->sector}
		, backSector{linedef.left
			? &linedef.left->sector
			: (Sector const*){nullptr}}
		, rw_angle1{}
	{}
};

struct Sector
{
	Color color;
	std::array<Seg::id, Seg::maxSegs> segIds;
};

// example map

static constexpr auto sectors = std::array<Sector, 3>
	{ Sector{0x7C00} // Red / left above triangle
	, Sector{0x03E0} // Blue / right below triangle
	, Sector{0x001F} // Green / right above triangle
};

static constexpr auto vertices = std::array<Vertex, 10>
	// surrounding box
	{ Vertex{-200,    0}, Vertex{  0,  200}, Vertex{200,    0}, Vertex{0, -200}
	, Vertex{  40, -140}, Vertex{-40, -140}, Vertex{  0, -180} // isoceles triangle
	, Vertex{  60, -140}, Vertex{-60, -140} // split on straight triangle base
	, Vertex{  10, -190} // split on lower triangle leg
};

static constexpr auto sidedefs = std::array<Sidedef, 16>
	{ Sidedef{sectors[0]}
	, Sidedef{sectors[0]}
	, Sidedef{sectors[0]}
	, Sidedef{sectors[0]}, Sidedef{sectors[2]}
	, Sidedef{sectors[0]}
	, Sidedef{sectors[0]}, Sidedef{sectors[1]}
	, Sidedef{sectors[0]}

	, Sidedef{sectors[1]}
	, Sidedef{sectors[1]}, Sidedef{sectors[2]}
	, Sidedef{sectors[1]}
	, Sidedef{sectors[1]}

	, Sidedef{sectors[2]}
	, Sidedef{sectors[2]}
};

static constexpr auto linedefs = std::array<Linedef, 13>
	{ Linedef{vertices[0], vertices[1], &sidedefs[0], nullptr}
	, Linedef{vertices[1], vertices[2], &sidedefs[1], nullptr}
	, Linedef{vertices[2], vertices[7], &sidedefs[2], nullptr}
	, Linedef{vertices[7], vertices[4], &sidedefs[3], &sidedefs[4]}
	, Linedef{vertices[4], vertices[5], &sidedefs[5], nullptr} // split 0
	, Linedef{vertices[5], vertices[8], &sidedefs[6], &sidedefs[7]}
	, Linedef{vertices[8], vertices[0], &sidedefs[8], nullptr}

	// 5 -> 8
	, Linedef{vertices[5], vertices[6], &sidedefs[ 9], nullptr} // split 1
	, Linedef{vertices[6], vertices[9], &sidedefs[10], &sidedefs[11]}
	, Linedef{vertices[9], vertices[3], &sidedefs[12], nullptr}
	, Linedef{vertices[3], vertices[8], &sidedefs[13], nullptr}

	, Linedef{vertices[6], vertices[4], &sidedefs[14], nullptr}
	// 4 -> 7
	, Linedef{vertices[7], vertices[9], &sidedefs[15], nullptr}
	// 9 -> 6
};

} // namespace bsp
