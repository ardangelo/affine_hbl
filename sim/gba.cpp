// #include <tonc.h>

#include <array>

#include "system.hpp"

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

static uint32_t GetTime() { return vblank_counter::get() / 2; }

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

	constexpr bool active() const { return state.active; }
};

struct Game {
	struct Cmd {
		uint32_t da_x;
	};

	struct Responder {
		bool keydown[3];

		constexpr bool operator()(event::Key key) {
			using Ty = event::Key::Type;
			using St = event::Key::State;

			switch (key.state) {

			case St::Down:
				switch (key.type) {
					case Ty::Pause:
						return true;

					default:
						keydown[(uint32_t)key.type] = true;
						return true;
				}

			case St::Up:
				keydown[(uint32_t)key.type] = false;
				return false;

			}
		}

		template <typename Any>
		constexpr bool operator()(Any&&) {
			keydown[0] = 1;
			return false;
		}

		constexpr Responder()
			: keydown{}
		{}

		constexpr Cmd BuildCmd() const {
			auto constexpr speed = 0x1;

			auto cmd = Cmd{};

			using Ty = event::Key::Type;
			if (keydown[(uint32_t)Ty::Left]) {
				cmd.da_x -= speed;
			}
			if (keydown[(uint32_t)Ty::Right]) {
				cmd.da_x += speed;
			}

			return cmd;
		}
	} responder;

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
	};

	struct Ticker {
		enum class State : uint32_t {
			InGame
		} state;

		Camera camera;

		constexpr void operator()(Cmd const& cmd) {
			if (state == State::InGame) {
				camera.Ticker(cmd);
			}
		}

		constexpr Ticker()
			: state{State::InGame}
			, camera{}
		{}
	} ticker;

	constexpr Game()
		: responder{}
		, ticker{}
	{}
};

template <typename T, size_t BackupTics>
struct InputDelayBuffer : public circular_queue<T, BackupTics>
{
	uint32_t inputTime;
	auto NextInputTimeFrame() {
		auto const lastInputTime = inputTime;
		inputTime = GetTime();
		return std::make_pair(lastInputTime, inputTime);
	}
};

struct World {
	circular_queue<event::type, 16> event_queue{};

	static constexpr auto BackupTics = 16;
	InputDelayBuffer<Game::Cmd, BackupTics> gameCmdQueue{};

	Game game;
	Menu menu;

	void FillEvents() {
		sys::pump_events(event_queue);
	}

	void DrainEvents() {
		while (!event_queue.empty()) {
			auto const ev = event_queue.pop_front();

			if (std::visit(menu.responder, ev)) {
				continue;
			}
			std::visit(game.responder, ev);
		}
	}

	void TryFillCmds() {
		auto const [lastInputTime, inputTime] = gameCmdQueue.NextInputTimeFrame();

		for (auto tic = lastInputTime; tic < inputTime; tic++) {
			FillEvents();
			DrainEvents();

			if (gameCmdQueue.full()) {
				break;
			} else {
				gameCmdQueue.push_back(game.responder.BuildCmd());
			}
		}
	}

	void DrainCmds() {
		while (!gameCmdQueue.empty()) {
			menu.ticker();

			auto const cmd = gameCmdQueue.pop_front();
			game.ticker(cmd);
		}
	}
};

void Display(World const& world) {
	sys::bg0HorzOffs = world.game.ticker.camera.a_x % (64 * 8);
}

static constexpr auto tiles = std::array<vram::CharEntry, 2> {
{
	{ 0x11111111
	, 0x01111111
	, 0x01111111
	, 0x01111111
	, 0x01111111
	, 0x01111111
	, 0x01111111
	, 0x00000001
	}
,
	{ 0x00000000, 0x00100100, 0x01100110, 0x00011000
	, 0x00011000, 0x01100110, 0x00100100, 0x00000000
	}
} };

static constexpr auto Red   = vram::Rgb15::make(31,  0,  0);
static constexpr auto Blue  = vram::Rgb15::make( 0, 31,  0);
static constexpr auto Green = vram::Rgb15::make( 0,  0, 31);
static constexpr auto Grey  = vram::Rgb15::make(15, 15, 15);

int main(int argc, char const* argv[])
{
	sys::dispCnt = 0x1100;
	sys::dispStat = 0x8;

	auto constexpr charBlockBase = 0;
	auto constexpr screenBlockBase = 28;

	sys::bg0Cnt = vram::BgCnt
		{ .priority = 0
		, .charBlockBase = charBlockBase
		, .mosaicEnabled = false
		, .palMode = vram::BgCnt::PalMode::Bits4
		, .screenBlockBase = screenBlockBase
		, .affineWrapEnabled = false
		, .mapSize = vram::BgCnt::MapSize::Reg64x64
	};

	sys::palBanks[0][1] = Red;
	sys::palBanks[1][1] = Blue;
	sys::palBanks[2][1] = Green;
	sys::palBanks[3][1] = Grey;

	sys::charBlocks[0][0] = tiles[0];
	sys::charBlocks[0][1] = tiles[1];

	for (uint32_t screenBlockOffs = 0; screenBlockOffs < 4; screenBlockOffs++) {
		for (uint32_t screenEntryIdx = 0; screenEntryIdx < 32 * 32; screenEntryIdx++) {
			sys::screenBlocks[screenBlockBase + screenBlockOffs][screenEntryIdx] = vram::ScreenEntry
				{ .tileId = 0
				, .horzFlip = 0
				, .vertFlip = 0
				, .palBank = (uint16_t)screenBlockOffs
			};
		}
	}

	sys::bg0Cnt |= vram::BgCnt{ .affineWrapEnabled = true };

	sys::screenBlocks[screenBlockBase][0] |= vram::ScreenEntry{ .tileId = 0x1 };

	vblank_counter::install();

	World world{};

	while (true) {
		sys::VBlankIntrWait();

		world.TryFillCmds();
		world.DrainCmds();

		Display(world);
	}

	return 0;
}
