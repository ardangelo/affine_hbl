#pragma once

#include <cassert>
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
cos(angle_fp alpha)
{
	return angle_fp{(float)::cos(alpha)};
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

static constexpr auto
calculateSlopeAngle(Vertex const& v1, Vertex const& v2)
{
	return angle_fp{recenter(safe_atan2(v2.y - v1.y, v2.z - v1.z))};
}

struct Linedef
{
	using id = ssize_t;

	Vertex const& v1;
	Vertex const& v2;
	angle_fp const slope;

	Sidedef const* front;
	Sidedef const* back;

	constexpr Linedef(Vertex const& v1_, Vertex const& v2_,
		Sidedef const* front_, Sidedef const* back_)
		: v1{v1_}
		, v2{v2_}
		, slope{calculateSlopeAngle(v1, v2)}

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

struct Node
{
	Vertex v1, v2;
	Sector const* frontSector;
	Sector const* backSector;
	int frontNodeIdx;
	int backNodeIdx;
};

struct ExampleMap
{

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

static constexpr auto nodes = std::array<Node, 2>
	{ Node{vertices[5], vertices[4], &sectors[0], nullptr, -1,  1}
	, Node{vertices[6], vertices[5], &sectors[1], &sectors[2], -1, -1}
};

};

static constexpr auto
on_right_side(coord_fp y, coord_fp z, coord_fp v1_y, coord_fp v1_z, coord_fp v2_y, coord_fp v2_z)
{
	auto const d_y = y - v2_y;
	auto const d_z = z - v2_z;
	auto const t_y = y - v1_y;
	auto const t_z = z - v1_z;

	// <0, v1_y, v1_z> x <0, v2_y, v2_z> = <x, 0, 0>
	// x-component of this cross-product
	auto const cross_product_x = coord_fp{d_y * d_z} - coord_fp{t_y * t_z};

	return cross_product_x < 0;
}

template <size_t NumNodes, size_t NumSectors>
static constexpr size_t
sort_sectors_from_cursor(coord_fp y, coord_fp z,
	std::array<Node, NumNodes> const& nodes, size_t node_cursor,
	std::array<Sector const*, NumSectors>& result, size_t result_cursor)
{
	while (result_cursor < NumSectors) {
		auto const& node = nodes[node_cursor];
		auto const on_front_side = bsp::on_right_side(y, z, node.v1.y, node.v1.z, node.v2.y, node.v2.z);
		if (on_front_side) {
			if (node.frontSector != nullptr) {
				result[result_cursor++] = node.frontSector;
			} else {
				result_cursor = sort_sectors_from_cursor(y, z,
					nodes, node.frontNodeIdx, result, result_cursor);
			}
			if (node.backSector != nullptr) {
				result[result_cursor++] = node.backSector;
			} else {
				result_cursor = sort_sectors_from_cursor(y, z,
					nodes, node.backNodeIdx, result, result_cursor);
			}
		} else {
			if (node.backSector != nullptr) {
				result[result_cursor++] = node.backSector;
			} else {
				result_cursor = sort_sectors_from_cursor(y, z,
					nodes, node.backNodeIdx, result, result_cursor);
			}
			if (node.frontSector != nullptr) {
				result[result_cursor++] = node.frontSector;
			} else {
				result_cursor = sort_sectors_from_cursor(y, z,
					nodes, node.frontNodeIdx, result, result_cursor);
			}
		}
	}
	return result_cursor;
}

template <typename Map>
static constexpr auto
sort_sectors_from(coord_fp y, coord_fp z)
{
	std::array<Sector const*, Map::sectors.size()> result = {};
	sort_sectors_from_cursor(y, z, Map::nodes, 0, result, 0);
	return result;
}

template <typename Map>
static constexpr auto
sector_for(coord_fp y, coord_fp z)
{
	return *sort_sectors_from<Map>(y, z)[0];
}

static constexpr auto
viewangle_for(coord_fp a_y, coord_fp a_z, angle_fp theta, coord_fp y, coord_fp z)
{
	auto const angle = recenter(safe_atan2(y - a_y, z - a_z) - theta);

	return angle;
}

struct Range
{
	h_fp top, bottom;
	bool operator==(Range const& rhs) const
	{
		return (top == rhs.top) && (bottom == rhs.bottom);
	}
};

struct Drawseg
{
	scale_fp lam_1, lam_2;
	coord_fp tx_1, tx_2;
	Linedef const* linedef;
};

struct Drawrange
{
	Range range;
	Drawseg const* drawseg;
};

template <int screenHeight>
struct Renderer
{
public: // static members
	static constexpr auto h_min = std::numeric_limits<h_fp>::min();
	static constexpr auto h_max = std::numeric_limits<h_fp>::max();
	static constexpr auto fullOcclusion  = Range{h_min, h_max};

public: // members
	std::array<Range, screenHeight> m_occlusions;
	size_t m_next_occlusion = 0;

	std::array<Drawseg, screenHeight> m_drawsegs;
	size_t m_next_drawseg = 0;

	std::array<Drawrange, screenHeight> m_drawranges;
	size_t m_next_drawrange = 0;

public: // interface

	constexpr Renderer()
	{
		m_occlusions[0] = Range{h_min, -1};
		m_occlusions[1] = Range{screenHeight, h_max};
		m_next_occlusion = 2;
	}

	constexpr bool fullyOccluded() const
	{
		return m_occlusions[0] == fullOcclusion;
	}

	constexpr bool rangeVisible(h_fp h1, h_fp h2) const
	{
		if (h1 == h2) { return false; }

		for (size_t i = 0; i < m_next_occlusion - 1; i++) {
			if ((h1 < m_occlusions[i + 1].top)
			 || (h2 > m_occlusions[i].bottom)) {
			 	return true;
			}
		}

		return false;
	}

	constexpr auto calculateDrawseg(coord_fp a_y, coord_fp a_z, screen_fp h_1, screen_fp h_2, angle_fp theta, Linedef const* linedef) const
	{
		auto const v1_cam_dist = point_distance(linedef->v1.y - a_y, linedef->v1.z - a_z);

#if 0
		// One-sided linedef has right side
		// Normal should originate from right side
		// Slope is from v1 to v2, subtract pi / 2 (not add)
		auto const normal_angle = recenter(linedef->slope - pi / 2);

		auto const v1_cam_world_angle = recenter(safe_atan2(linedef->v1.y - a_y, linedef->v1.z - a_z));
		auto const v2_cam_world_angle = recenter(safe_atan2(linedef->v2.y - a_y, linedef->v2.z - a_z));

		auto const linedef_v1_world_angle = v1_cam_world_angle - linedef->slope;

		auto const normal_dist = coord_fp{sin(linedef_v1_world_angle) * v1_cam_dist};

		fprintf(stderr, "v1_cam_world_angle: %.2f normal dist %.2f -> %d\n", float{v1_cam_world_angle}, float{normal_dist});
#else
		auto const lam_1 = v1_cam_dist / m7::D;

		auto const v2_cam_dist = point_distance(linedef->v2.y - a_y, linedef->v2.z - a_z);
		auto const lam_2 = v2_cam_dist / m7::D;

		fprintf(stderr, "lam_1: %.02f lam_2: %.02f\n", lam_1, lam_2);
#endif

		return Drawseg
			{ .lam_1 = lam_1
			, .lam_2 = lam_2
			, .tx_1 = {}
			, .tx_2 = {}
			, .linedef = linedef
		};
	}

	template <typename T, size_t N>
	static void array_insert(std::array<T, N>& arr, size_t& cursor, size_t idx, T const& elem)
	{
		cursor++;
		for (auto i = cursor; i > idx; i--) {
			arr[i] = arr[i - 1];
		}
		arr[idx] = elem;
	}

	template <typename T, size_t N>
	static auto array_pop(std::array<T, N>& arr, size_t& cursor, size_t idx)
	{
		auto const result = arr[idx];
		cursor--;
		for (auto i = idx; i < cursor; i++) {
			arr[i] = arr[i + 1];
		}
		return result;
	}

	template <typename T, size_t N>
	static void array_append(std::array<T, N>& arr, size_t& cursor, T const& elem)
	{
		arr[cursor++] = elem;
	}

	template <typename T, size_t N>
	static auto& array_last(std::array<T, N>& arr, size_t const& cursor)
	{
		return arr[cursor - 1];
	}

	constexpr void occludeDrawseg(h_fp h1, h_fp h2, Drawseg const* drawseg)
	{
		auto printOD = [this]() {
			fprintf(stderr, "drawranges: ");
			for (size_t i = 0; i < m_next_drawrange; i++) {
				fprintf(stderr, "(%d, %d, %p) ", m_drawranges[i].range.top, m_drawranges[i].range.bottom, m_drawranges[i].drawseg);
			}
			fprintf(stderr, "\n");
			fprintf(stderr, "occlusions: ");
			for (size_t i = 0; i < m_next_occlusion; i++) {
				auto const [h1, h2] = m_occlusions[i];
				fprintf(stderr, "(%d, %d) ", h1, h2);
			}
			fprintf(stderr, "\n");
		};

		printOD();
		fprintf(stderr, "occlude %d, %d\n", h1, h2);
		// 0 [T1 h1<=B1+ T2<=h2 B2] 160
		// 0 [T1 B1+]__[<h1 h2<]__[T2 B2] 160
		// 0 [T1 h1<=B1+ h2<]__[T2 B2] 160
		// 0 [T1 B1+]__[<h1 T2<=h2 B2] 160
		for (size_t i = 0; i < m_next_occlusion - 1; ) {
			auto const b1 = m_occlusions[i].bottom;
			auto const t2 = m_occlusions[i + 1].top;

			auto const h1_past_b1 = h1 <= b1 + 1;
			auto const h2_past_t2 = t2 <= h2;
			auto const h1_in_gap  = (b1 + 1 < h1) && (h1 < t2);
			auto const h2_in_gap  = (b1 + 1 < h2) && (h2 < t2);

			fprintf(stderr, "b1: %d, t2: %d: ", b1, t2);

			// Big. Remove gap
			if (h1_past_b1 && h2_past_t2) {
				fprintf(stderr, "big\n");
				m_occlusions[i].bottom = m_occlusions[i + 1].bottom;
				array_pop(m_occlusions, m_next_occlusion, i + 1);
				array_append(m_drawranges, m_next_drawrange, Drawrange
					{ .range = Range{b1 + 1, t2}
					, .drawseg = drawseg
				});
				continue;

			// Small. Insert new
			} else if (h1_in_gap && h2_in_gap) {
				fprintf(stderr, "small\n");
				array_insert(m_occlusions, m_next_occlusion, i + 1, {h1, h2});
				array_append(m_drawranges, m_next_drawrange, Drawrange
					{ .range = Range{h1 + 1, h2}
					, .drawseg = drawseg
				});
				break;

			// h2 in gap, h1 over b1. Extend b1 down to h2
			} else if (h1_past_b1 && h2_in_gap) {
				fprintf(stderr, "h2 in gap\n");
				m_occlusions[i].bottom = h2;
				array_append(m_drawranges, m_next_drawrange, Drawrange
					{ .range = Range{b1 + 1, h2}
					, .drawseg = drawseg
				});
				break;

			// h1 in gap, h2 over t2. Extend t2 up to h1
			} else if (h1_in_gap && h2_past_t2) {
				fprintf(stderr, "h1 in gap\n");
				m_occlusions[i + 1].top = h1;
				array_append(m_drawranges, m_next_drawrange, Drawrange
					{ .range = Range{h1 + 1, t2}
					, .drawseg = drawseg
				});
				break;
			} else {
				fprintf(stderr, "none\n");
			}

			i++;
		}

		fprintf(stderr, "completed\n");
		printOD();
		fprintf(stderr, "\n");
	}

	template <typename Map, size_t NumSectors>
	constexpr void
	calculateDrawranges(coord_fp a_y, coord_fp a_z, angle_fp theta,
		std::array<Sector const*, NumSectors> const& sortedSectors)
	{
		for (auto&& sectorPtr : sortedSectors) {
			for (auto const segId : sectorPtr->segIds) {
				if (segId < 0) { break; }
				auto const segPtr = &Map::linedefs[segId];

				if (segPtr->front && segPtr->back) { continue; }
				auto const angle_1 = bsp::viewangle_for(a_y, a_z, theta,
					segPtr->v1.y, segPtr->v1.z);
				auto const angle_2 = bsp::viewangle_for(a_y, a_z, theta,
					segPtr->v2.y, segPtr->v2.z);
				if (angle_2 > angle_1) { continue; }
				if ((abs(angle_1) >= bsp::pi / 2)
				 && (abs(angle_2) >= bsp::pi / 2)) { continue; }

				auto const y_1 = bsp::viewangletoy(angle_1);
				auto const y_2 = bsp::viewangletoy(angle_2);

				auto const h_1 = int(y_2 + 80);
				auto const h_2 = int(y_1 + 80);

				if (rangeVisible(h_1, h_2)) {
					auto drawseg = calculateDrawseg(a_y, a_z, theta, h_1, h_2, segPtr);
					array_append(m_drawsegs, m_next_drawseg, drawseg);
					auto const drawsegPtr = &array_last(m_drawsegs, m_next_drawseg);
					occludeDrawseg(h_1, h_2, drawsegPtr);
				}

				if (fullyOccluded()) {
					return;
				}
			}
		}

		assert(false);
	}

	constexpr auto begin() const { return m_drawranges.begin(); }
	constexpr auto end()   const { return m_drawranges.begin() + m_next_drawrange; }
};

} // namespace bsp
