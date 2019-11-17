#pragma once

#include <array>

namespace vram
{

struct BgCnt
{
	uint16_t priority : 2;
	uint16_t charBlockBase : 2;

	uint16_t _ : 2;

	uint16_t mosaicEnabled : 1;

	enum class PalMode : uint16_t
		{ Bits4 = 0
		, Bits8 = 1

		, Color16  = 0
		, Color256 = 1
	};
	PalMode palMode : 1;

	uint16_t screenBlockBase : 5;
	uint16_t affineWrapEnabled : 1;

	enum class MapSize : uint16_t
		{ Reg32x32 = 0
		, Reg64x32 = 1
		, Reg32x64 = 2
		, Reg64x64 = 3

		, Aff16x16 = 0
		, Aff32x32 = 1
		, Aff64x64 = 2
		, Aff128x128 = 3
	};
	MapSize mapSize : 2;

	constexpr operator uint16_t() const {
		return (priority          <<  0)
			 | (charBlockBase     <<  2)
			 | (mosaicEnabled     <<  6)
			 | ((uint16_t)palMode <<  7)
			 | (screenBlockBase   <<  8)
			 | (affineWrapEnabled <<  13)
			 | ((uint16_t)mapSize <<  14);
	}
} __attribute__((packed));

struct ScreenEntry {
	uint16_t tileId   : 10;
	uint16_t horzFlip :  1;
	uint16_t vertFlip :  1;
	uint16_t palBank  :  4;

	constexpr operator uint16_t() const {
		return (tileId   <<  0)
			 | (horzFlip << 10)
			 | (vertFlip << 11)
			 | (palBank  << 12);
	}
} __attribute__((packed));
static_assert(sizeof(ScreenEntry) == 2);

struct screen_blocks {
	using ScreenBlock = uint16_t[0x400];

	using storage = ScreenBlock[0x20];

	template <size_t address>
	struct memmap {
		static inline auto base = reinterpret_cast<ScreenBlock volatile*>(address);

		constexpr auto& operator[] (size_t i) const {
			return base[i];
		}
	};
};

struct CharEntry {
	uint32_t buf[8];

public:
	constexpr CharEntry() : buf{} {}

	constexpr CharEntry(std::initializer_list<uint32_t> lines)
		: buf{lines.begin()[0], lines.begin()[1], lines.begin()[2], lines.begin()[3], lines.begin()[4], lines.begin()[5], lines.begin()[6], lines.begin()[7]}
	{}

	auto& operator= (CharEntry const& rhs) volatile {
		std::copy(rhs.buf, rhs.buf + 8, buf);
		return *this;
	}
};

struct char_blocks {
	using CharBlock = CharEntry[0x200];

	using storage = CharBlock[0x4];

	template <size_t address>
	struct memmap {
		static inline auto base = reinterpret_cast<CharBlock volatile*>(address);

		constexpr auto& operator[] (size_t i) const {
			return base[i];
		}
	};
};

struct Rgb15 {
	constexpr static uint16_t make(uint16_t r, uint16_t g, uint16_t b) {
		return ((r & 0x1f) << (5 * 0))
		     | ((g & 0x1f) << (5 * 1))
		     | ((b & 0x1f) << (5 * 2));
	}
};

struct pal_banks {
	using PalBank = uint16_t[0x10];

	using storage = PalBank[0x10];

	template <size_t address>
	struct memmap {
		static inline auto base = reinterpret_cast<PalBank volatile*>(address);

		constexpr auto& operator[] (size_t i) const {
			return base[i];
		}
	};
};

} // namespace vram
