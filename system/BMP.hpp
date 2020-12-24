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

static void write_bitmap(char const* path, unsigned char const* colors_data, uint32_t height, uint32_t width)
{
	unsigned char const padding[] = {0, 0, 0};
	auto const padding_size = (4 - (width * bytes_per_pixel) % 4) % 4;

	auto image = std::unique_ptr<FILE, decltype(&::fclose)>
		{ ::fopen(path, "wb")
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
		static int frame = 0;
		if (frame++ == 60) {
			write_bitmap("image.bmp", (unsigned char const*)framebuf, screenHeight + 400, screenWidth + 400);
			exit(0);
		}
	}

}; // struct BMP
