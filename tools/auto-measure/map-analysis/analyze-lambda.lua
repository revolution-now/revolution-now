local Q = require( 'lib.query' )

return function( json )
  print( json.HEADER.colonize )
end