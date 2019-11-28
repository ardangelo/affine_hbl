// #include <tonc.h>

#include "system.hpp"
#include "resources.hpp"

using sys = GBA;

namespace vblank_counter
{
	namespace
	{
		static constexpr auto interrupt_mask = vram::interrupt_mask{ .vblank = 1 };
		static volatile  auto vblankCount = uint32_t{0};

		void isr()
		{
			vblankCount++;
			sys::irqsRaised = interrupt_mask;
			sys::biosIrqsRaised = interrupt_mask;
		}
	}

	void install()
	{
		sys::irqServiceRoutine = isr;
		sys::irqsEnabled |= interrupt_mask;
		sys::irqsEnabledFlag = 1;
	}

	auto get() { return vblankCount; }
};

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
		bool keydown[3];

		struct Cmd {
			uint32_t da_x;
		};

		constexpr Cmd BuildCmd() const {
			auto constexpr speed = 0x1;

			auto cmd = Cmd{};

			using Ty = event::Key::Type;
			if (keydown[Ty::Left]) {
				cmd.da_x -= speed;
			}
			if (keydown[Ty::Right]) {
				cmd.da_x += speed;
			}

			return cmd;
		}

		enum class Mode : uint32_t {
			InGame
		} mode;

		struct Camera {
			uint32_t a_x;

			void Think(Cmd const& cmd) {
				a_x += cmd.da_x;
			}

			void Ticker(Cmd const& cmd) {
				Think(cmd);
			}

			constexpr Camera()
				: a_x{0}
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

			case St::Down:
				switch (key.type) {
					case Ty::Pause:
						return true;

					default:
						state.keydown[key.type] = true;
						return true;
				}

			case St::Up:
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

	static uint32_t GetTime() { return vblank_counter::get() / 2; }

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

	void Display() {
		sys::VBlankIntrWait();

		sys::bg2P[0] = 1 << 8;
		sys::bg2P[1] = 0 << 8;
		sys::bg2P[2] = 0 << 8;
		sys::bg2P[3] = 1 << 8;

		sys::bg2dx[0] = (world.state.camera.a_x % (64 * 8)) << 8;
		sys::bg2dx[1] = 0 << 8;

	#if 0
		volatile int x = 100000;
		while (x--);
	#endif
	}

	Game()
		: event_queue{}
		, worldCmdQueue{}
		, world{}
		, menu{}
	{
		sys::dispCnt = 0x402;
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
	}
};

int main(int argc, char const* argv[])
{
	vblank_counter::install();

	Game game{};

	while (true) {
		game.Simulate();
		game.Display();
	}

	return 0;
}
