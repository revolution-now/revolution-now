CFLAGS= -DLUA_COMPAT_5_2 $(SYSCFLAGS) $(MYCFLAGS)

LIBS= -lm $(SYSLIBS) $(MYLIBS)

# == END OF USER SETTINGS -- NO NEED TO CHANGE ANYTHING BELOW THIS LINE =======

PLATS= aix bsd c89 freebsd generic linux macosx mingw posix solaris

liblua.a
	lapi.o
	lauxlib.o
	lbaselib.o
	lbitlib.o
	lcode.o
	lcorolib.o
	lctype.o
	ldblib.o
	ldebug.o
	ldo.o
	ldump.o
	lfunc.o
	lgc.o
	linit.o
	liolib.o
	llex.o
	lmathlib.o
	lmem.o
	loadlib.o
	lobject.o
	lopcodes.o
	loslib.o
	lparser.o
	lstate.o
	lstring.o
	lstrlib.o
	ltable.o
	ltablib.o
	ltm.o
	lundump.o
	lutf8lib.o
	lvm.o
	lzio.o

lua
	lua.c

luac
	luac.c

linux:
	$(MAKE) $(ALL) SYSCFLAGS="-DLUA_USE_LINUX" SYSLIBS="-Wl,-E -ldl -lreadline"

macosx:
	$(MAKE) $(ALL) SYSCFLAGS="-DLUA_USE_MACOSX" SYSLIBS="-lreadline"
