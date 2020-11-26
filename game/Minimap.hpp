#pragma once

#include <cmath>

#include <array>
#include <limits>
#include <tuple>
#include <algorithm>

#include "math.hpp"
#include "vram.hpp"

template <typename Map, int height, int width, size_t bytes_per_pixel>
struct Minimap
{
private: // types
	using co = bsp::coord_fp;
	using ag = bsp::angle_fp;

public: // members
	static constexpr auto R = int{width / 2};
	static constexpr auto T = int{height / 2};
	unsigned char buf[height + 1][width + 1][bytes_per_pixel];

private: // internal interface
	void plotPt(co y, co z, int r, int g, int b)
	{
		if ((abs(y) > T) || (abs(z) > R)) { return; }
		auto const row = std::clamp(int(y) + T, 0, height);
		auto const col = std::clamp(int(z) + R, 0, width);
		buf[height - row][col][2] = r;
		buf[height - row][col][1] = g;
		buf[height - row][col][0] = b;
	};

	void plotLine(co v1_y, co v1_z, co v2_y, co v2_z, int r, int g, int b)
	{
		auto const dy = v2_y - v1_y;
		auto const dz = v2_z - v1_z;
		if (dz == 0) {
			if (dy > 0) {
				for (int i = 0; i < dy; i++) {
					plotPt(v1_y + i, v1_z, r, g, b);
				}
			} else {
				for (int i = dy; i < 0; i++) {
					plotPt(v1_y + i, v1_z, r, g, b);
				}
			}
		} else {
			auto const m = float(dy) / dz;
			if (dz > 0) {
				for (int i = 0; i < dz; i++) {
					plotPt(v1_y + m * i, v1_z + i, r, g, b);
				}
			} else {
				for (int i = dz; i < 0; i++) {
					plotPt(v1_y + m * i, v1_z + i, r, g, b);
				}
			}
		}
	};

	static inline auto rgb15(bsp::Color const rgb)
	{
		return std::make_tuple
			( (rgb >> 10) & 0x1f
			, (rgb >>  5) & 0x1f
			, (rgb >>  0) & 0x1f
		);
	};

public: // interface
	template <int RenderHeight>
	void render(bsp::coord_fp const a_x, bsp::coord_fp const a_y, bsp::coord_fp const a_z, bsp::coord_fp const phi,
		bsp::Renderer<RenderHeight> const& renderer)
	{
		auto const& cam_sector = bsp::sector_for<Map>(a_y, a_z);

		for (int y = -T; y <= T; y++) {
			for (int z = -R; z <= R; z++) {
				auto const& sector = bsp::sector_for<Map>(y, z);
				auto const bright = (sector.color == cam_sector.color)
					? 4 : 1;
				auto const [r, g, b] = rgb15(sector.color);
				plotPt(y, z, r * bright, g * bright, b * bright);
			}
		}

		plotPt(a_y, a_z, 0xff, 0xff, 0xff);

		for (auto&& ld : Map::linedefs) {
			auto const [r, g, b] = (ld.back != nullptr)
				? std::make_tuple(0x7f, 0x00, 0x00)
				: std::make_tuple(0x7f, 0x7f, 0x00);
			plotLine(ld.v1.y, ld.v1.z, ld.v2.y, ld.v2.z, r, g, b);
		}
		for (auto&& v : Map::vertices) {
			plotPt(v.y, v.z, 0xff, 0, 0xff);
		}

		auto const so_y = a_y + ::sin(phi) * bsp::m7::D;
		auto const so_z = a_z + ::cos(phi) * bsp::m7::D;
		auto const sx_y = so_y + ::sin(phi + bsp::pi / 2) * bsp::m7::T;
		auto const sx_z = so_z + ::cos(phi + bsp::pi / 2) * bsp::m7::T;
		plotLine(a_y, a_z, so_y, so_z, 0x7f, 0x7f, 0x7f);
		plotLine(so_y, so_z, sx_y, sx_z, 0x7f, 0x7f, 0x7f);
		plotLine(a_y, a_z, sx_y, sx_z, 0x3f, 0x3f, 0x3f);

		auto const ray1 = phi + bsp::ytoviewangle(-80);
		auto const cast1_y = a_y + ::sin(ray1) * 1000;
		auto const cast1_z = a_z + ::cos(ray1) * 1000;
		plotLine(a_y, a_z, cast1_y, cast1_z, 0x00, 0x7f, 0x00);

		auto const ray2 = phi + bsp::ytoviewangle(80);
		auto const cast2_y = a_y + ::sin(ray2) * 1000;
		auto const cast2_z = a_z + ::cos(ray2) * 1000;
		plotLine(a_y, a_z, cast2_y, cast2_z, 0x00, 0x7f, 0x00);

		fprintf(stderr, "rendering drawranges:\n");
		for (auto&& drawrange : renderer) {
			assert(drawrange.drawseg != nullptr);
			fprintf(stderr, "(%d, %d, ds %p, s %p)\n", drawrange.range.top, drawrange.range.bottom, drawrange.drawseg, drawrange.drawseg->seg);

			auto const drawsegPtr = drawrange.drawseg;
			auto const segPtr = drawsegPtr->seg;

			plotLine(segPtr->v1.y, segPtr->v1.z, segPtr->v2.y, segPtr->v2.z, 0xff, 0xff, 0xff);
		}
	}
};
