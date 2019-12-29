#include <cmath>

#include "vram.hpp"
#include "resources.hpp"
#include "math.hpp"

#if defined(__gba)
#include "system/GBA.hpp"
using sys = GBA;

#define IWRAM_CODE __attribute__((section(".iwram"), long_call))
#define dbgprintf(...)

#elif defined(__sdl)
#include "system/SDL.hpp"
using sys = SDL;

#define IWRAM_CODE
#define dbgprintf fprintf

#endif

#include "World.hpp"
#include "Menu.hpp"

class Game
{
public: // types
	static constexpr auto BackupTics = 16;
	struct InputDelayBuffer : public circular_queue<World::Cmd, BackupTics>
	{
		uint32_t inputTime = 0;
		auto NextInputTimeFrame() {
			auto const lastInputTime = inputTime;
			inputTime = GetTime();
			return std::make_pair(lastInputTime, inputTime);
		}
	};

public: // members
	circular_queue<event::type, 16> m_eventQueue;
	vram::affine::param m_affineParams[sys::screenHeight];

	InputDelayBuffer m_worldCmdQueue;
	World m_world;
	Menu m_menu;

private: // helpers
	// 1. Try to collect input commands to simulate
	void tryFillCmds();
	// 2. Simulate world state from available input commands
	void drainCmds();

public: // interface
	static uint32_t GetTime();

	Game();

	// Collect and simulate input commands
	void Simulate();

	// Render game (menu, world) state into affine buffers
	IWRAM_CODE void Render();

	// Copy affine buffers to system video memory
	void FlipAffineBuffer() const;
};
