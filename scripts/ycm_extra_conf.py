import os, sys, json

files = json.loads( file( '.builds/current/compile_commands.json', 'r' ).read() )

flags = {}
directories = {}

for f in files:
    flags[str( f['file'] )] = str( f['command'] ).split()
    directories[str( f['file'] )] = str( f['directory'] )

def FlagsForFile( filename, **kwargs ):
    try:
        result = flags[filename]
        result_dir = directories[filename]
    except:
        # Try to find a file in the same folder and use those
        for f,cmd in flags.iteritems():
            if os.path.dirname( f ) == os.path.dirname( filename ):
                result = cmd
                result_dir = directories[f]
                break
        else:
            result = []
    if result:
        # Here we need to scan for any -I directives and, if they contain
        # relative paths, we need to make them absolute.  Seems that some
        # CMake generators will make them relative paths and that seems to
        # mess up YCM.
        def fix( i ):
            if i.startswith( '-I' ):
                include = i[2:]
                abs_include = os.path.abspath( os.path.join( result_dir, include ) )
                return '-I%s' % abs_include
            return i
        result = map( fix, result )
    return { 'flags': result }

if __name__ == '__main__':
    print FlagsForFile( sys.argv[1] )
