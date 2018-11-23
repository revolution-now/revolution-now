import json
files = json.loads( file( '.builds/current/compile_commands.json', 'r' ).read() )

flags = {}

for f in files:
    flags[str( f['file'] )] = str( f['command'] )

def FlagsForFile( filename, **kwargs ):
    return { 'flags': flags[filename].split() }

print flags
