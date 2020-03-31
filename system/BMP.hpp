#pragma once

#include <stdlib.h>
#include <stdio.h>

#include <memory>

#include "PC.hpp"

#include "game/BSP.hpp"

static constexpr auto file_header_size = 14;
static constexpr auto info_header_size = 40;
static constexpr auto bytes_per_pixel = 3;

struct file_header {
	char signature[2];
	uint32_t size;
	uint32_t _;
	uint32_t array_start;
} __attribute__((packed));

auto const make_file_header = [](uint32_t height, uint32_t width, int padding)
{
	auto const array_start = file_header_size + info_header_size;
	auto const file_size = array_start + (bytes_per_pixel * width + padding) * height;

	return file_header
		{ .signature = {'B', 'M'}
		, .size = file_size
		, .array_start = array_start
	};
};
struct info_header {
	uint32_t header_size;
	uint32_t width, height;
	uint16_t color_planes;
	uint16_t bpp;
	uint32_t compression;
	uint32_t size;
	uint32_t horz_res, vert_res;
	uint32_t colors_in_table;
	uint32_t color_count;
} __attribute__((packed));

auto const make_info_header = [](uint32_t height, uint32_t width) {
	return info_header
		{ .header_size = info_header_size
		, .width = width
		, .height = height
		, .color_planes = 1
		, .bpp = bytes_per_pixel * 8
	};
};

static void write_bitmap(unsigned char const* colors_data, uint32_t height, uint32_t width)
{
	unsigned char const padding[] = {0, 0, 0};
	auto const padding_size = (4 - (width * bytes_per_pixel) % 4) % 4;

	auto image = std::unique_ptr<FILE, decltype(&::fclose)>
		{ ::fopen("image.bmp", "wb")
		, ::fclose
	};

	auto const fileHeader = make_file_header(height, width, padding_size);
	auto const infoHeader = make_info_header(height, width);

	::fwrite((unsigned char const*)&fileHeader, 1, file_header_size, image.get());
	::fwrite((unsigned char const*)&infoHeader, 1, info_header_size, image.get());

	for (uint32_t i = 0; i < height; i++) {
		::fwrite(colors_data + ((height - i - 1) * width * bytes_per_pixel), bytes_per_pixel, width, image.get());
		::fwrite(padding, 1, padding_size, image.get());
	}
}

class BMP : public PC<BMP>
{

public: // static members
	static inline unsigned char framebuf[screenHeight][screenWidth][bytes_per_pixel];

public: // interface

	static void waitNextFrame()
	{}

	static void pumpEvents(event::queue_type& queue)
	{
		queue.push_back(event::Key
			{ .type  = event::Key::Type::Right
			, .state = event::Key::State::On
		});
		queue.push_back(event::Key
			{ .type  = event::Key::Type::Down
			, .state = event::Key::State::On
		});
	}

	static void drawPx(int colPx, int rowPx, int r, int g, int b)
	{
		framebuf[rowPx][colPx][2] = r;
		framebuf[rowPx][colPx][1] = g;
		framebuf[rowPx][colPx][0] = b;
	}

	static void updateDisplay()
	{
		unsigned char buf[401][401][3];

		using co = bsp::coord_fp;
		using ag = bsp::angle_fp;
		auto const a_x = co{0};
		auto const a_y = co{100};
		auto const a_z = co{0};
		auto const phi = ag{4};

		auto const& cam_sector = bsp::sector_for(a_y, a_z);

		auto const plotPt = [&](co y, co z, int r, int g, int b) {
			if ((abs(y) > 200) || (abs(z) > 200)) { return; }
			auto const row = std::clamp(int(y) + 200, 0, 400);
			auto const col = std::clamp(int(z) + 200, 0, 400);
			buf[400 - row][col][2] = r;
			buf[400 - row][col][1] = g;
			buf[400 - row][col][0] = b;
		};

		auto const rgb15 = [](bsp::Color const rgb) {
			return std::make_tuple
				( (rgb >> 10) & 0x1f
				, (rgb >>  5) & 0x1f
				, (rgb >>  0) & 0x1f
			);
		};

		for (int y = -200; y <= 200; y++) {
			for (int z = -200; z <= 200; z++) {
				auto const& sector = bsp::sector_for(y, z);
				auto const bright = (sector.color == cam_sector.color)
					? 4 : 1;
				auto const [r, g, b] = rgb15(sector.color);
				plotPt(y, z, r * bright, g * bright, b * bright);

			}
		}

		plotPt(a_y, a_z, 0xff, 0xff, 0xff);

		auto const plotLine = [&](co v1_y, co v1_z, co v2_y, co v2_z, int r, int g, int b) {
			auto const dy = v2_y - v1_y;
			auto const dz = v2_z - v1_z;
			if (dz == 0) {
				if (dy > 0) {
					for (int i = 0; i < dy; i++) { plotPt(v1_y + i, v1_z, r, g, b); }
				} else {
					for (int i = dy; i < 0; i++) { plotPt(v1_y + i, v1_z, r, g, b); }
				}
			} else {
				auto const m = float(dy) / dz;
				if (dz > 0) {
					for (int i = 0; i < dz; i++) { plotPt(v1_y + m * i, v1_z + i, r, g, b); }
				} else {
					for (int i = dz; i < 0; i++) { plotPt(v1_y + m * i, v1_z + i, r, g, b); }
				}
			}
			
		};
		for (auto&& ld : bsp::linedefs) {
			auto const [r, g, b] = (ld.back != nullptr)
				? std::make_tuple(0x7f, 0x00, 0x00)
				: std::make_tuple(0x7f, 0x7f, 0x00);
			plotLine(ld.v1.y, ld.v1.z, ld.v2.y, ld.v2.z, r, g, b);
		}
		for (auto&& v : bsp::vertices) {
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

		auto const sortedSectors = bsp::sort_sectors_from(a_y, a_z);
		auto occlusionArray = std::array<bsp::Linedef const*, screenHeight>{};
		for (auto&& sectorPtr : sortedSectors) {
			for (auto const segId : sectorPtr->segIds) {
				if (segId < 0) { break; }
				auto const segPtr = &bsp::linedefs[segId];

				if (segPtr->front && segPtr->back) { continue; }
				auto const angle_1 = bsp::viewangle_for(a_y, a_z, phi,
					segPtr->v1.y, segPtr->v1.z);
				auto const angle_2 = bsp::viewangle_for(a_y, a_z, phi,
					segPtr->v2.y, segPtr->v2.z);
				if (angle_2 > angle_1) { continue; }
				if ((abs(angle_1) >= bsp::pi / 2)
				 && (abs(angle_2) >= bsp::pi / 2)) { continue; }

				auto const y_1 = bsp::viewangletoy(angle_1);
				auto const y_2 = bsp::viewangletoy(angle_2);

				auto const h_1 = int(y_1 + 80);
				auto const h_2 = int(y_2 + 80);

				for (auto h = h_2; h < h_1; h++) {
					if (occlusionArray[h] == nullptr) {
						occlusionArray[h] = segPtr;
					}
				}
			}
		}

		for (int h = 0; h < 160; h++) {
			for (int z = 100; z <= 200; z++) {
				plotPt(200 - h, z, 0,0,0);
			}
		}

		int h = 0;
		for (auto&& segPtr : occlusionArray) {
			auto const y_1 = segPtr->v1.y;
			auto const z_1 = segPtr->v1.z;
			auto const y_2 = segPtr->v2.y;
			auto const z_2 = segPtr->v2.z;

auto const al_s = atan2(y_2 - y_1, z_2 - z_1);
auto const al_1 = atan2(y_1 - a_y, z_1 - a_z);
auto const al_2 = atan2(y_2 - a_y, z_2 - a_z);
auto const al_n = al_s + bsp::pi / 2;

auto const dist = [](co y, co z) { return sqrt(y * y + z * z); };
auto const d_1 = dist(y_1 - a_y, z_1 - a_z);
auto const d_n = sin(bsp::pi - (al_1 - al_n) + bsp::pi/2) * d_1;

auto const be_1 = al_1 + bsp::pi - phi;
auto const be_2 = al_2 + bsp::pi - phi;

auto const h_1 = std::clamp(int(tan(be_1) * bsp::m7::D), -80, 80);
auto const h_2 = std::clamp(int(tan(be_2) * bsp::m7::D), -80, 80);

auto const va_1 = atan2(h_1, bsp::m7::D);
auto const va_2 = atan2(h_2, bsp::m7::D);

auto const sc_1 = (bsp::m7::D * cos(va_1 + phi - al_n))
	/ (d_n * cos(va_1));
auto const sc_2 = (bsp::m7::D * cos(va_2 + phi - al_n))
	/ (d_n * cos(va_2));

fprintf(stderr, "d_n %.02f, angles %.02f-%.02f -> %d-%d, scale from %.02f to %.02f\n", d_n, be_1, be_2, h_1 + 80, h_2 + 80, sc_1, sc_2);

auto const dsc = (sc_2 - sc_1) / float(h_2 - h_1);
auto const sc = abs(sc_1 + dsc * float((h - 80) - h_1));

			auto const bright = std::min(int(sc * 8), 8);
			auto const width = std::min(int(sc * 100), 100);
			fprintf(stderr, "sc %.02f -> bright %d width %d\n\n", sc, bright, width);
			plotLine(segPtr->v1.y, segPtr->v1.z, segPtr->v2.y, segPtr->v2.z, 0xff, 0xff, 0xff);
			auto const [r, g, b] = rgb15(segPtr->front->color);
			auto const dw = std::min(int(width / 2), 50);
			for (int z = 150 - dw; z <= 150 + dw; z++) {
				plotPt(200 - h, z, r * bright, g * bright, b * bright);
			}
			h++;
		}

		write_bitmap((unsigned char const*)buf, 401, 401);
		exit(0);

		static int frame = 0;
		if (frame++ == 10) {
			write_bitmap((unsigned char const*)framebuf, screenHeight, screenWidth);
			exit(0);
		}
	}

}; // struct BMP
