# This Makefile is just for convenience

build-current := .builds/current

all: $(build-current)
	@cd $(build-current) && $(MAKE) -s all

run: $(build-current)
	@cd $(build-current) && $(MAKE) -s run

clean:
	@cd $(build-current) && $(MAKE) -s clean

distclean:
	@rm -rf .builds

.PHONY: all run clean distclean

$(build-current):
	@./configure
