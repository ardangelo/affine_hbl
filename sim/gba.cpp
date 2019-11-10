#include <tonc.h>

#include <assert.h>

#include <type_traits>
#include <array>

static constexpr auto bpp = 8;
static constexpr auto pitch = bpp * 8;

template <typename T>
struct Bitfield
{
	constexpr operator T() const { return *(T*)this; }
};

using CharEntry = TILE;

struct ScreenEntry : public Bitfield<uint16_t>
{
	uint16_t tileId  : 10;
	uint16_t horFlip :  1;
	uint16_t verFlip :  1;
	uint16_t palBank :  4;

	constexpr static ScreenEntry tile_id(uint16_t tile_id_) {
		return ScreenEntry { .tileId = tile_id_ };
	}
	constexpr static ScreenEntry pal_bank(uint16_t pal_bank_) {
		return ScreenEntry { .palBank = pal_bank_ };
	}

} __attribute__((packed));
static_assert(sizeof(ScreenEntry) == 2);

static constexpr auto charEntries   = 0x4000 / sizeof(CharEntry);
static constexpr auto screenEntries = 0x800 / sizeof(ScreenEntry);

static constexpr auto screenBlockHeight = 32;
static constexpr auto screenBlockWidth  = 32;

auto charBlocks    = tile_mem;
auto screenBlocks  = se_mem;

static constexpr CharEntry tiles[] =
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
};

struct Rgb15 : public Bitfield<uint16_t>
{
	uint16_t _ : 1;
	uint16_t r : 5;
	uint16_t g : 5;
	uint16_t b : 5;

	constexpr Rgb15() : _{}, r{}, g{}, b{} {}
	constexpr Rgb15(uint16_t r_, uint16_t g_, uint16_t b_) : _{}, r{r_}, g{g_}, b{b_} {}
};
static_assert(sizeof(Rgb15) == 2);

auto bgPalBanks = pal_bg_bank;

struct BgControlReg : public Bitfield<uint16_t>
{
	uint16_t priority : 2;
	uint16_t charBaseBlock : 2;

	uint16_t _ : 2;

	uint16_t mosaicEnabled : 1;

	enum class ColorMode : uint16_t
		{ FourBitsPerPixel  = 0
		, EightBitsPerPixel = 1
	};

	ColorMode colorMode : 1;

	uint16_t screenBaseBlock : 5;
	uint16_t affineWrapEnabled : 1;

	enum class BgSize : uint16_t
		{ Reg32x32 = 0
		, Reg64x32 = 1
		, Reg32x64 = 2
		, Reg64x64 = 3

		, Aff16x16 = 0
		, Aff32x32 = 1
		, Aff64x64 = 2
		, Aff128x128 = 3
	};

	BgSize bgSize : 2;

} __attribute__((packed));
static_assert(sizeof(BgControlReg) == 2);

struct Bg64x64
{
	BgControlReg volatile& bgControlReg;
	uint32_t &horzOffs, &vertOffs;

	static constexpr uint32_t charBaseBlock = 0;
	static constexpr uint32_t screenBaseBlock = 28;

	Bg64x64()
		: bgControlReg{*(BgControlReg*)(0x04000008 + 2 * 0)} // REG_BG0CNT
		, horzOffs{*(uint32_t*)(0x04000010 + 4 * 0)} // REG_BG0HOFS
		, vertOffs{*(uint32_t*)(0x04000012 + 4 * 0)} // REG_BGVHOFS
	{
		bgControlReg.priority = 0;
		bgControlReg.charBaseBlock = charBaseBlock;
		bgControlReg.mosaicEnabled = false;
		bgControlReg.colorMode = BgControlReg::ColorMode::FourBitsPerPixel;
		bgControlReg.screenBaseBlock = screenBaseBlock;
		bgControlReg.affineWrapEnabled = false;
		bgControlReg.bgSize = BgControlReg::BgSize::Reg64x64;

		horzOffs = 0;
		vertOffs = 0;
	}

	constexpr auto charBlockIdx(auto tileId) const {
		return charBaseBlock
		    + (tileId / charEntries);
	}

	constexpr auto charEntryIdx(auto tileId) const {
		return (tileId % charEntries);
	}

	constexpr auto screenBlockIdx(auto tx, auto ty) const {
		return screenBaseBlock
		     + (ty / screenBlockHeight) * (pitch / 32)
		     + (tx / screenBlockWidth);
	}

	constexpr auto screenEntryIdx(auto tx, auto ty) const {
		return (ty % screenBlockHeight) * screenBlockHeight
		     + (tx % screenBlockWidth);
	}
};

int main(int argc, char const* argv[]) {
	REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_OBJ;

	auto bg0 = Bg64x64{};

	bg0.horzOffs = 0;
	bg0.vertOffs = 0;

	auto const charBaseBlock = bg0.charBaseBlock;
	charBlocks[charBaseBlock + 0][0] = tiles[0];
	charBlocks[charBaseBlock + 0][1] = tiles[1];

	bgPalBanks[0][1] = Rgb15{31,  0,  0};
	bgPalBanks[1][1] = Rgb15{ 0, 31,  0};
	bgPalBanks[2][1] = Rgb15{ 0,  0, 31};
	bgPalBanks[3][1] = Rgb15{16, 16, 16};

	auto const screenBaseBlock = bg0.screenBaseBlock;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < screenBlockHeight * screenBlockWidth; j++) {
			screenBlocks[screenBaseBlock + i][j] = ScreenEntry
				{ .tileId  = 0
				, .horFlip = 0
				, .verFlip = 0
				, .palBank = (uint16_t)i
				};
		}
	}

	auto const screenBlockIdx = bg0.screenBlockIdx(0, 0);
	auto const screenEntryIdx = bg0.screenEntryIdx(0, 0);
	screenBlocks[screenBlockIdx][screenEntryIdx] |= ScreenEntry::tile_id(1);

	while (1) {}

	return 0;
}