# Run a python script to convert Pyxel Edit's JSON tile-map output
# files to a format easier to parse.

json_files := $(wildcard *.json)

tms := $(json_files:.json=.tm)

all: $(tms)

%.tm: %.json $(lastword $(MAKEFILE_LIST))
	python tile-map-2-txt.py $<

clean:
	rm $(tms)

.PHONY: all clean
