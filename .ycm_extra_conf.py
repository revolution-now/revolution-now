from os import uname
from os.path import relpath, dirname, splitext, basename, join
import subprocess as sp
import sys

CLANG_INCLUDES = '/opt/local/libexec/llvm-6.0/include/c++/v1'

def isLinux():
    return 'Linux' in uname()

def isMac():
    return 'Darwin' in uname()

assert isLinux() or isMac()

DEBUG    = False
LOG_FILE = "/tmp/ycm-extra-conf-log.txt"
LIB_DIR  = '.lib-linux64' if isLinux() else '.lib-osx'

def _run_cmd( cmd ):
    p = sp.Popen( cmd, stdout=sp.PIPE, stderr=sp.PIPE )
    (stdout, stderr) = p.communicate()
    assert p.returncode == 0, 'error:\n%s' % stderr
    return stdout

def _make( path ):

    # Convert file to object file, put it inside the lib folder,
    # and then make it relative to current directory, e.g.:
    #
    #    /A/B/C/file.cpp ==> ../../C/.lib-linux64/file.o

    rel_file = relpath( path )
    rel_dir  = dirname( rel_file )
    stem     = basename( splitext( rel_file )[0] )
    target   = join( rel_dir, LIB_DIR, stem) + '.o'

    # -n : don't run the target, just print commands,
    # -B : act as if the target were out of date
    # V= : tells nr-make to not suppress command echoing
    cmd = ['/usr/bin/make', 'CLANG=', '-nB', 'V=', target]

    # For some reason the clang that YCM is using on Mac
    # does not seem to have all of the C++17 libraries
    # available, despite supporting c++17 mode.  So we will
    # inject the include paths from macports clang 6.0 which
    # seems to have them and hope that it will work out.
    if isMac():
        cmd.append( 'CFLAGS+=-I%s' % CLANG_INCLUDES )

    stdout = _run_cmd( cmd )

    (line,) = [l for l in stdout.split( '\n' ) if target in l]

    return line

def FlagsForFile( filename, **kwargs ):

    flags = _make( filename )

    if DEBUG:
        with file( LOG_FILE, 'a' ) as f:
            f.write( "-\nfilename: %s\n" % filename )
            f.write( "cmd:      %s\n" % flags )

    return { 'flags': flags.split() }

if __name__ == '__main__':
    print ' '.join( FlagsForFile( sys.argv[1] )['flags'] )
