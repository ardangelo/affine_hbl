// #include <tonc.h>

#include <cmath>

#include "system.hpp"
#include "resources.hpp"

using sys = GBA;

namespace irq
{
	static volatile auto vblankCount = uint32_t{0};

	static vram::affine::param affineParams[161];
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
		auto const irqsRaised = sys::irqsRaised.Get();

		if (auto const vblankMask = irqsRaised & vram::interrupt_mask{ .vblank = 1 }) {
			vblankCount++;

			sys::dma3Control = 0;
			sys::dma3Source  = (const void*)&affineParams[1];
			sys::dma3Dest    = (void*)sys::bg2aff.base;
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
}

struct Menu {
	struct State {
		bool active;

		constexpr State()
			: active{false}
		{}
	} state;

	struct Responder {
		State& state;

		template <typename Any>
		constexpr bool operator()(Any&&) const {
			return false;
		}
	} responder;

	struct Ticker {
		State& state;

		constexpr void operator()() {
			if (state.active) {

			}
		}
	} ticker;

	constexpr Menu()
		: state{}
		, responder{state}
		, ticker{state}
	{}
};

struct World {
	struct State {
		bool keydown[5];

		struct Cmd {
			uint32_t da_x, da_y;
		};

		constexpr Cmd BuildCmd() const {
			auto constexpr speed = 0x1;

			auto cmd = Cmd{};

			using Ty = event::Key::Type;
			if (keydown[Ty::Right]) {
				cmd.da_x += speed;
			}
			if (keydown[Ty::Left]) {
				cmd.da_x -= speed;
			}
			if (keydown[Ty::Up]) {
				cmd.da_y -= speed;
			}
			if (keydown[Ty::Down]) {
				cmd.da_y += speed;
			}

			return cmd;
		}

		enum class Mode : uint32_t {
			InGame
		} mode;

		struct Camera {
			uint32_t a_x, a_y;

			void Think(Cmd const& cmd) {
				a_x += cmd.da_x;
				a_y += cmd.da_y;
			}

			void Ticker(Cmd const& cmd) {
				Think(cmd);
			}

			constexpr Camera()
				: a_x{0}
				, a_y{0}
			{}
		} camera;

		constexpr State()
			: keydown{}
			, mode{Mode::InGame}
			, camera{}
		{}
	} state;

	struct Responder {
		State& state;

		constexpr bool operator()(event::Key key) {
			using Ty = event::Key::Type;
			using St = event::Key::State;

			switch (key.state) {

			case St::On:
				switch (key.type) {
					case Ty::Pause:
						return true;

					default:
						state.keydown[key.type] = true;
						return true;
				}

			case St::Off:
				state.keydown[key.type] = false;
				return false;

			}
		}

		template <typename Any>
		constexpr bool operator()(Any&&) {
			return false;
		}
	} responder;

	struct Ticker {
		State& state;

		constexpr void operator()(State::Cmd const& cmd) {
			if (state.mode == State::Mode::InGame) {
				state.camera.Ticker(cmd);
			}
		}
	} ticker;

	constexpr World()
		: state{}
		, responder{state}
		, ticker{state}
	{}
};

struct Game {
	circular_queue<event::type, 16> event_queue;

	vram::affine::param affineParams[sys::screenHeight + 1];

	static uint32_t GetTime() { return irq::getVblankCount() / 2; }

	static constexpr auto BackupTics = 16;
	struct InputDelayBuffer : public circular_queue<World::State::Cmd, BackupTics> {
		uint32_t inputTime = 0;
		auto NextInputTimeFrame() {
			auto const lastInputTime = inputTime;
			inputTime = GetTime();
			return std::make_pair(lastInputTime, inputTime);
		}
	} worldCmdQueue;

	World world;
	Menu menu;

	void TryFillCmds() {
		auto const [lastInputTime, inputTime] = worldCmdQueue.NextInputTimeFrame();

		for (auto tic = lastInputTime; tic < inputTime; tic++) {
			// Fill events
			sys::pump_events(event_queue);

			// Drain events
			while (!event_queue.empty()) {
				auto const ev = event_queue.pop_front();

				if (std::visit(menu.responder, ev)) {
					continue;
				}
				std::visit(world.responder, ev);
			}

			if (worldCmdQueue.full()) {
				break;
			} else {
				worldCmdQueue.push_back(world.state.BuildCmd());
			}
		}
	}

	void DrainCmds() {
		while (!worldCmdQueue.empty()) {
			menu.ticker();

			auto const cmd = worldCmdQueue.pop_front();
			world.ticker(cmd);
		}
	}

	void Simulate() {
		TryFillCmds();
		DrainCmds();
	}

	void Render() {
		auto const M7_D = 128;
		auto const phi = .4;
		auto const cos_phi = (uint32_t)(::cos(phi) * (1 << 8));
		auto const sin_phi = (uint32_t)(::sin(phi) * (1 << 8));

		for (int i = 0; i < sys::screenHeight; i++) {
			auto const lam = i ? ((160 << 12) / i) : -1;
			auto const lam_cos_phi = (lam * cos_phi) >> 8;
			auto const lam_sin_phi = (lam * sin_phi) >> 8;

			affineParams[i].P[0] = lam_cos_phi >> 4;
			affineParams[i].P[1] = 0;
			affineParams[i].P[2] = lam_sin_phi >> 4;
			affineParams[i].P[3] = 0;

			affineParams[i].dx[0] = (world.state.camera.a_x << 8)
				- (  120 * (lam_cos_phi  >> 4))
				+ ((M7_D *  lam_sin_phi) >> 4);
			affineParams[i].dx[1] = (world.state.camera.a_y << 8)
				- (  120 * (lam_sin_phi  >> 4))
				- ((M7_D *  lam_cos_phi) >> 4);
		}
		affineParams[sys::screenHeight] = affineParams[0];

	#if 0
		volatile int x = 100000;
		while (x--);
	#endif
	}

	void FlipAffineBuffer() const {
		sys::VBlankIntrWait();

		for (int i = 1; i < sys::screenHeight + 1; i++) {
			irq::affineParams[i] = affineParams[i];
		}
		irq::affineParams[sys::screenHeight] = affineParams[0];
	}

	Game()
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

		sys::palBanks[1] = res::Red;
		sys::palBanks[2] = res::Blue;
		sys::palBanks[3] = res::Green;
		sys::palBanks[4] = res::Grey;

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
};

int main(int argc, char const* argv[])
{
	Game game{};

	while (true) {
		game.Simulate();
		game.Render();
		game.FlipAffineBuffer();
	}

	return 0;
}
