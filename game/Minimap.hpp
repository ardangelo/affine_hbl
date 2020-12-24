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

	void plotPolar(co v_y, co v_z, co mag, ag angle, int r, int g, int b)
	{
		auto const d_y = (co)(::sin(angle) * mag);
		auto const d_z = (co)(::cos(angle) * mag);
		plotLine(v_y, v_z, v_y + d_y, v_z + d_z, r, g, b);
	}

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
	void render(bsp::coord_fp const a_x, bsp::coord_fp const a_y, bsp::coord_fp const a_z, bsp::coord_fp const theta,
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

		auto const so_y = a_y + ::sin(theta) * bsp::m7::D;
		auto const so_z = a_z + ::cos(theta) * bsp::m7::D;
		auto const sx_y = so_y + ::sin(theta + bsp::pi / 2) * bsp::m7::T;
		auto const sx_z = so_z + ::cos(theta + bsp::pi / 2) * bsp::m7::T;
		plotLine(a_y, a_z, so_y, so_z, 0x7f, 0x7f, 0x7f);
		plotLine(so_y, so_z, sx_y, sx_z, 0x7f, 0x7f, 0x7f);
		plotLine(a_y, a_z, sx_y, sx_z, 0x3f, 0x3f, 0x3f);

		auto const ray1 = theta + bsp::ytoviewangle(-80);
		auto const cast1_y = a_y + ::sin(ray1) * 1000;
		auto const cast1_z = a_z + ::cos(ray1) * 1000;
		plotLine(a_y, a_z, cast1_y, cast1_z, 0x00, 0x7f, 0x00);

		auto const ray2 = theta + bsp::ytoviewangle(80);
		auto const cast2_y = a_y + ::sin(ray2) * 1000;
		auto const cast2_z = a_z + ::cos(ray2) * 1000;
		plotLine(a_y, a_z, cast2_y, cast2_z, 0x00, 0x7f, 0x00);

		fprintf(stderr, "rendering drawranges:\n");
		for (auto&& drawrange : renderer) {
			assert(drawrange.drawseg != nullptr);
			fprintf(stderr, "(%d, %d)\n", drawrange.range.top, drawrange.range.bottom);

			auto const drawsegPtr = drawrange.drawseg;
			auto const linedefPtr = drawsegPtr->linedef;

			plotLine(linedefPtr->v1.y, linedefPtr->v1.z, linedefPtr->v2.y, linedefPtr->v2.z, 0xff, 0xff, 0xff);

			{ using namespace bsp;
				auto const [h_1, h_2] = drawrange.range;
				auto const linedef = drawrange.drawseg->linedef;

				auto const normal_angle = recenter(linedef->slope + pi / 2);

				auto const v1_cam_world_angle = recenter(safe_atan2(linedef->v1.y - a_y, linedef->v1.z - a_z));

				auto const linedef_v1_world_angle = v1_cam_world_angle - linedef->slope;

				auto const v1_cam_dist = point_distance(linedef->v1.y - a_y, linedef->v1.z - a_z);

				auto const normal_dist = abs(coord_fp{bsp::sin(linedef_v1_world_angle) * v1_cam_dist});

				plotPolar(a_y, a_z, normal_dist, normal_angle, 0xff, 0xff, 0x00);

				auto const skew_1 = normal_angle - (theta + ytoviewangle(h_1 - 80));
				auto const dist_1 = (skew_1 != (pi / 2))
					? normal_dist / abs(bsp::cos(skew_1))
					: 100;

				auto const skew_2 = normal_angle - (theta + ytoviewangle(h_2 - 80));
				auto const dist_2 = (skew_2 != (pi / 2))
					? normal_dist / abs(bsp::cos(skew_2))
					: 100;

				auto const y_dist = [](screen_fp y) {
					return (coord_fp)::sqrt(m7::D * m7::D + y * y);
				};
				auto const lambda_1 = (dist_1 > 0)
					? y_dist(h_1 - 80) / dist_1
					: std::numeric_limits<scale_fp>::max();
				auto const lambda_2 = (dist_2 > 0)
					? y_dist(h_2 - 80) / dist_2
					: std::numeric_limits<scale_fp>::max();
				auto const dlambda_dh = (lambda_2 - lambda_1) / (h_2 - h_1);

				for (auto i = h_1; i < h_2; i++) {
					auto const lambda = dlambda_dh * (i - h_1) + lambda_1;
					auto const line_width = (int)(m7::D * lambda);

					auto const [r, g, b] = rgb15(linedef->front->color);

					auto const z_1 = 100 - (line_width / 2);
					auto const z_2 = 100 + (line_width / 2);
					plotLine(i, z_1, i, z_2,
						std::min(0xff, (int)(8 * r * lambda)),
						std::min(0xff, (int)(8 * g * lambda)),
						std::min(0xff, (int)(8 * b * lambda)));
				}
			}
		}
	}
};
