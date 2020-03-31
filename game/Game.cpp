#include "Game.hpp"

// Global IRQ implementation
namespace irq
{

namespace iwram
{
	extern
	volatile uint32_t vblankCount;

	extern
	vram::affine::param affineParams[161];

	IWRAM_CODE extern
	void isr();

	IWRAM_CODE extern
	void install();
} // namespace iwram

auto getVblankCount() { return iwram::vblankCount; }

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
	auto const [lastInputTime, inputTime] = m_worldCmdQueue.NextInputTimeFrame();

	for (auto tic = lastInputTime; tic < inputTime; tic++) {
		// Fill events
		sys::PumpEvents(m_eventQueue);

		// Drain events
		while (!m_eventQueue.empty()) {
			auto const ev = m_eventQueue.pop_front();

			if (std::visit(Responder{m_menu}, ev)) {
				continue;
			}
			std::visit(Responder{m_world}, ev);
		}

		if (m_worldCmdQueue.full()) {
			break;
		} else {
			m_worldCmdQueue.push_back(m_world.BuildCmd());
		}
	}
}

void Game::drainCmds()
{
	while (!m_worldCmdQueue.empty()) {
		m_menu.Tick();

		auto const cmd = m_worldCmdQueue.pop_front();
		m_world.Tick(cmd);
	}
}

uint32_t Game::GetTime()
{
	return irq::getVblankCount() / 2;
}

Game::Game()
	: m_eventQueue{}
	, m_worldCmdQueue{}
	, m_world{}
	, m_menu{}
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

	irq::iwram::install();
}

void Game::Simulate()
{
	tryFillCmds();
	drainCmds();
}

void Game::FlipAffineBuffer() const
{
	sys::VBlankIntrWait();

	for (int i = 0; i < sys::screenHeight; i++) {
		irq::iwram::affineParams[i] = m_affineParams[i];
	}
	irq::iwram::affineParams[sys::screenHeight] = m_affineParams[0];
}
