# Script to convert Pyxel Edit's JSON tile-map output files
# to a format easier to parse.
#
# The output file format is an ascii text file that contains
# the description of one block per line, with space-separated
# numbers.
import json, os, sys

json_file = sys.argv[1]

print 'input file:', json_file
d = json.loads( file( json_file, "r" ).read() )

print 'tileswide:', d['tileswide']
print 'tileshigh:', d['tileshigh']

for l in d['layers']: # l = layer
    layer_name = l['name'].lower().replace( '/', '_' ).replace( '*', '_' )
    tm_file = os.path.join(os.path.dirname( json_file ), layer_name+'.tm' )
    print 'processing', tm_file
    with file( tm_file, 'w' ) as f:
        f.write( '# index x y tile rot flipX\n' )
        for b in l['tiles']: # b = block
             f.write( '%d %d %d %d %d %s\n' %
                     (b['index'], b['x'], b['y'], b['tile'], b['rot'], int(b['flipX'])) )

print 'finished.'
