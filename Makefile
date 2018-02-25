PATH := $(DEVKITARM)/bin:$(PATH)
include $(DEVKITARM)/gba_rules

CC			:= arm-none-eabi-gcc
GRIT		:= grit

INCLUDE	:= -Iinclude
LIBS		:= -ltonc -lkrawall
LIBPATHS	:= -Llib

SPECS	:= -specs=gba_mb.specs
ARCH	:= -mthumb-interwork -mthumb
RARCH	:= -mthumb-interwork -mthumb
IARCH	:= -mthumb-interwork -marm -mlong-calls

ASFLAGS	:= -mthumb-interwork
CFLAGS	:= $(INCLUDE) -mcpu=arm7tdmi -mtune=arm7tdmi -O2 -Wall -ffast-math -fno-strict-aliasing
LDFLAGS	:= $(ARCH) $(SPECS) $(LIBPATHS) $(LIBS) -Wl,-Map,$(PROJ).map

ROMNAME	:= 3ps

all: $(ROMNAME).gba

# compile the background resources

# Bowser's Castle I floor : affine map, 128x128t, LZ77 compressed
gfx/bc1floor.o gfx/bc1floor.h : gfx/bc1floor.png
	$(GRIT) gfx/bc1floor.png -ogfx/bc1floor -fts -gB8 -mRa -mLa -p! -Zl
	$(CC) $(ASFLAGS) -c gfx/bc1floor.s -o gfx/bc1floor.o
# Bowser's Castle I sky : normal map, 64x32t, LZ77 compressed.
gfx/bc1sky.o gfx/bc1sky.h : gfx/bc1sky.png
	$(GRIT) gfx/bc1sky.png -ogfx/bc1sky -fts -gB4 -mLs -mR4 -ma 128 -p! -Zl
	$(CC) $(ASFLAGS) -c gfx/bc1sky.s -o gfx/bc1sky.o
# Menu borders, LZ77 compressed.
gfx/border.o gfx/border.h : gfx/border.png
	$(GRIT) gfx/border.png -ogfx/border -fts -gt -gB4 -gzl -p!
	$(CC) $(ASFLAGS) -c gfx/border.s -o gfx/border.o
# Background palette, LZ77 compressed.
gfx/bgpal.o gfx/bgpal.h : gfx/bgpal.png
	$(GRIT) gfx/bgpal.png -ogfx/bgpal -fts -gB8 -g! -s bg -Zl
	$(CC) $(ASFLAGS) -c gfx/bgpal.s -o gfx/bgpal.o

# compile the sprite resources

# Karts. Not compressed.
gfx/karts.s gfx/karts.h : gfx/karts.png
	$(GRIT) gfx/karts.png -ogfx/karts -fts -gt -gB4 -Mw4 -Mh4 -p!
	$(CC) $(ASFLAGS) -c gfx/karts.s -o gfx/karts.o
# Thwomp tiles, LZ77 compressed.
gfx/thwomp.s gfx/thwomp.h : gfx/thwomp.png
	$(GRIT) gfx/thwomp.png -ogfx/thwomp -fts -gt -gB4 -p! -Zl
	$(CC) $(ASFLAGS) -c gfx/thwomp.s -o gfx/thwomp.o
# Object palette, LZ77 compressed.
gfx/objpal.s gfx/objpal.h : gfx/objpal.png
	$(GRIT) gfx/objpal.png -ogfx/objpal -fts -gB8 -g! -s obj -Zl
	$(CC) $(ASFLAGS) -c gfx/objpal.s -o gfx/objpal.o

GFX_ASM := gfx/bc1floor.s gfx/bc1sky.s gfx/border.s gfx/bgpal.s
GFX_ASM += gfx/karts.s gfx/thwomp.s gfx/objpal.s
GFX_HEADERS := $(GFX_ASM:.s=.h)
GFX_OBJS := $(GFX_ASM:.s=.o)

# compile the code object files
main.o : main.c $(GFX_HEADERS)
	$(CC) $(CFLAGS) $(RARCH) -c main.c -o main.o

CODE_OBJS := main.o

# link objects into an elf
$(ROMNAME).elf : $(CODE_OBJS) $(GFX_OBJS)
	$(CC) $(CODE_OBJS) $(GFX_OBJS) $(LDFLAGS) -o $(ROMNAME).elf

# objcopy and fix the rom
$(ROMNAME).gba : $(ROMNAME).elf
	arm-none-eabi-objcopy -v -O binary $(ROMNAME).elf $(ROMNAME).gba
	gbafix $(ROMNAME).gba -t$(ROMNAME)

clean :
	@rm -fv *.gba *.elf
	@rm -fv *.o
	@rm -fv gfx/*.s gfx/*.h gfx/*.o
	@rm -fv main.s .map
