#include "Game.hpp"

static constexpr auto stopDmaControl = vram::dma_control{};
static constexpr auto affineHblankDmaControl = vram::dma_control
	{ .count = 4
	, .destAdjust    = vram::dma_control::Adjust::Reload
	, .sourceAdjust  = vram::dma_control::Adjust::Increment
	, .repeatAtBlank = true
	, .chunkSize     = vram::dma_control::ChunkSize::Bits32
	, .timing        = vram::dma_control::Timing::Hblank
	, .irqNotify     = false
	, .enabled       = true
};

namespace irq
{
namespace iwram
{

volatile uint32_t vblankCount = 0;
vram::affine::param affineParams[161];

IWRAM_CODE
void isr()
{
	auto const irqsRaised = uint16_t{sys::irqsRaised};

	if (auto const vblankMask = irqsRaised & vram::interrupt_mask{ .vblank = 1 }) {
		vblankCount++;

		sys::dma3Control = stopDmaControl;
		sys::dma3Source  = (const void*)&affineParams[1];
		sys::dma3Dest    = (void*)sys::bg2aff.begin();
		sys::dma3Control = affineHblankDmaControl;

		sys::irqsRaised     = vblankMask;
		sys::biosIrqsRaised = vblankMask;
	}
}

IWRAM_CODE
void install()
{
	sys::irqServiceRoutine = isr;
	sys::irqsEnabled = vram::interrupt_mask { .vblank = 1 };
	sys::irqsEnabledFlag = 1;
}

} // namespace iwram
} // namespace irq