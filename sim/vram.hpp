#pragma once

#include <array>

namespace vram
{

template <typename Base, size_t address>
struct memmap {
	static inline auto base = reinterpret_cast<typename Base::element volatile*>(address);

	constexpr auto& operator[] (size_t i) const {
		return base[i];
	}
};

struct bg_control
{
	uint16_t priority : 2;
	uint16_t charBlockBase : 2;

	uint16_t _ : 2;

	uint16_t mosaicEnabled : 1;

	enum PalMode : uint16_t
		{ Bits4 = 0
		, Bits8 = 1

		, Color16  = 0
		, Color256 = 1
	};
	uint16_t palMode : 1;

	uint16_t screenBlockBase : 5;
	uint16_t affineWrapEnabled : 1;

	enum MapSize : uint16_t
		{ Reg32x32 = 0
		, Reg64x32 = 1
		, Reg32x64 = 2
		, Reg64x64 = 3

		, Aff16x16 = 0
		, Aff32x32 = 1
		, Aff64x64 = 2
		, Aff128x128 = 3
	};
	uint16_t mapSize : 2;

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

struct screen_entry {
	uint8_t buf[4];

	constexpr screen_entry() : buf{} {}

	constexpr screen_entry(std::initializer_list<uint8_t> indices)
		: buf
			{ indices.begin()[0], indices.begin()[1]
			, indices.begin()[2], indices.begin()[3]
		}
	{}

	constexpr operator uint32_t() const {
		return (buf[0] <<  0) | (buf[1] <<  8)
			 | (buf[2] << 16) | (buf[3] << 24);
	}
} __attribute__((packed));
static_assert(sizeof(screen_entry) == 4);

struct screen_blocks {
	using element = uint32_t[0x200];
	using storage = element[0x20];
};

struct char_entry {
	uint32_t buf[16];

public:
	constexpr char_entry() : buf{} {}

	constexpr char_entry(std::initializer_list<uint32_t> lines)
		: buf
			{ lines.begin()[ 0], lines.begin()[ 1], lines.begin()[ 2], lines.begin()[ 3]
			, lines.begin()[ 4], lines.begin()[ 5], lines.begin()[ 6], lines.begin()[ 7]
			, lines.begin()[ 8], lines.begin()[ 9], lines.begin()[10], lines.begin()[11]
			, lines.begin()[12], lines.begin()[13], lines.begin()[14], lines.begin()[15]
		}
	{}

	void operator= (char_entry const& rhs) volatile {
		std::copy(rhs.buf, rhs.buf + 16, buf);
	}
};

struct char_blocks {
	using element = char_entry[0x100];
	using storage = element[0x4];
};


namespace affine {

struct P {
	using element = uint16_t;
	using storage = element[4];
};

struct dx {
	using element = uint32_t;
	using storage = element[2];
};

}

struct rgb15 {
	constexpr static uint16_t make(uint16_t r, uint16_t g, uint16_t b) {
		return ((r & 0x1f) << (5 * 0))
		     | ((g & 0x1f) << (5 * 1))
		     | ((b & 0x1f) << (5 * 2));
	}
};

struct pal_banks {
	using element = uint16_t;
	using storage = element[0x100];
};

struct interrupt_mask
{
	uint16_t vblank : 1;
	uint16_t hblank : 1;
	uint16_t vcount : 1;
	uint16_t timer0 : 1;
	uint16_t timer1 : 1;
	uint16_t timer2 : 1;
	uint16_t timer3 : 1;
	uint16_t serialComm : 1;
	uint16_t dma0 : 1;
	uint16_t dma1 : 1;
	uint16_t dma2 : 1;
	uint16_t dma3 : 1;
	uint16_t keypad : 1;
	uint16_t gamePak : 1;
	uint16_t _ : 2;

	constexpr operator uint16_t() const {
		return (vblank << 0)
			 | (hblank << 1)
			 | (vcount << 2)
			 | (timer0 << 3)
			 | (timer1 << 4)
			 | (timer2 << 5)
			 | (timer3 << 6)
			 | (serialComm << 7)
			 | (dma0 <<  8)
			 | (dma1 <<  9)
			 | (dma2 << 10)
			 | (dma3 << 11)
			 | (gamePak << 12);
	}
} __attribute__((packed));

} // namespace vram
