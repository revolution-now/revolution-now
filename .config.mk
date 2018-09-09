CXXFLAGS += -std=c++17

ifneq ($(origin CLANG),undefined)
    CXXFLAGS += -Wno-c++17-extensions
endif

sdl_cflags  := $(shell sdl2-config --cflags)
sdl_ldflags := $(shell sdl2-config --libs)

CFLAGS  += $(sdl_cflags)
LDFLAGS += $(sdl_ldflags) -lSDL2_image -lSDL2_mixer

main_is := MAIN

$(call enter,src)

build-art:
	@cd $(root)/art && $(MAKE) -s

all: build-art

.PHONY: build-art
