override CXXFLAGS += -std=c++1z -Wno-c++17-extensions

sdl_cflags  := $(shell sdl2-config --cflags)
sdl_ldflags := $(shell sdl2-config --libs)

override CFLAGS  += $(sdl_cflags)
override LDFLAGS += $(sdl_ldflags) -lSDL2_image

main_is := MAIN

$(call enter,src)

build-art:
	@cd $(root)/art && $(MAKE) -s

all: build-art

.PHONY: build-art
