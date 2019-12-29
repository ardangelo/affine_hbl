#include "SDL.hpp"

LibSDLState::LibSDLState()
{
	SDL_Init(SDL_INIT_VIDEO);

	{ SDL_Renderer *rawRenderer = nullptr;
	  SDL_Window   *rawWindow   = nullptr;

		SDL_CreateWindowAndRenderer(SDL::screenWidth, SDL::screenHeight, 0, &rawWindow, &rawRenderer);
		renderer = unique_SDL_Renderer{rawRenderer, SDL_DestroyRenderer};
		window   = unique_SDL_Window{rawWindow, SDL_DestroyWindow};
	}

	SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 0);
	SDL_RenderClear(renderer.get());
}

LibSDLState::~LibSDLState()
{
	SDL_Quit();
}

void SDL::DMA::runTransfer()
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

void SDL::runRender()
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

			SDL_SetRenderDrawColor(libSDLState.renderer.get(), 8 * r, 8 * g, 8 * b, 0xff);
			SDL_RenderDrawPoint(libSDLState.renderer.get(), colPx, rowPx);
		}

		runHblank();
	}

	SDL_RenderPresent(libSDLState.renderer.get());
}

void SDL::runHblank()
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

void SDL::runVblank()
{
	constexpr auto vblankInterruptMask = vram::interrupt_mask{ .vblank = 1 };
	if (irqsEnabledFlag) {
		if (irqsEnabled & vblankInterruptMask) {
			irqsRaised.vblank = 1;
			irqServiceRoutine();
			biosIrqsRaised.vblank = 0;
			irqsRaised.vblank = 0;
		}
	}

	runRender();
}

void SDL::VBlankIntrWait()
{
	runVblank();
}

void SDL::PumpEvents(event::queue_type& queue)
{
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
