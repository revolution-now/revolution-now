#!/usr/bin/env python3
'''
Pretty-prints JSON in a way that preserves the key order in ob-
jects. This is possible because python 3.7+ has dictionaries that
preserve key insertion order, which the json module automatically
inherits and thus does the same.
'''
import sys
import argparse
import json

def main( args ):
  major, minor = (sys.version_info.major, sys.version_info.minor)
  if (major, minor) < (3, 7):
    raise Exception( 'This script requires python 3.7+ because ' + \
                     'it relies on ordered dictionary keys.' )
  with open( args.input, 'r' ) as f:
    j = json.loads( f.read() )
  res = json.dumps( j, indent=2 )
  with open( args.output, 'w' ) as f:
    f.write( res )

if __name__ == '__main__':
  parser = argparse.ArgumentParser()
  parser.add_argument( dest='input', type=str, help='input JSON file.' )
  parser.add_argument( dest='output', type=str, help='output JSON file.' )
  main( parser.parse_args() )