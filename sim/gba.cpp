#include <tonc.h>

#include <array>

#include "system.hpp"

using sys = GBA;

struct TicCmd {
	uint32_t da_x;
};

struct Menu {
	struct State {
		bool active;

		constexpr State()
			: active{false}
		{}
	};

	struct Responder {
		State& state;

		template <typename Any>
		constexpr bool operator()(Any&&) const {
			return false;
		}
	};

	constexpr void Ticker() {
	}

	State state;
	Responder responder;

	constexpr Menu()
		: state{}
		, responder{state}
	{}

	constexpr bool active() const { return state.active; }
};

struct Camera {
	uint32_t a_x;

	constexpr void Think(TicCmd const& ticCmd) {
		a_x += ticCmd.da_x;
	}

	constexpr void Ticker(TicCmd const& ticCmd) {
		Think(ticCmd);
	}

	constexpr Camera()
		: a_x{0}
	{}
};

struct Game {
	using Tic = uint32_t;
	static constexpr auto BackupTics = 12;

	struct State {
		bool keydown[3];

		circular_buffer<TicCmd, BackupTics> ticCmdBuf;
		Tic gameTime, inputTime, lastInputTime;

		constexpr State()
			: keydown{}
			, ticCmdBuf{}
			, gameTime{0}
			, inputTime{0}
			, lastInputTime{0}
		{}
	};

	struct Responder {
		State& state;

		constexpr bool operator()(event::Key key) const {
			using Ty = event::Key::Type;
			using St = event::Key::State;

			switch (key.state) {
			
			case St::Down:
				switch (key.type) {
					case Ty::Pause:
						return true;

					default:
						state.keydown[(uint32_t)key.type] = true;
						return true;
				}

			case St::Up:
				state.keydown[(uint32_t)key.type] = false;
				return false;

			}
		}

		template <typename Any>
		constexpr bool operator()(Any&&) const {
			return false;
		}
	};

	constexpr void Ticker(TicCmd const& ticCmd) {
		camera.Ticker(ticCmd);
	}

	State state;
	Responder responder;
	Camera camera;

	constexpr Game()
		: state{}
		, responder{state}
	{}

	constexpr void BuildTicCmd(TicCmd& cmd) const {
		auto constexpr speed = 0x1;

		using Ty = event::Key::Type;
		if (state.keydown[(uint32_t)Ty::Left]) {
			cmd.da_x -= speed;
		}
		if (state.keydown[(uint32_t)Ty::Right]) {
			cmd.da_x += speed;
		}
	}
};

void Display(Menu const& menu, Game const& game) {
	sys::bg0HorzOffs = game.camera.a_x;
}

void ProcessEvents(Menu& menu, Game& game) {
	sys::pump_events(event::queue);

	while (!event::queue.empty()) {
		auto const ev = event::queue.pop_front();

		if (std::visit(menu.responder, ev)) {
			continue;
		}
		std::visit(game.responder, ev);
	}
}

uint32_t frame = 0;
void TryRunTics(Menu& menu, Game& game) {
	auto& st = game.state;

	auto inputTimeElapsed = (frame / 2) - st.lastInputTime; //sys::GetTime() - st.lastInputTime;

	st.lastInputTime += inputTimeElapsed;

	while (inputTimeElapsed--) {
		ProcessEvents(menu, game);

		if ((st.inputTime - st.gameTime) > (Game::BackupTics / 2)) {
			break;
		}

		game.BuildTicCmd(st.ticCmdBuf.at(st.inputTime));
		st.inputTime++;
	}

	auto ticsToRun = st.inputTime - st.gameTime;
	if (ticsToRun == 0) {
		return;
	}

	while (ticsToRun--) {
		if (menu.active()) {
			menu.Ticker();
		}

		game.Ticker(game.state.ticCmdBuf.at(st.gameTime));
		st.gameTime++;
	}
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

	Menu menu{};
	Game game{};

	irq_init(nullptr);
	irq_add(II_VBLANK, nullptr);

	while (true) {
		TryRunTics(menu, game);
		Display(menu, game);
		frame++;
		VBlankIntrWait();
	}

	return 0;
}
