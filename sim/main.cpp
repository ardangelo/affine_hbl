#include "game/Game.hpp"

int
#ifdef _WIN32
WinMain
#else
main
#endif
(int argc, char const* argv[])
{
	Game game{};

	while (true) {
		dbgprintf(stdout, "%u\n", game.GetTime());

		game.Simulate();
		game.Render();
		game.FlipAffineBuffer();
	}

	return 0;
}
