#pragma once

#include "register.hpp"
#include "vram.hpp"
#include "event.hpp"

struct GBA
{

static inline constexpr auto dispCnt = reg::read_write<uint16_t, 0x04000000>{};

static inline constexpr auto bg0Cnt      = reg::read_write<uint16_t, 0x04000008>{};
static inline constexpr auto bg0HorzOffs = reg::write_only<uint32_t, 0x04000010>{};

static inline constexpr auto palBanks = vram::pal_banks::memmap<0x05000000>{};

static inline constexpr auto screenBlocks = vram::screen_blocks::memmap<0x06000000>{};
static inline constexpr auto charBlocks   = vram::char_blocks::memmap<0x06000000>{};

static inline constexpr void pump_events(event::queue_type& queue) {
	queue.push_back(event::Key
		{ .type  = event::Key::Type::Right
		, .state = event::Key::State::Down
	});
}

static inline constexpr auto vcount = reg::read_only<uint16_t, 0x04000006>{};

static inline constexpr uint32_t GetTime() { return uint16_t{vcount} / 2; }

}; // struct GBA

struct SDL
{

static inline auto dispCnt = val::read_write<uint16_t>{};

static inline auto bg0Cnt      = val::read_write<uint16_t>{};
static inline auto bg0HorzOffs = val::write_only<uint32_t>{};

static inline vram::pal_banks::storage palBanks = {};

static inline vram::screen_blocks::storage screenBlocks = {};
static inline vram::char_blocks::storage   charBlocks = {};

}; // struct SDL
