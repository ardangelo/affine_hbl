PATH := $(DEVKITARM)/bin:$(DEVKITPRO)/tools/bin:$(PATH)
include $(DEVKITARM)/gba_rules

ROMNAME	= main

CXXFLAGS = -I. -std=c++17 -g -O2 -Wall -ffast-math -fconcepts -fno-exceptions -fno-non-call-exceptions -fno-strict-aliasing -fno-rtti -Winvalid-pch
LDFLAGS  =

# GBA flags

TONC_CFLAGS = -I../include
TONC_LIBS   = -Llib -ltonc

GBA_SPECS = -specs=gba_mb.specs

GBA_RARCH = -mthumb-interwork -mthumb
GBA_IARCH = -mthumb-interwork -marm -mlong-calls

GBA_CC       = arm-none-eabi-g++
GBA_ASFLAGS  = -mthumb-interwork
GBA_CXXFLAGS = $(TONC_CFLAGS) -mcpu=arm7tdmi -mtune=arm7tdmi -D__gba
GBA_LDFLAGS	 = $(TONC_LIBS) $(GBA_RARCH) $(GBA_SPECS) -Wl,-Map,$(ROMNAME).map

# SDL flags

#PACKAGES = /opt/local/
PACKAGES = /mingw64

SDL_CC = g++
SDL_CXXFLAGS = $(shell $(PACKAGES)/bin/sdl2-config --cflags) -D__sdl
SDL_LDFLAGS  = $(shell $(PACKAGES)/bin/sdl2-config --libs)

all: $(ROMNAME).gba $(ROMNAME).exe

HEADERS = register.hpp vram.hpp resources.hpp
GBA_HEADERS = $(HEADERS) system/GBA.hpp
SDL_HEADERS = $(HEADERS) system/SDL.hpp

# GBA targets

main.gba.o : main.cpp $(GBA_HEADERS)
	$(CC) $(CXXFLAGS) $(GBA_CXXFLAGS) $(IARCH) -c $< -o $@

GBA_OBJS = main.gba.o

# link objects into an elf
$(ROMNAME).elf : $(GBA_OBJS)
	$(GBA_CC) $(GBA_OBJS) $(LDFLAGS) $(GBA_LDFLAGS) -o $(ROMNAME).elf

# objcopy and fix the rom
$(ROMNAME).gba : $(ROMNAME).elf
	arm-none-eabi-objcopy -v -O binary $(ROMNAME).elf $(ROMNAME).gba
	gbafix $(ROMNAME).gba -t$(ROMNAME)

# SDL targets

main.sdl.o: main.cpp $(SDL_HEADERS)
	$(SDL_CC) $(CXXFLAGS) $(SDL_CXXFLAGS) -c $< -o $@

SDL_OBJS = main.sdl.o

$(ROMNAME).exe: $(SDL_OBJS)
	$(SDL_CC) $(SDL_OBJS) $(LDFLAGS) $(SDL_LDFLAGS) -o $(ROMNAME).exe

clean :
	@rm -fv *.gba *.elf *.sav *.exe
	@rm -fv *.o
	@rm -fv *.map
