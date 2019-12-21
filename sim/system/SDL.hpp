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

struct DispControl : public io_val<DispControl, uint16_t>
{
	using io_base = io_val<DispControl, uint16_t>;
	using io_base::operator=;
	using io_base::operator|=;
	static inline void apply(uint16_t const raw) {

	}
};
static inline auto dispControl = DispControl{0};

struct DispStat : public io_val<DispStat, uint16_t>
{
	using io_base = io_val<DispStat, uint16_t>;
	using io_base::operator=;
	using io_base::operator|=;
	static inline void apply(uint16_t const raw) {

	}
};
static inline auto dispStat = DispStat{0};

struct BgControl : public io_val<BgControl, uint16_t>
{
	using io_base = io_val<BgControl, uint16_t>;
	using io_base::operator=;
	using io_base::operator|=;
	static inline void apply(uint16_t const raw) {

	}
};
static inline auto bg2Control = BgControl{0};

static constexpr auto bg2aff = vram::affine::param::storage{};
static constexpr auto bg2P   = vram::affine::P::storage{};
static constexpr auto bg2dx  = vram::affine::dx::storage{};

struct DMA
{
	struct Source : public io_val<Source, void const*> {
		using io_base = io_val<Source, void const*>;
		using io_base::operator=;
		static inline void apply(void const* src) {}
	};

	struct Dest : public io_val<Dest, void*> {
		using io_base = io_val<Dest, void*>;
		using io_base::operator=;
		static inline void apply(void* dest) {}
	};

	struct Control : public io_val<Control, vram::dma_control> {
		using io_base = io_val<Control, vram::dma_control>;
		using io_base::operator=;
		using io_base::operator|=;
		static inline void apply(vram::dma_control const raw) {}
	};
};

static inline auto dma3Source = DMA::Source{nullptr};
static inline auto dma3Dest   = DMA::Dest{nullptr};
static inline auto dma3Control= DMA::Control{};

struct IRQ
{
	struct BiosIrqsRaised : public io_val<BiosIrqsRaised, vram::interrupt_mask> {
		using io_base = io_val<BiosIrqsRaised, vram::interrupt_mask>;
		using io_base::operator=;
		using io_base::operator|=;
		static inline void apply(vram::interrupt_mask const mask) {}
	};

	struct IrqServiceRoutine : public io_val<IrqServiceRoutine, void(*)(void)> {
		using Func = void(*)(void);
		using io_base = io_val<IrqServiceRoutine, Func>;
		using io_base::operator=;
		static inline void apply(Func func) {}
	};

	struct IrqsEnabled : public io_val<IrqsEnabled, vram::interrupt_mask> {
		using io_base = io_val<IrqsEnabled, vram::interrupt_mask>;
		using io_base::operator=;
		using io_base::operator|=;
		static inline void apply(vram::interrupt_mask const mask) {}
	};

	struct IrqsRaised : public io_val<IrqsRaised, vram::interrupt_mask> {
		using io_base = io_val<IrqsRaised, vram::interrupt_mask>;
		using io_base::operator=;
		using io_base::operator|=;
		static inline void apply(vram::interrupt_mask const mask) {}
	};

	struct IrqsEnabledFlag : public io_val<IrqsEnabledFlag, uint16_t> {
		using io_base = io_val<IrqsEnabledFlag, uint16_t>;
		using io_base::operator=;
		using io_base::operator|=;
		static inline void apply(uint16_t const flag) {}
	};
};

static inline auto biosIrqsRaised    = IRQ::BiosIrqsRaised{0};
static inline auto irqServiceRoutine = IRQ::IrqServiceRoutine{nullptr};
static inline auto irqsEnabled       = IRQ::IrqsEnabled{0};
static inline auto irqsRaised        = IRQ::IrqsRaised{0};
static inline auto irqsEnabledFlag   = IRQ::IrqsEnabledFlag{false};

static inline auto palBanks = vram::pal_banks::storage{};

static inline auto screenBlocks = vram::screen_blocks::storage{};
static inline auto charBlocks   = vram::char_blocks::storage{};

static inline void VBlankIntrWait() {
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
