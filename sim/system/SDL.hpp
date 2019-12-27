#pragma once

#include <memory>

#include <SDL.h>

#include "register.hpp"
#include "vram.hpp"
#include "event.hpp"

struct SDL
{

static inline constexpr auto screenWidth  = 240;
static inline constexpr auto screenHeight = 160;

struct SDLState
{
	using unique_SDL_Renderer = std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;
	unique_SDL_Renderer renderer{nullptr, SDL_DestroyRenderer};

	using unique_SDL_Window = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;
	unique_SDL_Window window{nullptr, SDL_DestroyWindow};

	SDLState()
	{
		SDL_Init(SDL_INIT_VIDEO);

		{ SDL_Renderer *rawRenderer = nullptr;
		  SDL_Window   *rawWindow   = nullptr;

			SDL_CreateWindowAndRenderer(screenWidth, screenHeight, 0, &rawWindow, &rawRenderer);
			renderer = unique_SDL_Renderer{rawRenderer, SDL_DestroyRenderer};
			window   = unique_SDL_Window{rawWindow, SDL_DestroyWindow};
		}

		SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 0);
		SDL_RenderClear(renderer.get());
	}

	~SDLState()
	{
		SDL_Quit();
	}
};
static inline auto sdlState = SDLState{};

static inline auto dispControl = uint16_t{0};
static inline auto dispStat = uint16_t{};

static inline auto bg2Control = vram::bg_control{};

static inline auto  bg2aff = vram::affine::param::storage{};
static inline auto& bg2P   = bg2aff.P;
static inline auto& bg2dx  = bg2aff.dx;

struct DMA
{
	using Source = void const*;
	Source source;

	using Dest = void*;
	Dest dest;

	struct Control : public io_val<Control, vram::dma_control> {
		using io_base = io_val<Control, vram::dma_control>;
		using io_base::operator=;
		using io_base::operator|=;
		static inline void apply(vram::dma_control const raw) {}
	} control;

	void runTransfer() {
		auto const originalDest = dest;

		auto const adjustPointer = [](auto const adj, auto const pointer, auto const chunkSize) {
			using Ad = vram::dma_control::Adjust;

			switch ((Ad)adj) {
			case Ad::Increment: return pointer + chunkSize;
			case Ad::Decrement: return pointer - chunkSize;
			case Ad::None:      return pointer + 0;
			case Ad::Reload:    return pointer + chunkSize; // Not valid for source
			default: std::terminate();
			}
		};

		for (uint32_t i = 0; i < control.Get().count; i++) {
			auto const chunkSize = (control.Get().chunkSize == vram::dma_control::ChunkSize::Bits16)
				? 2
				: 4;

			for (int i = 0; i < chunkSize; i++) {
				((char*)dest)[i] = ((char const*)source)[i];
			}

			dest   = adjustPointer(control.Get().destAdjust,   (char*)dest,   chunkSize);
			source = adjustPointer(control.Get().sourceAdjust, (char const*)source, chunkSize);
		}

		if (control.Get().destAdjust == vram::dma_control::Adjust::Reload) {
			dest = originalDest;
		}

		// TODO: DMA IRQ
	}

	static constexpr auto hblankEnabledDmaMask = vram::dma_control
		{ .timing = vram::dma_control::Timing::Hblank
		, .enabled = true
	};
	void TryRunHblank() {
		if (control.Get() & hblankEnabledDmaMask) {
			runTransfer();
		}

		// TODO: determine if this is the right blank repeat behavior
		if (control.Get().repeatAtBlank == false) {
			control.Get().enabled = false;
		}
	}
};

static inline auto dma3 = DMA{};
static inline auto& dma3Dest    = dma3.dest;
static inline auto& dma3Source  = dma3.source;
static inline auto& dma3Control = dma3.control;

static inline auto biosIrqsRaised    = vram::interrupt_mask{};
static inline auto irqServiceRoutine = (void(*)(void)){nullptr};
static inline auto irqsEnabled       = vram::interrupt_mask{};
static inline auto irqsRaised        = vram::interrupt_mask{};
static inline auto irqsEnabledFlag   = uint16_t{};

static inline auto palBank = vram::pal_bank::storage{};

static inline auto screenBlocks = vram::screen_blocks::storage{};
static inline auto charBlocks   = vram::char_blocks::storage{};

static inline void VBlankIntrWait() {
	constexpr auto vblankInterruptMask = vram::interrupt_mask{ .vblank = 1 };
	if (irqsEnabledFlag) {
		if (irqsEnabled & vblankInterruptMask) {
			irqsRaised.vblank = 1;
			irqServiceRoutine();
			biosIrqsRaised.vblank = 0;
			irqsRaised.vblank = 0;
		}
	}

	auto const getMapHeight = [](uint16_t dispControl, vram::bg_control::MapSize const mapSize) {
		return 64;
	};

	auto const getMapWidth = [](uint16_t dispControl, vram::bg_control::MapSize const mapSize) {
		return 64;
	};

	auto const sbb = bg2Control.screenBlockBase;
	auto const cbb = bg2Control.charBlockBase;

	auto const getColorIdx = [&](int rowPxScreen, int colPxScreen) {
		auto const mapWidth = getMapWidth(dispControl, (vram::bg_control::MapSize)bg2Control.mapSize);
		auto const mapHeight = getMapHeight(dispControl, (vram::bg_control::MapSize)bg2Control.mapSize);

		auto const mod = [](auto a, auto b) {
			auto const c = a % b; return (c < 0) ? c + b : c;
		};

		auto const rowPx = mod(rowPxScreen, 8 * mapHeight);
		auto const colPx = mod(colPxScreen, 8 * mapWidth);

		auto const rowTx     = rowPx / 8;
		auto const rowTxOffs = rowPx % 8;

		auto const colTx     = colPx / 8;
		auto const colTxOffs = colPx % 8;

		auto const screenEntryIdx = (rowTx * mapWidth) + colTx;
		auto const screenEntry = ((uint8_t*)&screenBlocks[sbb])[screenEntryIdx];

		auto const charBlock = charBlocks[cbb][screenEntry];

		auto const px = (rowTxOffs * 8) + colTxOffs;
		auto const colorIdx = ((uint8_t*)charBlock.buf)[px];

		return colorIdx;
	};

	for (int i = 0; i < screenHeight; i++) {
		auto const rowPx = i;

		for (int j = 0; j < screenWidth; j++) {
			auto const colPx = j - 176;

			auto const colorIdx = getColorIdx(rowPx, colPx);
			if (colorIdx > 0) {
				auto const color = palBank[colorIdx];
				auto const r = (color >>  0) & 0x1f;
				auto const g = (color >>  5) & 0x1f;
				auto const b = (color >> 10) & 0x1f;

				SDL_SetRenderDrawColor(sdlState.renderer.get(), 8 * r, 8 * g, 8 * b, 0xff);
				SDL_RenderDrawPoint(sdlState.renderer.get(), j, i);
			}
		}

		dma3.TryRunHblank();
	}

	SDL_RenderPresent(sdlState.renderer.get());
}

static inline void pump_events(event::queue_type& queue) {
	SDL_Event event;

	if (!SDL_PollEvent(&event)) {
		return;
	}

	if (event.type == SDL_QUIT) {
		exit(0);
	}

	queue.push_back(event::Key
		{ .type  = event::Key::Type::Right
		, .state = event::Key::State::On
	});
	queue.push_back(event::Key
		{ .type  = event::Key::Type::Down
		, .state = event::Key::State::On
	});
}

}; // struct SDL
