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

int main() {
	__asm("@ zero REG_WIN0V start");
	REG_WIN0V = 0;
	__asm("@ zero REG_WIN0V done");
	{ using namespace Kvasir::WIN0V;
		__asm("@ set REG_WIN0V start");
		apply(
			write(y2, SCREEN_HEIGHT));
		__asm("@ set REG_WIN0V end");
	}

	__asm("@ read REG_WIN0V start");
	auto x = apply(read(Kvasir::BG0CNT::screenSize));
	__asm("@ read REG_WIN0V end");

	while(1) {
		VBlankIntrWait();
	}

	return 0;

}
