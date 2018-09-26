from os import uname
from os.path import relpath, dirname, splitext, basename, join
import subprocess as sp
import sys

def isLinux():
    return 'Linux' in uname()

def isMac():
    return 'Darwin' in uname()

assert isLinux() or isMac()

LOG_FILE = "/tmp/ycm-extra-conf-log.txt"
LIB_DIR  = '.lib-linux64' if isLinux() else '.lib-osx'

# cmd is a string with a shell command.
def _run_cmd( cmd ):
    p = sp.Popen( cmd, stdout=sp.PIPE, stderr=sp.PIPE, shell=True )
    (stdout, stderr) = p.communicate()
    assert p.returncode == 0, 'error:\n%s' % stderr
    return stdout

def find_system_include_paths( compiler_binary ):
  cmd = 'echo | %s -v -E -x c++ - 2>&1' % compiler_binary
  output = _run_cmd( cmd )
  is_header = False
  search_paths = []
  # Print lines between the two marker patterns.
  for line in output.split( '\n' ):
    if 'End of search' in line:
      is_header = False
    if is_header:
      search_paths.append( line.strip() )
    if 'include <..' in line:
      is_header = True
  return search_paths

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
    cmd = ['/usr/bin/make', 'USE_CLANG=', '-nB', 'V=', target]

    stdout = _run_cmd( ' '.join( cmd ) )

    (line,) = [l for l in stdout.split( '\n' ) if target in l]
    words = line.split()

    # words[0] is assumed to contain the compiler binary.
    isystems = find_system_include_paths( words[0] )
    if isystems:
        words.extend( ['-isystem %s' % f for f in isystems] )

    return ' '.join( words )

def FlagsForFile( filename, **kwargs ):
    return { 'flags': _make( filename ).split() }

if __name__ == '__main__':
    print ' '.join( FlagsForFile( sys.argv[1] )['flags'] )
