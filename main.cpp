#include "game/Game.hpp"

int
#ifdef _WIN32
WinMain
#else
main
#endif
(int argc, char const* argv[])
{
	auto game = Game{};

	while (true) {
		dbgprintf(stdout, "time: %u\n", game.GetTime());

		game.Simulate();
		game.Render();
		game.FlipAffineBuffer();
	}

	return 0;
}
