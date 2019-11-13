#include <array>

template <typename T, size_t address> 
class read_write
{
private:
	static volatile inline T& ref = *reinterpret_cast<T*>(address);

public:
	constexpr static auto& Get() {
		return ref;
	}

	constexpr static void Set(T const& val) {
		ref = val;
	}

	template <typename U>
	constexpr operator U() const {
		return U{ref};
	}

	constexpr auto& operator=(T const& val) const {
		ref = val;
		return *this;
	}

	constexpr auto& operator |= (T const& val) const {
		ref |= val;
		return *this;
	}
};

template <typename T, size_t address> 
class write_only
{
private:
	static volatile inline T& ref = *reinterpret_cast<T*>(address);

public:
	constexpr static void Set(T const& val) {
		ref = val;
	}

	constexpr auto& operator=(T const& val) const {
		ref = val;
		return *this;
	}
};

template <typename T>
struct Bitfield
{
	constexpr T& raw() {
		return *reinterpret_cast<T*>(this);
	}

	constexpr T const& raw() const {
		return *reinterpret_cast<T const*>(this);
	}

	constexpr T volatile& raw() volatile {
		return *reinterpret_cast<T volatile*>(this);
	}
};

struct BgCnt : public Bitfield<uint16_t>
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
		{ reg32x32 = 0
		, reg64x32 = 1
		, reg32x64 = 2
		, reg64x64 = 3

		, aff16x16 = 0
		, aff32x32 = 1
		, aff64x64 = 2
		, aff128x128 = 3
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

struct ScreenEntry : Bitfield<uint16_t> {
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

template <size_t address> 
class screen_blocks {
	using ScreenBlock = uint16_t[0x400];
	static inline auto base = reinterpret_cast<ScreenBlock volatile*>(address);

public:
	constexpr auto& operator[] (size_t i) const {
		return base[i];
	}
};

struct CharEntry {
	uint32_t buf[8];

public:
	constexpr CharEntry(std::initializer_list<uint32_t> lines)
		: buf{lines.begin()[0], lines.begin()[1], lines.begin()[2], lines.begin()[3], lines.begin()[4], lines.begin()[5], lines.begin()[6], lines.begin()[7]}
	{}

	auto& operator= (CharEntry const& rhs) volatile {
		std::copy(rhs.buf, rhs.buf + 8, buf);
		return *this;
	}
};

template <size_t address> 
class char_blocks {
	using CharBlock = CharEntry[0x200];
	static inline auto base = reinterpret_cast<CharBlock volatile*>(address);

public:
	constexpr auto& operator[] (size_t i) const {
		return base[i];
	}
};

struct Rgb15 {
	constexpr static uint16_t make(uint16_t r, uint16_t g, uint16_t b) {
		return ((r & 0x1f) << (5 * 0))
		     | ((g & 0x1f) << (5 * 1))
		     | ((b & 0x1f) << (5 * 2));
	}
};

template <size_t address> 
struct pal_banks {
	using PalBank = uint16_t[0x10];
	static inline auto base = reinterpret_cast<PalBank volatile*>(address);

public:
	constexpr auto& operator[] (size_t i) const {
		return base[i];
	}
};

static constexpr auto dispCntReg_addr = size_t{0x04000000};
auto dispCntReg = read_write<uint16_t, dispCntReg_addr>{};

static constexpr auto bg0CntReg_addr = size_t{0x04000008};
auto bg0Cnt = read_write<uint16_t, bg0CntReg_addr>{};

static constexpr auto bg0horzOffsReg_addr = size_t{0x04000010};
auto bg0HorzOffsReg = write_only<uint32_t, bg0horzOffsReg_addr>{};

static constexpr auto vidMem_addr = size_t{0x06000000};
auto screenBlocks = screen_blocks<vidMem_addr>{};
auto charBlocks   = char_blocks<vidMem_addr>{};

static constexpr auto palMem_addr = size_t{0x05000000};
auto palBanks = pal_banks<palMem_addr>{};

static constexpr auto tiles = std::array<CharEntry, 2> {
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

static constexpr auto Red   = Rgb15::make(31,  0,  0);
static constexpr auto Blue  = Rgb15::make( 0, 31,  0);
static constexpr auto Green = Rgb15::make( 0,  0, 31);
static constexpr auto Grey  = Rgb15::make(15, 15, 15);

int main(int argc, char const* argv[])
{
	dispCntReg = 0x1100;

	auto constexpr charBlockBase = 0;
	auto constexpr screenBlockBase = 28;

	bg0Cnt = BgCnt
		{ .priority = 0
		, .charBlockBase = charBlockBase
		, .mosaicEnabled = false
		, .palMode = BgCnt::PalMode::Bits4
		, .screenBlockBase = screenBlockBase
		, .affineWrapEnabled = false
		, .mapSize = BgCnt::MapSize::reg64x64
	};

	auto offs = uint32_t{0x4};
	bg0HorzOffsReg = offs;
	offs++;
	bg0HorzOffsReg = offs;

	palBanks[0][1] = Red;
	palBanks[1][1] = Blue;
	palBanks[2][1] = Green;
	palBanks[3][1] = Grey;

	charBlocks[0][0] = tiles[0];
	charBlocks[0][1] = tiles[1];

	for (uint32_t screenBlockOffs = 0; screenBlockOffs < 4; screenBlockOffs++) {
		for (uint32_t screenEntryIdx = 0; screenEntryIdx < 32 * 32; screenEntryIdx++) {
			screenBlocks[screenBlockBase + screenBlockOffs][screenEntryIdx] = ScreenEntry
				{ .tileId = 0
				, .horzFlip = 0
				, .vertFlip = 0
				, .palBank = (uint16_t)screenBlockOffs
			};
		}
	}

	bg0Cnt |= BgCnt{ .affineWrapEnabled = true };

	screenBlocks[screenBlockBase][0] |= ScreenEntry{ .tileId = 0x1 };

	while (true) {}

	return 0;
}
