#include "Game.hpp"

// Global IRQ implementation
namespace irq
{
static volatile auto vblankCount = uint32_t{0};

static vram::affine::param affineParams[161];
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

void install()
{
	sys::irqServiceRoutine = isr;
	sys::irqsEnabled = vram::interrupt_mask { .vblank = 1 };
	sys::irqsEnabledFlag = 1;
}

auto getVblankCount() { return vblankCount; }

} // namespace irq

template <typename T>
struct Responder
{
	T& target;
	Responder(T& target_) : target{target_} {}
	template <typename Arg>
	auto operator()(Arg&& var) const {
		return target.Respond(std::forward<Arg>(var));
	}
};

// struct Game

void Game::tryFillCmds()
{
	auto const [lastInputTime, inputTime] = worldCmdQueue.NextInputTimeFrame();

	for (auto tic = lastInputTime; tic < inputTime; tic++) {
		// Fill events
		sys::pump_events(event_queue);

		// Drain events
		while (!event_queue.empty()) {
			auto const ev = event_queue.pop_front();

			if (std::visit(Responder{menu}, ev)) {
				continue;
			}
			std::visit(Responder{world}, ev);
		}

		if (worldCmdQueue.full()) {
			break;
		} else {
			worldCmdQueue.push_back(world.BuildCmd());
		}
	}
}

void Game::drainCmds()
{
	while (!worldCmdQueue.empty()) {
		menu.Tick();

		auto const cmd = worldCmdQueue.pop_front();
		world.Tick(cmd);
	}
}

uint32_t Game::GetTime()
{
	return irq::getVblankCount() / 2;
}

Game::Game()
	: event_queue{}
	, worldCmdQueue{}
	, world{}
	, menu{}
{
	sys::dispControl = 0x402;
	sys::dispStat = 0x8;

	auto constexpr charBlockBase = 0;
	auto constexpr screenBlockBase = 8;

	sys::bg2Control = vram::bg_control
		{ .priority = 0
		, .charBlockBase = charBlockBase
		, .mosaicEnabled = false
		, .palMode = vram::bg_control::PalMode::Bits8
		, .screenBlockBase = screenBlockBase
		, .affineWrapEnabled = false
		, .mapSize = vram::bg_control::MapSize::Aff64x64
	};

	sys::bg2Control |= vram::bg_control{ .affineWrapEnabled = true };

	sys::palBank[1] = res::Red;
	sys::palBank[2] = res::Blue;
	sys::palBank[3] = res::Green;
	sys::palBank[4] = res::Grey;

	sys::charBlocks[0][0] = res::tiles[0];
	sys::charBlocks[0][1] = res::tiles[1];
	sys::charBlocks[0][2] = res::tiles[2];
	sys::charBlocks[0][3] = res::tiles[3];
	sys::charBlocks[0][4] = res::tiles[4];

	for (uint8_t quadrant = 0; quadrant < 4; quadrant++) {
		for (uint32_t screenEntryIdx = 0; screenEntryIdx < 0x100; screenEntryIdx++) {
			sys::screenBlocks[screenBlockBase][(quadrant * 0x100) + screenEntryIdx] =
				vram::screen_entry{quadrant, quadrant, quadrant, quadrant};
		}
	}
	sys::screenBlocks[screenBlockBase][0] =
		vram::screen_entry{4, 0, 0, 0};

	irq::install();
}

void Game::Simulate()
{
	tryFillCmds();
	drainCmds();
}

void Game::Render()
{
	auto const& camera = world.camera;

	using fp0      = math::fixed_point< 0, uint32_t>;
	using trig_fp  = math::fixed_point< 8, uint32_t>;
	using scale_fp = math::fixed_point<12, uint32_t>;
	using P_fp     = math::fixed_point< 8, uint16_t>;
	using dx_fp    = math::fixed_point< 8, uint32_t>;

	auto const M7_D = fp0{128};
	auto const phi = 0.4;
	auto const cos_phi = trig_fp{::cos(phi)};
	auto const sin_phi = trig_fp{::sin(phi)};

	for (int i = 0; i < sys::screenHeight; i++) {
		auto const lam = scale_fp{scale_fp{80} / fp0{i}};
		auto const lam_cos_phi = scale_fp{lam * cos_phi};
		auto const lam_sin_phi = scale_fp{lam * sin_phi};

		affineParams[i].P[0] = P_fp{lam_cos_phi}.to_rep();
		affineParams[i].P[1] = 0;
		affineParams[i].P[2] = P_fp{lam_sin_phi}.to_rep();
		affineParams[i].P[3] = 0;

		affineParams[i].dx[0] = dx_fp
			{ dx_fp{camera.a_x}
			- (fp0{120} * dx_fp{lam_cos_phi})
			+ dx_fp{M7_D * lam_sin_phi}
		}.to_rep();
		affineParams[i].dx[1] = dx_fp
			{ dx_fp{camera.a_y}
			- (fp0{120} * dx_fp{lam_sin_phi})
			- dx_fp{M7_D * lam_cos_phi}
		}.to_rep();
	}

#if 0
	volatile int x = 100000;
	while (x--);
#endif
}

void Game::FlipAffineBuffer() const
{
	sys::VBlankIntrWait();

	for (int i = 0; i < sys::screenHeight; i++) {
		irq::affineParams[i] = affineParams[i];
	}
	irq::affineParams[sys::screenHeight] = affineParams[0];
}
