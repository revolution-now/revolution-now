.DEFAULT_GOAL := all
build-current := .builds/current

all run clean test: $(build-current)
	@cd $(build-current) && $(MAKE) -s $@

distclean:
	@rm -rf .builds

$(build-current):
	@scripts/configure

.PHONY: all run clean test distclean
