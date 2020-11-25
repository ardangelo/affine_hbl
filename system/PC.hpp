#pragma once

#include <memory>

#include "register.hpp"
#include "vram.hpp"
#include "event.hpp"

template <typename DrawImpl_>
class PC
{
public: // types
	using DrawImpl = DrawImpl_;

	class DMA
	{
	public: // members
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

	public: // interface
		void runTransfer();
	};

private: // helpers

	// Called by PC::VBlankIntrWait implementation
	static void runVblank();

	// Called after Vblank interrupts in runVblank
	static void runRender();

	// Called after each line rendered in runRender
	static void runHblank();

public: // static members

	static inline constexpr auto screenWidth  = 240;
	static inline constexpr auto screenHeight = 160;

	static inline auto dispControl = uint16_t{};
	static inline auto dispStat = uint16_t{};

	static inline auto bg2Control = vram::bg_control{};

	static inline auto  bg2aff = vram::affine::param::storage{};
	static inline auto& bg2P   = bg2aff.P;
	static inline auto& bg2dx  = bg2aff.dx;

	static inline auto  dma3 = DMA{};
	static inline auto& dma3Dest    = dma3.dest;
	static inline auto& dma3Source  = dma3.source;
	static inline auto& dma3Control = dma3.control;

	static inline auto biosIrqsRaised    = vram::overlay<vram::interrupt_mask, uint16_t>{};
	static inline auto irqServiceRoutine = (void(*)(void)){nullptr};
	static inline auto irqsEnabled       = vram::overlay<vram::interrupt_mask, uint16_t>{};
	static inline auto irqsRaised        = vram::overlay<vram::interrupt_mask, uint16_t>{};
	static inline auto irqsEnabledFlag   = vram::overlay<vram::interrupt_master, uint32_t>{};

	static inline auto palBank = vram::pal_bank::storage{};

	static inline auto screenBlocks = vram::screen_blocks::storage{};
	static inline auto charBlocks   = vram::char_blocks::storage{};

public: // sys interface

	static void VBlankIntrWait()
	{
		runVblank();

		DrawImpl::waitNextFrame();
	}

	static void PumpEvents(event::queue_type& queue)
	{
		DrawImpl::pumpEvents(queue);
	}

}; // struct PC

template <typename DrawImpl>
void PC<DrawImpl>::DMA::runTransfer()
{
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

template <typename DrawImpl>
void PC<DrawImpl>::runRender()
{
	auto const getMapHeight = [](uint16_t dispControl, vram::bg_control::MapSize const mapSize) {
		return 64;
	};

	auto const getMapWidth = [](uint16_t dispControl, vram::bg_control::MapSize const mapSize) {
		return 64;
	};

	auto const sbb = bg2Control.screenBlockBase;
	auto const cbb = bg2Control.charBlockBase;

	auto const getColorIdx = [&](int colPxScreen, int rowPxScreen) {
		auto const applyAffineTxX = [](auto const& P, auto const& dx, int h, int colPx, int rowPx) {
			auto const colPxP = (P[0] * colPx + P[1] * (rowPx - 0) + dx[0]) >> 8;
			return colPxP;
		};
		auto const applyAffineTxY = [](auto const& P, auto const& dx, int h, int colPx, int rowPx) {
			// TODO: determine if the scanline offset should be performed implicitly in the simulated hblank interrupt
			auto const rowPxP = (P[2] * colPx + P[3] * (rowPx - h) + dx[1]) >> 8;
			return rowPxP;
		};

		auto const mapWidth = getMapWidth(dispControl, (vram::bg_control::MapSize)bg2Control.mapSize);
		auto const mapHeight = getMapHeight(dispControl, (vram::bg_control::MapSize)bg2Control.mapSize);

		auto const mod = [](auto a, auto b) {
			auto const c = a % b; return (c < 0) ? c + b : c;
		};

		auto const colPxIn = mod(colPxScreen, 8 * mapWidth);
		auto const rowPxIn = mod(rowPxScreen, 8 * mapHeight);

		auto const colPx = mod(applyAffineTxX(bg2P, bg2dx, rowPxScreen, colPxIn, rowPxIn), 8 * mapWidth);
		auto const rowPx = mod(applyAffineTxY(bg2P, bg2dx, rowPxScreen, colPxIn, rowPxIn), 8 * mapHeight);

		// fprintf(stderr, "(%3d, %3d) -> (%3d, %3d)\n", colPxIn, rowPxIn, colPx, rowPx);

		auto const colTile     = colPx / 8;
		auto const colTileOffs = colPx % 8;

		auto const rowTile     = rowPx / 8;
		auto const rowTileOffs = rowPx % 8;

		auto const screenEntryIdx = colTile + (rowTile * mapWidth);
		auto const screenEntry = ((uint8_t*)&screenBlocks[sbb])[screenEntryIdx];

		auto const charBlock = charBlocks[cbb][screenEntry];

		auto const px = colTileOffs + (rowTileOffs * 8);
		auto const colorIdx = ((uint8_t*)charBlock.buf)[px];

		return colorIdx;
	};

	for (int rowPx = 0; rowPx < screenHeight; rowPx++) {

		for (int colPx = 0; colPx < screenWidth; colPx++) {

			auto const colorIdx = getColorIdx(colPx, rowPx);
			auto const color = (colorIdx > 0)
				? palBank[colorIdx]
				: 0;
			auto const r = (color >>  0) & 0x1f;
			auto const g = (color >>  5) & 0x1f;
			auto const b = (color >> 10) & 0x1f;

			DrawImpl::drawPx(colPx, rowPx, r, g, b);
		}

		runHblank();
	}

	DrawImpl::updateDisplay();
}

template <typename DrawImpl>
void PC<DrawImpl>::runHblank()
{
	static constexpr auto hblankEnabledDmaMask = vram::dma_control
		{ .timing = vram::dma_control::Timing::Hblank
		, .enabled = true
	};

	if (dma3.control.Get() & hblankEnabledDmaMask) {
		dma3.runTransfer();
	}

	// TODO: determine if this is the right blank repeat behavior
	if (dma3.control.Get().repeatAtBlank == false) {
		dma3.control.Get().enabled = false;
	}
}

template <typename DrawImpl>
void PC<DrawImpl>::runVblank()
{
	constexpr auto vblankInterruptMask = vram::interrupt_mask{ .vblank = 1 };
	if (irqsEnabledFlag) {
		if (irqsEnabled & vblankInterruptMask) {
			irqsRaised->vblank = 1;
			irqServiceRoutine();
			biosIrqsRaised->vblank = 0;
			irqsRaised->vblank = 0;
		}
	}

	runRender();
}