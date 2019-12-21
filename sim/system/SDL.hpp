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

struct device
{
	using unique_SDL_Renderer = std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;
	unique_SDL_Renderer renderer{nullptr, SDL_DestroyRenderer};

	using unique_SDL_Window = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;
	unique_SDL_Window window{nullptr, SDL_DestroyWindow};

	device()
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

	~device()
	{
		SDL_Quit();
	}
};

template <typename Applier>
struct io_reg
{
	uint16_t value;

	auto& operator= (uint16_t const raw) {
		value = raw;
		Applier::apply(value);
		return *this;
	}

	auto& operator|= (uint16_t const raw) {
		value |= raw;
		Applier::apply(value);
		return *this;
	}
};

struct DispControl : public io_reg<DispControl>
{
	using io_reg<DispControl>::operator=;
	using io_reg<DispControl>::operator|=;
	static inline void apply(uint16_t const raw) {

	}
};
static inline auto dispControl = DispControl{0};

struct DispStat : public io_reg<DispStat>
{
	using io_reg<DispStat>::operator=;
	using io_reg<DispStat>::operator|=;
	static inline void apply(uint16_t const raw) {

	}
};
static inline auto dispStat = DispStat{0};

struct BgControl : public io_reg<BgControl>
{
	using io_reg<BgControl>::operator=;
	using io_reg<BgControl>::operator|=;
	static inline void apply(uint16_t const raw) {

	}
};
static inline auto bg2Control = BgControl{0};

static constexpr auto bg2aff = vram::affine::param::storage{};
static constexpr auto bg2P   = vram::affine::P::storage{};
static constexpr auto bg2dx  = vram::affine::dx::storage{};

static inline constexpr auto dma3Source  = reg::write_only<void const*, 0x040000D4>{};
static inline constexpr auto dma3Dest    = reg::write_only<void*, 0x040000D8>{};
static inline constexpr auto dma3Control = reg::write_only<uint32_t, 0x040000DC>{};

static inline constexpr auto biosIrqsRaised    = reg::read_write<uint16_t, 0x03fffff8>{};
static inline constexpr auto irqServiceRoutine = reg::write_only<void(*)(void), 0x03fffffc>{};
static inline constexpr auto irqsEnabled       = reg::read_write<uint16_t, 0x04000200>{};
static inline constexpr auto irqsRaised        = reg::read_write<uint16_t, 0x04000202>{};
static inline constexpr auto irqsEnabledFlag   = reg::read_write<uint16_t, 0x04000208>{};

static inline auto palBanks = vram::pal_banks::storage{};

static inline auto screenBlocks = vram::screen_blocks::storage{};
static inline auto charBlocks   = vram::char_blocks::storage{};

static inline void VBlankIntrWait() {

}

static inline constexpr void pump_events(event::queue_type& queue) {
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
