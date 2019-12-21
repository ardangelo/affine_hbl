#include <assert.h>

#include <SDL.h>

#include <array>

static constexpr auto bpp = 8;
static constexpr auto pitch = bpp * 8;
using CharEntry = std::array<uint32_t, bpp>;

struct ScreenEntry
{
	uint16_t tileId : 10;
	uint16_t horFlip    :  1;
	uint16_t verFlip    :  1;
	uint16_t palBank :  4;
} __attribute__((packed));
static_assert(sizeof(ScreenEntry) == 2);

static constexpr auto charEntries = 0x4000 / sizeof(CharEntry);
using CharBlock = std::array<CharEntry, charEntries>;

static constexpr auto screenBlockHeight = 32;
static constexpr auto screenBlockWidth  = 32;
static constexpr auto screenEntries = 0x800 / sizeof(ScreenEntry);
using ScreenBlock = std::array<ScreenEntry, screenEntries>;

static constexpr auto tiles = std::array<CharEntry, 2>
{ CharEntry
	{ 0x11111111
	, 0x01111111
	, 0x01111111
	, 0x01111111
	, 0x01111111
	, 0x01111111
	, 0x01111111
	, 0x00000001
	}
, CharEntry
	{ 0x00000000, 0x00100100, 0x01100110, 0x00011000
	, 0x00011000, 0x01100110, 0x00100100, 0x00000000
	}
};

struct Rgb15
{
	using Channel = uint16_t;

	Channel _ : 1;
	Channel r : 5;
	Channel g : 5;
	Channel b : 5;

	constexpr Rgb15() : _{0}, r{0}, g{0}, b{0} {}

	constexpr Rgb15(Channel r_, Channel g_, Channel b_) : _{0}, r{r_}, g{g_}, b{b_} {}

} __attribute__((packed));
static_assert(sizeof(Rgb15) == 2);

using BgPal = std::array<Rgb15, 16>;

static auto bgPalBanks = std::array<BgPal, 16>{};

struct Bg64x64
{
	uint32_t horzOffs, vertOffs;

	std::array<CharBlock,   1> charBlocks;
	std::array<ScreenBlock, 4> screenBlocks;
	
	static auto charBlockIdx(auto tileId) {
		return (tileId / charEntries);
	}

	static auto charEntryIdx(auto tileId) {
		return (tileId % charEntries);
	}

	static auto screenBlockIdx(auto tx, auto ty) {
		return (ty / screenBlockHeight) * (pitch / 32) + (tx / screenBlockWidth);
	}

	static auto screenEntryIdx(auto tx, auto ty) {
		return (ty % screenBlockHeight) * screenBlockHeight
		     + (tx % screenBlockWidth);
	}
};


int
#ifdef _WIN32
WinMain
#else
main
#endif
	(int argc, char const* argv[]) {

	auto bg0 = Bg64x64{};

	bg0.horzOffs = 0;
	bg0.vertOffs = 0;

	bg0.charBlocks[0][0] = tiles[0];
	bg0.charBlocks[0][1] = tiles[1];
	
	bgPalBanks[0][1] = Rgb15{31,  0,  0};
	bgPalBanks[1][1] = Rgb15{ 0, 31,  0};
	bgPalBanks[2][1] = Rgb15{ 0,  0, 31};
	bgPalBanks[3][1] = Rgb15{16, 16, 16};

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < screenBlockHeight * screenBlockWidth; j++) {
			bg0.screenBlocks[i][j] = ScreenEntry
				{ .tileId = 0
				, .horFlip = 0
				, .verFlip = 0
				, .palBank = (uint8_t)i
				};
		}
	}
	
	auto const screenBlockIdx = Bg64x64::screenBlockIdx(0, 0);
	auto const screenEntryIdx = Bg64x64::screenEntryIdx(0, 0);
	bg0.screenBlocks[screenBlockIdx][screenEntryIdx].tileId = 1;

	SDL_Init(SDL_INIT_VIDEO);

	SDL_Event event;
	SDL_Renderer *renderer;
	SDL_Window *window;

	SDL_CreateWindowAndRenderer(240, 160, 0, &window, &renderer);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);

	for (int i= 0; i < 160; i++) {
		auto const rowPx = bg0.vertOffs + i;

		auto const rowTx     = rowPx / 8;
		auto const rowTxOffs = rowPx % 8;

		for (int j = 0; j < 240; j++) {
			auto const colPx = bg0.horzOffs + j;

			auto const colTx     = colPx / 8;
			auto const colTxOffs = colPx % 8;
			
			fprintf(stderr, "row px %lu tx %lu offs %lu, col px %lu tx %lu offs %lu\n", rowPx, rowTx, rowTxOffs, colPx, colTx, colTxOffs);

			auto const screenBlockIdx = Bg64x64::screenBlockIdx(colTx, rowTx);
			assert(screenBlockIdx < 4);
			auto const screenEntryIdx = Bg64x64::screenEntryIdx(colTx, rowTx);
			auto const screenEntry = bg0.screenBlocks[screenBlockIdx][screenEntryIdx];
			assert(screenEntryIdx < screenEntries);
			assert(screenEntry.tileId <= 1);
			assert(screenEntry.palBank < 4);

			auto const charBlockIdx = Bg64x64::charBlockIdx(screenEntry.tileId);
			assert(charBlockIdx == 0);
			auto const charEntryIdx = Bg64x64::charEntryIdx(screenEntry.tileId);
			assert(charEntryIdx <= 1);
			auto const tile = bg0.charBlocks[charBlockIdx][charEntryIdx];
			fprintf(stderr, "charBlockIdx %lu charEntryIdx %lu\n", charBlockIdx, charEntryIdx);

			auto const palBank = bgPalBanks[screenEntry.palBank];

			auto const wordIdx = (rowTxOffs * bpp) / 8;
			assert(wordIdx < bpp);

			static_assert(bpp == 8);
			auto const word = tile[wordIdx];
			auto const nibIdx = colTxOffs;
			auto const colorIdx = (word >> (nibIdx * 4)) & 0xf;
			fprintf(stderr, "word %08x nibIdx %lu, colorIdx %lu\n", word, nibIdx, colorIdx);
			assert(colorIdx <= 1);
			fprintf(stderr, "wordIdx %lu word %08x colorIdx %lu\n", wordIdx, word, colorIdx);

			if (colorIdx > 0) {
				auto const color = palBank[colorIdx];

				fprintf(stderr, "SDL_SetRenderDrawColor %04x - %d, %d, %d\n", *(uint16_t*)&color, 8 * int{color.r}, 8 * int{color.g}, 8 * int{color.b});
				SDL_SetRenderDrawColor(renderer, 8 * color.r, 8 * color.g, 8 * color.b, 0xff);
				SDL_RenderDrawPoint(renderer, colPx, rowPx);
			}

			fprintf(stderr, "\n");
		}
	}

	SDL_RenderPresent(renderer);
	while (true) {
		if (SDL_PollEvent(&event) && event.type == SDL_QUIT) {
			break;
		}
	}
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
