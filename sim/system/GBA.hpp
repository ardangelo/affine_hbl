#pragma once

// #include <tonc.h>

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
static inline constexpr auto screenHeight = 160;

static inline constexpr auto dispControl  = reg::read_write<uint16_t, 0x04000000>{};
static inline constexpr auto dispStat     = reg::read_write<uint16_t, 0x04000004>{};

static inline constexpr auto bg2Control = reg::read_write<uint16_t, 0x0400000c>{};

static inline constexpr auto bg2aff = vram::memmap<vram::affine::param, 0x04000020>{};
static inline constexpr auto bg2P   = vram::memmap<vram::affine::P,     0x04000020>{};
static inline constexpr auto bg2dx  = vram::memmap<vram::affine::dx,    0x04000028>{};

static inline constexpr auto dma3Source  = reg::write_only<void const*, 0x040000D4>{};
static inline constexpr auto dma3Dest    = reg::write_only<void*, 0x040000D8>{};
static inline constexpr auto dma3Control = reg::write_only<uint32_t, 0x040000DC>{};

static inline constexpr auto biosIrqsRaised    = reg::read_write<uint16_t, 0x03fffff8>{};
static inline constexpr auto irqServiceRoutine = reg::write_only<void(*)(void), 0x03fffffc>{};
static inline constexpr auto irqsEnabled       = reg::read_write<uint16_t, 0x04000200>{};
static inline constexpr auto irqsRaised        = reg::read_write<uint16_t, 0x04000202>{};
static inline constexpr auto irqsEnabledFlag   = reg::read_write<uint16_t, 0x04000208>{};

static inline constexpr auto palBanks = vram::memmap<vram::pal_banks, 0x05000000>{};

static inline constexpr auto screenBlocks = vram::memmap<vram::screen_blocks, 0x06000000>{};
static inline constexpr auto charBlocks   = vram::memmap<vram::char_blocks,   0x06000000>{};

static inline void VBlankIntrWait() {
	swi_call(0x05);
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

}; // struct GBA
