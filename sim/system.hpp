#pragma once

#include "register.hpp"
#include "vram.hpp"
#include "event.hpp"

#ifdef __thumb__
#define swi_call(x)   asm volatile("swi\t"#x ::: "r0", "r1", "r2", "r3")
#else
#define swi_call(x)   asm volatile("swi\t"#x"<<16" ::: "r0", "r1", "r2", "r3")
#endif

struct GBA
{

static inline constexpr auto biosIrqsRaised    = reg::read_write<uint16_t, 0x03fffff8>{};
static inline constexpr auto irqServiceRoutine = reg::write_only<void(*)(void), 0x03fffffc>{};
static inline constexpr auto irqsEnabled       = reg::read_write<uint16_t, 0x04000200>{};
static inline constexpr auto irqsRaised        = reg::read_write<uint16_t, 0x04000202>{};
static inline constexpr auto irqsEnabledFlag   = reg::read_write<uint16_t, 0x04000208>{};

static inline constexpr auto dispCnt  = reg::read_write<uint16_t, 0x04000000>{};
static inline constexpr auto dispStat = reg::read_write<uint16_t, 0x04000004>{};

static inline constexpr auto bg2Cnt = reg::read_write<uint16_t, 0x0400000c>{};

static inline constexpr auto bg2P   = vram::memmap<vram::affine::P,  0x04000020>{};
static inline constexpr auto bg2dx  = vram::memmap<vram::affine::dx, 0x04000028>{};

static inline constexpr auto palBanks = vram::memmap<vram::pal_banks, 0x05000000>{};

static inline constexpr auto screenBlocks = vram::memmap<vram::screen_blocks, 0x06000000>{};
static inline constexpr auto charBlocks   = vram::memmap<vram::char_blocks,   0x06000000>{};

static inline void VBlankIntrWait() {
	swi_call(0x05);
}

static inline constexpr void pump_events(event::queue_type& queue) {
	queue.push_back(event::Key
		{ .type  = event::Key::Type::Right
		, .state = event::Key::State::Down
	});
}

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
