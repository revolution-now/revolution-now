import json

# Seeks to encode a JSON file in the style of the upstream
# smcol_saves_utility repo's SAV file structure document.
class SMColCompactJSONEncoder( json.JSONEncoder ):
  '''Maximum width of a list that might be put on a single line.'''
  MAX_LIST_WIDTH = 80

  def __init__( self, *args, **kwargs ):
    # Using this class without indentation is pointless.
    if kwargs.get( 'indent' ) is None:
      kwargs['indent'] = 2
    super().__init__( *args, **kwargs )
    self.indentation_level = 0

  def indent_str( self ) -> str:
    return ' ' * (self.indentation_level * self.indent)

  def encode( self, o ):
    if isinstance( o, (list, tuple) ): return self.encode_list( o )
    if isinstance( o, dict ):          return self.encode_object( o )
    return json.dumps( o, indent=self.indent )

  def encode_list( self, o ):
    if self.put_on_single_line( o ):
      return '[' + ', '.join( self.encode( el ) for el in o ) + ']'
    self.indentation_level += 1
    output = [self.indent_str() + self.encode( el ) for el in o]
    self.indentation_level -= 1
    return '[\n' + ',\n'.join( output ) + '\n' + self.indent_str() + ']'

  def encode_object( self, o ):
    if not o: return '{}'
    if self.put_on_single_line( o ):
      return (
        '{' +
        ', '.join( f'{self.encode( k )}: {self.encode( el )}'
                   for k,el in o.items() ) +
        '}'
      )
    def maybe_newline( x ):
      return '\n' if (isinstance( x, dict ) and \
                    not self.put_on_single_line( x )) \
                  else ' '
    self.indentation_level += 1
    output = []
    for k,v in o.items():
      key_str = json.dumps( k )
      val_str = self.encode( v )
      indent = self.indent_str()
      spacing = maybe_newline( v )
      output.append( f'{indent}{key_str}:{spacing}{val_str}' )
    self.indentation_level -= 1
    return self.indent_str() + '{\n' + \
             ',\n'.join( output ) + '\n' + \
           self.indent_str() + '}'

  def put_on_single_line( self, o ):
    if isinstance( o, dict ):
      return o.get( 'size' ) is not None and \
         not o.get( 'struct' ) and \
         not o.get( 'bit_struct' )
    elif isinstance( o, list ):
      return len( str( o ) ) - 2 <= self.MAX_LIST_WIDTH
    return False
