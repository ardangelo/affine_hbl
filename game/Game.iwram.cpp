#include "Game.hpp"

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

static constexpr auto vblankEnabledMask = vram::interrupt_mask
	{ .vblank = 1
};
static constexpr auto interruptsEnabled = vram::interrupt_master
	{ .enabled = 1
};

namespace irq
{
namespace iwram
{

volatile uint32_t vblankCount = 0;
vram::affine::param affineParams[161];

IWRAM_CODE
void isr()
{
	if (auto const vblankMask = sys::irqsRaised & vblankEnabledMask) {
		vblankCount++;

		sys::dma3Control = stopDmaControl;
		sys::dma3Source  = (const void*)&affineParams[1];
		sys::dma3Dest    = (void*)sys::bg2aff.begin();
		sys::dma3Control = affineHblankDmaControl;

		sys::irqsRaised     = vblankMask;
		sys::biosIrqsRaised = vblankMask;
	}
}

IWRAM_CODE
void install()
{
	sys::irqServiceRoutine = isr;
	sys::irqsEnabled = vblankEnabledMask;
	sys::irqsEnabledFlag = interruptsEnabled;
}

} // namespace iwram
} // namespace irq

namespace
{
	using fp0      = math::fixed_point< 0, uint32_t>;
	using scale_fp = math::fixed_point<12, uint32_t>;
	
	using trig_fp  = math::fixed_point< 8, uint32_t>;
	using P_fp     = math::fixed_point< 8, uint16_t>;
	using dx_fp    = math::fixed_point< 8, uint32_t>;
}

IWRAM_CODE
void Game::Render()
{
	auto& cam = m_world.m_camera;
	cam.theta += bsp::angle_fp{0.1};
	cam.a_y = bsp::coord_fp{30};
	cam.a_z = bsp::coord_fp{-20};

	auto const M7_D = fp0{128};

#if 0
	auto const phi = 0.0;
	auto const cos_phi = trig_fp{::cos(phi)};
	auto const sin_phi = trig_fp{::sin(phi)};

	for (int i = 0; i < sys::screenHeight; i++) {
		auto const lam = scale_fp{scale_fp{80} / fp0{i}};
		auto const lam_cos_phi = scale_fp{lam * cos_phi};
		auto const lam_sin_phi = scale_fp{lam * sin_phi};

		m_affineParams[i].P[0] = P_fp{lam_cos_phi}.to_rep();
		m_affineParams[i].P[1] = 0;
		m_affineParams[i].P[2] = P_fp{lam_sin_phi}.to_rep();
		m_affineParams[i].P[3] = 0;

		m_affineParams[i].dx[0] = dx_fp
			{ dx_fp{camera.a_x}
			- (fp0{120} * dx_fp{lam_cos_phi})
			+ dx_fp{M7_D * lam_sin_phi}
		}.to_rep();
		m_affineParams[i].dx[1] = dx_fp
			{ dx_fp{camera.a_y}
			- (fp0{120} * dx_fp{lam_sin_phi})
			- dx_fp{M7_D * lam_cos_phi}
		}.to_rep();
	}
#else
	using Map = bsp::ExampleMap;
	static auto sortedSectors = bsp::sort_sectors_from<Map>(cam.a_y, cam.a_z);
	static auto renderer = bsp::Renderer<sys::screenHeight>{};
	static auto last_a_y = std::numeric_limits<bsp::coord_fp>::min();
	static auto last_a_z = std::numeric_limits<bsp::coord_fp>::min();
	static auto last_theta = std::numeric_limits<bsp::angle_fp>::min();

	auto const moved = (last_a_y != cam.a_y) || (last_a_z != cam.a_z);
	auto const rotated = (last_theta != cam.theta);
	if (moved) {
		sortedSectors = bsp::sort_sectors_from<Map>(cam.a_y, cam.a_z);
		last_a_y = cam.a_y;
		last_a_z = cam.a_z;
	}
	if (rotated) {
		renderer = bsp::Renderer<sys::screenHeight>{};
		last_theta = cam.theta;
	}
	if (moved || rotated) {
		renderer.calculateDrawranges<Map>(cam.a_y, cam.a_z, cam.theta, sortedSectors
			);
	}

	{ using DrawImpl = SDL;
		auto minimap = Minimap<Map, 400, 400, 3>{};
		minimap.render(cam.a_x, cam.a_y, cam.a_z, cam.theta, renderer);

		for (int rowPx = 0; rowPx < 400; rowPx++) {
			for (int colPx = 0; colPx < 400; colPx++) {
				auto const& pix = minimap.buf[rowPx][colPx];
				DrawImpl::drawPx(sys::screenWidth + colPx, rowPx, pix[2] / 8, pix[1] / 8, pix[0] / 8);
			}
		}
		getchar();
	}

#endif

#if 0
	volatile int x = 100000;
	while (x--);
#endif
}
