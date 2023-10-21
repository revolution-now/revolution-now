#!/usr/bin/env python3
'''
The purpose of this script is to preprocess an upstream JSON file
containing the definition of the OG's SAV file structure so that
we can read it in. The file is in JSON format which the game can
already parse since JSON is a subset of rcl. However, the issue
is that (as in the JSON spec) rcl/cdr keys are not ordered,
whereas the upstream document assumes that it will be parsed by a
JSON parser that retains object key ordering. The reason that or-
dering of keys is important is because the structure document
represents the list of fields in the save file (along with their
sizes and types) as keys in a table, and so obviously ordering is
important their if we are using that to parse the SAV file.

So the goal of this script therefore is to rewrite that file,
keeping the formatting as close as possible to the original (for
easy diffing), but injecting a field called "__key_order" into
each table where key order is relevant. This new field is a list
of strings that gives the ordering of the keys in that table, so
that we can later iterate over them in the original order.

In order to maintain the formatting as close as possible to the
original (which was written in a style dictated by the arbitrary
personal preferences of the author) we use a custom JSON encoder
that tries to emit spacing, brace placement, and single-line ta-
bles in that same style. This formatting is not really necessary,
but it does actually make the document more readable by keeping
those tables on a single line that give the type/size of each
field.
'''

import sys
import json
import argparse

from smcol_json_encoder import SMColCompactJSONEncoder

# The reason we don't just inject the new __key_order field into
# the existing dictionary is because then it would add it to the
# end, which when we output to JSON would cause diffs on the line
# above since it would need to add a trailing comma. So we add
# __key_order as the first field which keeps the diffs clean.
def add_key_orders( o ):
  if isinstance( o, dict ):
    new_dict = {}
    if not 'struct' in o and \
       not 'bit_struct' in o and \
       not 'size' in o:
      new_dict['__key_order'] = [k for k in o]
    for k,v in o.items():
      if '__' in k:
        new_dict[k] = v
      else:
        new_dict[k] = add_key_orders( v )
    return new_dict
  return o

def main( args ):
  version = (sys.version_info.major, sys.version_info.minor)
  print( f'python version: {version}' )
  if version < (3, 7):
    raise Exception( 'This script requires python 3.7+ because ' + \
                     'it relies on ordered dictionary keys.' )
  print( f'reading {args.input}...' )
  with open( args.input, 'r' ) as f:
    print( 'parsing json...' )
    j = json.loads( f.read() )
  print( 'adding key orderings...' )
  j = add_key_orders( j )
  print( 'dumping to json...' )
  # We could use an indent of 2 here, but we use 4 because that
  # is what the upstream file uses, and it makes it easier to
  # compare the output of this script with the original file to
  # make sure that this script is doing its job.
  res = json.dumps( j, cls=SMColCompactJSONEncoder, indent=4 )
  print( f'opening output file {args.output}...' )
  with open( args.output, 'w' ) as f:
    print( f'writing json...' )
    f.write( res )
  print( 'FINISHED.' )

if __name__ == '__main__':
  parser = argparse.ArgumentParser()
  parser.add_argument( dest='input', type=str,
      help='''path to input file in JSON format representing
              the structure of COLONY??.SAV files.''' )
  parser.add_argument( dest='output', type=str,
      help='path to output json file.' )
  main( parser.parse_args() )