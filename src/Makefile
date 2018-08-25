ifndef root
    include $(dir $(lastword $(MAKEFILE_LIST)))../Makefile
else
    # In general, must enter in order of dependencies.
    $(call make_exe,MAIN,rn)
endif
