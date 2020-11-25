#pragma once

#include <cmath>

#include <array>
#include <limits>
#include <tuple>
#include <algorithm>

#include "math.hpp"
#include "vram.hpp"

namespace bsp
{

using Color = uint16_t;

using coord_fp = float;
using angle_fp = float;
using screen_fp = float;
using h_fp = int;
using scale_fp = float;

namespace m7
{
	static constexpr auto D = coord_fp{120};
	static constexpr auto B = coord_fp{-80};
	static constexpr auto T = coord_fp{ 80};
}

static constexpr auto
sin(angle_fp alpha)
{
	return angle_fp{(float)::sin(alpha)};
}

static constexpr auto
tan(angle_fp alpha)
{
	return angle_fp{(float)::tan(alpha)};
}

static constexpr auto
safe_atan2(coord_fp y, coord_fp z)
{
	return angle_fp{(float)::atan2(y, z)};
}

static constexpr auto pi = angle_fp{safe_atan2(0,-1)};

static constexpr auto
wrap(angle_fp a)
{
	auto const whole = int(a / (2 * pi));
	a = a - (whole * 2 * pi);
	if (a < 0) { a += 2 * pi; }
	return a;
}

static constexpr auto
recenter(angle_fp a)
{
	a = wrap(a);
	a = (a < pi) ? a : (a - (2 * pi));
	return a;
}

static constexpr auto
point_distance(coord_fp dy, coord_fp dz)
{
	auto const [du, dv] = (dz > dy) ? std::tie(dz, dy) : std::tie(dy, dz);

	auto const angle = safe_atan2(du, dv);
	auto const sin_angle = sin(angle);
	if (sin_angle) {
		return coord_fp{du / sin_angle};
	}
	return std::numeric_limits<coord_fp>::max();
}

static constexpr auto
ytoviewangle(screen_fp y)
{
	return safe_atan2(y, m7::D);
}

static constexpr auto
viewangletoy(angle_fp alpha)
{
	if (alpha < 0)  { alpha = std::max(alpha, ytoviewangle(m7::B)); }
	else if (alpha > 0) { alpha = std::min(alpha, ytoviewangle(m7::T)); }
	auto const result = coord_fp{tan(alpha) * m7::D};
	return std::clamp(result, m7::B, m7::T);
}

struct Vertex
{
	coord_fp y, z;
};

struct Sidedef;

struct Linedef
{
	using id = ssize_t;

	Vertex const& v1;
	Vertex const& v2;
	Sidedef const* front;
	Sidedef const* back;

	constexpr Linedef(Vertex const& v1_, Vertex const& v2_,
		Sidedef const* front_, Sidedef const* back_)
		: v1{v1_}
		, v2{v2_}
		, front{front_}
		, back{back_}
	{}
};

struct Sector
{
	using id = ssize_t;

	Color color;
	std::array<Linedef::id, 16> segIds;
};

struct Sidedef
{
	Sector const& sector;
	Color color;

	constexpr Sidedef(Sector const& sector_)
		: sector{sector_}
		, color{sector.color}
	{}
};

// example map

static constexpr auto sectors = std::array<Sector, 3>
	{ Sector{0x001F // Blue / right of triangle
		, 0, 1, 2, 3, 4, 5, 6, -1}
	, Sector{0x7C00 // Red / left above triangle
		, 7, 8, 9, 10, -1}
	, Sector{0x03E0 // Green / left below triangle
		, 11, 12, -1}
};

static constexpr auto vertices = std::array<Vertex, 10>
	// surrounding box
	{ Vertex{ 200,0}, Vertex{  0,  200}, Vertex{-200,0}, Vertex{0, -200}
	, Vertex{  40, -140}, Vertex{-40, -140}, Vertex{   0, -180} // isoceles triangle
	, Vertex{  60, -140}, Vertex{-60, -140} // split on straight triangle base
	, Vertex{  10, -190} // split on lower triangle leg
};

static constexpr auto sidedefs = std::array<Sidedef, 16>
	{ Sidedef{sectors[0]}
	, Sidedef{sectors[0]}
	, Sidedef{sectors[0]}
	, Sidedef{sectors[0]}
	, Sidedef{sectors[0]}
	, Sidedef{sectors[0]}
	, Sidedef{sectors[0]}

	, Sidedef{sectors[1]}
	, Sidedef{sectors[1]}
	, Sidedef{sectors[1]}
	, Sidedef{sectors[1]}
	, Sidedef{sectors[1]}

	, Sidedef{sectors[2]}
	, Sidedef{sectors[2]}
	, Sidedef{sectors[2]}
	, Sidedef{sectors[2]}
};

static constexpr auto linedefs = std::array<Linedef, 13>
	{ Linedef{vertices[0], vertices[1], &sidedefs[0], nullptr} // 0
	, Linedef{vertices[1], vertices[2], &sidedefs[1], nullptr}
	, Linedef{vertices[2], vertices[8], &sidedefs[2], nullptr}
	, Linedef{vertices[8], vertices[5], &sidedefs[3], &sidedefs[7]}
	, Linedef{vertices[5], vertices[4], &sidedefs[4], nullptr}
	, Linedef{vertices[4], vertices[7], &sidedefs[5], &sidedefs[14]}
	, Linedef{vertices[7], vertices[0], &sidedefs[6], nullptr}

	// Sec 0 -> Sec 1 via 5 -> 8
	, Linedef{vertices[8], vertices[3], &sidedefs[ 8], nullptr} // 7
	, Linedef{vertices[3], vertices[9], &sidedefs[ 9], nullptr}
	, Linedef{vertices[9], vertices[6], &sidedefs[10], &sidedefs[12]}
	, Linedef{vertices[6], vertices[5], &sidedefs[11], nullptr}

	// Sec 1 -> Sec 2 via 6 -> 9
	, Linedef{vertices[9], vertices[7], &sidedefs[13], nullptr} // 11
	, Linedef{vertices[4], vertices[6], &sidedefs[15], nullptr}
};

struct Node
{
	Vertex v1, v2;
	Sector const* frontSector;
	Sector const* backSector;
	int frontNodeIdx;
	int backNodeIdx;
};

static constexpr auto nodes = std::array<Node, 2>
	{ Node{vertices[5], vertices[4], &sectors[0], nullptr, -1,  1}
	, Node{vertices[6], vertices[5], &sectors[1], &sectors[2], -1, -1}
};

static constexpr auto
on_right_side(coord_fp y, coord_fp z, coord_fp v1_y, coord_fp v1_z, coord_fp v2_y, coord_fp v2_z)
{
	auto const t1 = y - v1_y;
	auto const t2 = z - v1_z;
	auto const d1 = y - v2_y;
	auto const d2 = z - v2_z;
	auto const right_side = (d1 * t2 - d2 * t1) < 0;
	return right_side;
}

using SortedSectors = std::array<Sector const*, sectors.size()>;

static constexpr size_t
sort_sectors_from_cursor(coord_fp y, coord_fp z,
	Node const& node, size_t cursor, SortedSectors& result)
{
	auto const on_front_side = bsp::on_right_side(y, z, node.v1.y, node.v1.z, node.v2.y, node.v2.z);
	if (on_front_side) {
		if (node.frontSector != nullptr) {
			result[cursor++] = node.frontSector;
		} else {
			cursor = sort_sectors_from_cursor(y, z,
				nodes[node.frontNodeIdx], cursor, result);
		}
		if (node.backSector != nullptr) {
			result[cursor++] = node.backSector;
		} else {
			cursor = sort_sectors_from_cursor(y, z,
				nodes[node.backNodeIdx], cursor, result);
		}
	} else {
		if (node.backSector != nullptr) {
			result[cursor++] = node.backSector;
		} else {
			cursor = sort_sectors_from_cursor(y, z,
				nodes[node.backNodeIdx], cursor, result);
		}
		if (node.frontSector != nullptr) {
			result[cursor++] = node.frontSector;
		} else {
			cursor = sort_sectors_from_cursor(y, z,
				nodes[node.frontNodeIdx], cursor, result);
		}
	}
	return cursor;
}

static constexpr auto
sort_sectors_from(coord_fp y, coord_fp z)
{
	SortedSectors result = {};
	sort_sectors_from_cursor(y, z, nodes[0], 0, result);
	return result;
}

static constexpr auto
sector_for(coord_fp y, coord_fp z)
{
	return *sort_sectors_from(y, z)[0];
}

static constexpr auto
viewangle_for(coord_fp a_y, coord_fp a_z, angle_fp phi, coord_fp y, coord_fp z)
{
	auto const angle = recenter(safe_atan2(y - a_y, z - a_z) - phi);

	return angle;
}

template <size_t N>
struct Renderer
{
using Range = std::pair<h_fp, h_fp>;

static constexpr auto h_min = std::numeric_limits<h_fp>::min();
static constexpr auto h_max = std::numeric_limits<h_fp>::max();
static constexpr auto emptyOcclusion = Range{h_min, -1};
static constexpr auto fullOcclusion  = Range{h_min, h_max};

std::array<Range, N> m_occlusions = emptyOcclusion;

struct Drawseg
{
scale_fp sc1, dsc;
coord_fp tx1, dtx;
Linedef const* seg;
};

std::array<Drawseg, N> m_drawsegs;
size_t m_next_drawseg = 0;

struct Drawrange
{
Range range;
Drawseg const* drawseg;
};
std::array<Drawrange, N> m_drawranges;
size_t m_next_drawrange = 0;

constexpr bool fullyOccluded() const
{
	return m_occlusions[0] == fullOcclusion;
}

constexpr bool rangeVisible(h_fp h1, h_fp h2) const
{
// read occlusions
	return false;
}

constexpr auto calculateDrawseg(Linedef const* seg) const
{
// run drawseg calculations
}

constexpr void occludeDrawseg(h_fp h1, h_fp h2, Drawseg const* drawseg)
{
// update occlusions
// for each hole, add new drawrange
}

constexpr auto drawranges_begin() const { return m_drawranges.begin(); }
constexpr auto drawranges_end()   const { return m_drawranges.begin() + m_next_drawrange + 1; }
};

} // namespace bsp
