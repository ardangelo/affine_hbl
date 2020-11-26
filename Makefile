ROMNAME	= main

TARGETS = $(ROMNAME)-bmp.exe

# Detect devkitPro support
ifneq (,$(wildcard $(DEVKITARM)/gba_rules))
	# Source GBA Make rules
	PATH := $(DEVKITARM)/bin:$(DEVKITPRO)/tools/bin:$(PATH)
	include $(DEVKITARM)/gba_rules

	# Add GBA target
	TARGETS += $(ROMNAME).gba
endif

# Set SDL package location for macOS / Windows
#PACKAGES = /opt/local
#PACKAGES = /mingw64
PACKAGES = /usr

# Detect SDL support, add SDL target
ifneq (,$(wildcard $(PACKAGES)/bin/sdl2-config))
	TARGETS += $(ROMNAME)-sdl.exe
endif

all: $(TARGETS)

# General C++, LD flags
CXXFLAGS = -I. -std=c++17 -g -O2 -Wall -ffast-math -fconcepts -fno-exceptions -fno-non-call-exceptions -fno-strict-aliasing -fno-rtti -Winvalid-pch
LDFLAGS  = $(CXXFLAGS)

# GBA flags

TONC_CFLAGS = -Iinclude
TONC_LIBS   = -Llib -ltonc

GBA_SPECS = -specs=gba_mb.specs

GBA_RARCH = -mthumb-interwork -mthumb
GBA_IARCH = -mthumb-interwork -marm -mlong-calls

GBA_CC       = arm-none-eabi-g++
GBA_ASFLAGS  = -mthumb-interwork
GBA_CXXFLAGS = $(TONC_CFLAGS) -mcpu=arm7tdmi -mtune=arm7tdmi -D__gba
GBA_LDFLAGS	 = $(TONC_LIBS) $(GBA_SPECS) -Wl,-Map,$(ROMNAME).map

# SDL flags

SDL_CC = g++
SDL_CXXFLAGS = $(shell $(PACKAGES)/bin/sdl2-config --cflags) -D__sdl
SDL_LDFLAGS  = $(shell $(PACKAGES)/bin/sdl2-config --libs)
#SDL_LDFLAGS  = $(shell $(PACKAGES)/bin/sdl2-config --libs) -mconsole

# BMP flags

BMP_CC = g++
BMP_CXXFLAGS = -D__bmp
BMP_LDFLAGS  =

# Game sources

SYS_HEADERS  = register.hpp vram.hpp event.hpp
GAME_HEADERS = game/Game.hpp game/World.hpp game/Menu.hpp game/resources.hpp game/BSP.hpp

GBA_HEADERS = $(SYS_HEADERS) $(GAME_HEADERS) system/GBA.hpp
SDL_HEADERS = $(SYS_HEADERS) $(GAME_HEADERS) system/PC.hpp system/SDL.hpp
BMP_HEADERS = $(SYS_HEADERS) $(GAME_HEADERS) system/PC.hpp system/BMP.hpp

# Game objects

GAME_OBJS = main.@.o game/Game.@.o game/Game.iwram.@.o game/Menu.@.o game/World.@.o

GBA_OBJS = $(patsubst %.@.o,%.gba.o,$(GAME_OBJS))
SDL_OBJS = $(patsubst %.@.o,%.sdl.o,$(GAME_OBJS))
BMP_OBJS = $(patsubst %.@.o,%.bmp.o,$(GAME_OBJS))

# GBA targets

%.iwram.gba.o : %.iwram.cpp $(GBA_HEADERS)
	$(GBA_CC) $(CXXFLAGS) $(GBA_CXXFLAGS) $(GBA_IARCH) -c $< -o $@

%.gba.o : %.cpp $(GBA_HEADERS)
	$(GBA_CC) $(CXXFLAGS) $(GBA_CXXFLAGS) -flto $(GBA_RARCH) -c $< -o $@

# link objects into an elf
$(ROMNAME).elf : $(GBA_OBJS)
	$(GBA_CC) $(GBA_OBJS) $(LDFLAGS) $(GBA_LDFLAGS) -o $(ROMNAME).elf

# objcopy and fix the rom
$(ROMNAME).gba : $(ROMNAME).elf
	arm-none-eabi-objcopy -v -O binary $(ROMNAME).elf $(ROMNAME).gba
	gbafix $(ROMNAME).gba -t$(ROMNAME)

# SDL targets

%.sdl.o: %.cpp $(SDL_HEADERS)
	$(SDL_CC) $(CXXFLAGS) $(SDL_CXXFLAGS) -c $< -o $@

$(ROMNAME)-sdl.exe: $(SDL_OBJS)
	$(SDL_CC) $(SDL_OBJS) $(LDFLAGS) $(SDL_LDFLAGS) -o $(ROMNAME)-sdl.exe

# BMP targets

%.bmp.o: %.cpp $(BMP_HEADERS)
	$(BMP_CC) $(CXXFLAGS) $(BMP_CXXFLAGS) -c $< -o $@

$(ROMNAME)-bmp.exe: $(BMP_OBJS)
	$(BMP_CC) $(BMP_OBJS) $(LDFLAGS) $(BMP_LDFLAGS) -o $(ROMNAME)-bmp.exe

clean:
	@rm -fv *.gba *.elf *.sav *.map
	@rm -fv *.o game/*.o system/*.o
	@rm -rfv *.exe *.dSYM
