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

ROMNAME	:= affine_hbl

all: $(ROMNAME).gba

# compile the background resources

# Fan room floor : affine map, 64x64t, LZ77 compressed
gfx/fanroom.o gfx/fanroom.h : gfx/fanroom.png
	$(GRIT) gfx/fanroom.png -ogfx/fanroom -fts -gB8 -mRa -mLa -p! -Zl
	$(CC) $(ASFLAGS) -c gfx/fanroom.s -o gfx/fanroom.o
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

GFX_ASM := gfx/fanroom.s gfx/bgpal.s
GFX_ASM += gfx/karts.s gfx/thwomp.s gfx/objpal.s
GFX_HEADERS := $(GFX_ASM:.s=.h)
GFX_OBJS := $(GFX_ASM:.s=.o)

# compile the code object files
mode7.iwram.o : mode7.iwram.c mode7.h
	$(CC) $(CFLAGS) $(IARCH) -c mode7.iwram.c -o mode7.iwram.o
mode7.o : mode7.c mode7.h
	$(CC) $(CFLAGS) $(RARCH) -c mode7.c -o mode7.o
main.o : main.c $(GFX_HEADERS)
	$(CC) $(CFLAGS) $(RARCH) -c main.c -o main.o

CODE_OBJS := main.o mode7.o mode7.iwram.o

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
