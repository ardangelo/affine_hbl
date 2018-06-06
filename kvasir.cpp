#include <tonc.h>

#include <Register/Utility.hpp>

#include <Chip/Unknown/Nintendo/GBA.hpp>
#include <Register/Register.hpp>
using namespace Kvasir::Register;

/* RGB15-specific types */
enum class RGB15ComponentName : std::uint8_t {
	red = 0, green = 1, blue = 2
};
template <typename Addr, RGB15ComponentName i>
using RGB15Component = Kvasir::ReadWriteField<Addr, uint8_t, 5 + (5 * static_cast<int>(i)), 5 * static_cast<int>(i)>;

template <typename Addr>
using PixelRedComponent = RGB15Component<Addr, RGB15ComponentName::red>;
template <typename Addr>
using PixelGreenComponent = RGB15Component<Addr, RGB15ComponentName::green>;
template <typename Addr>
using PixelBlueComponent = RGB15Component<Addr, RGB15ComponentName::blue>;

/* raw video memory address type */
template <int offset>
using RawVidMemAddr = Kvasir::Register::Address<offset, 0x0, 0x0, std::uint16_t>;

/* convert screen coordinate to vid_mem offset */
constexpr uint32_t plotMemLoc(uint32_t x, uint32_t y) {
	return (uint32_t)&vid_mem[y * SCREEN_WIDTH + x];
}

int main() {
	/* clear out SFRs */
	REG_DISPCNT = 0;

	{ /* set display control: Mode 3 and Enable BG2 */
		using namespace Kvasir::DISPCNT;
		apply(
			write(bgMode, BackgroundModes::mode3),
			set(screenDisplayBg2));
	}

	{ /* plot red pixel at 120, 80 */
		using M3PlotAddr = RawVidMemAddr<plotMemLoc(120, 80)>;
		apply(
			write(PixelRedComponent<M3PlotAddr>{},   31),
			write(PixelGreenComponent<M3PlotAddr>{},  0),
			write(PixelBlueComponent<M3PlotAddr>{},   0));
	}

	{ /* plot green pixel at 136, 80 */
		using M3PlotAddr = RawVidMemAddr<plotMemLoc(136, 80)>;
		apply(
			write(PixelRedComponent<M3PlotAddr>{},   0),
			write(PixelGreenComponent<M3PlotAddr>{}, 31),
			write(PixelBlueComponent<M3PlotAddr>{},  0));
	}

	{ /* plot blue pixel at 120, 96 */
		using M3PlotAddr = RawVidMemAddr<plotMemLoc(120, 96)>;
		apply(
			write(PixelRedComponent<M3PlotAddr>{},   0),
			write(PixelGreenComponent<M3PlotAddr>{}, 0),
			write(PixelBlueComponent<M3PlotAddr>{},  31));
	}

	while(1) {
		VBlankIntrWait();
	}

	return 0;

}
