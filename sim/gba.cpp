#include <array>

#include "system.hpp"

using sys = GBA;

static constexpr auto tiles = std::array<vram::CharEntry, 2> {
{
	{ 0x11111111
	, 0x01111111
	, 0x01111111
	, 0x01111111
	, 0x01111111
	, 0x01111111
	, 0x01111111
	, 0x00000001
	}
,
	{ 0x00000000, 0x00100100, 0x01100110, 0x00011000
	, 0x00011000, 0x01100110, 0x00100100, 0x00000000
	}
} };

static constexpr auto Red   = vram::Rgb15::make(31,  0,  0);
static constexpr auto Blue  = vram::Rgb15::make( 0, 31,  0);
static constexpr auto Green = vram::Rgb15::make( 0,  0, 31);
static constexpr auto Grey  = vram::Rgb15::make(15, 15, 15);

int main(int argc, char const* argv[])
{
	sys::dispCnt = 0x1100;

	auto constexpr charBlockBase = 0;
	auto constexpr screenBlockBase = 28;

	sys::bg0Cnt = vram::BgCnt
		{ .priority = 0
		, .charBlockBase = charBlockBase
		, .mosaicEnabled = false
		, .palMode = vram::BgCnt::PalMode::Bits4
		, .screenBlockBase = screenBlockBase
		, .affineWrapEnabled = false
		, .mapSize = vram::BgCnt::MapSize::Reg64x64
	};

	auto offs = uint32_t{240};
	sys::bg0HorzOffs = offs;
	offs++;
	sys::bg0HorzOffs = offs;

	sys::palBanks[0][1] = Red;
	sys::palBanks[1][1] = Blue;
	sys::palBanks[2][1] = Green;
	sys::palBanks[3][1] = Grey;

	sys::charBlocks[0][0] = tiles[0];
	sys::charBlocks[0][1] = tiles[1];

	for (uint32_t screenBlockOffs = 0; screenBlockOffs < 4; screenBlockOffs++) {
		for (uint32_t screenEntryIdx = 0; screenEntryIdx < 32 * 32; screenEntryIdx++) {
			sys::screenBlocks[screenBlockBase + screenBlockOffs][screenEntryIdx] = vram::ScreenEntry
				{ .tileId = 0
				, .horzFlip = 0
				, .vertFlip = 0
				, .palBank = (uint16_t)screenBlockOffs
			};
		}
	}

	sys::bg0Cnt |= vram::BgCnt{ .affineWrapEnabled = true };

	sys::screenBlocks[screenBlockBase][0] |= vram::ScreenEntry{ .tileId = 0x1 };

	while (true) {}

	return 0;
}
