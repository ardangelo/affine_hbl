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

static inline struct SDLState
{
	using unique_SDL_Renderer = std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;
	unique_SDL_Renderer renderer{nullptr, SDL_DestroyRenderer};

	using unique_SDL_Window = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;
	unique_SDL_Window window{nullptr, SDL_DestroyWindow};

	SDLState();
	~SDLState();
} sdlState;

static inline auto dispControl = uint16_t{0};
static inline auto dispStat = uint16_t{};

static inline auto bg2Control = vram::bg_control{};

static inline auto  bg2aff = vram::affine::param::storage{};
static inline auto& bg2P   = bg2aff.P;
static inline auto& bg2dx  = bg2aff.dx;

struct DMA
{
private:
	void runTransfer();

public:
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

	void TryRunHblank();
};

static inline auto  dma3 = DMA{};
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

static void VBlankIntrWait();

static void pump_events(event::queue_type& queue);

}; // struct SDL
