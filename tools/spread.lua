-- Proof of concept for icon spreading algorithm.
-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local insert = table.insert
local format = string.format
local rep = string.rep
local min = math.min
local max = math.max

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local function print_bar( len )
  len = len or 65
  print( string.rep( '-', len ) )
end

local function print_kv( k, v, indent )
  if indent and #indent > 0 then
    print( indent .. k, ':', v )
  else
    print( k, ':', v )
  end
end

local function print_input( name, spec )
  print( name )
  print_bar()
  print_kv( ' bounds', spec.bounds )
  for i, spread in ipairs( spec.spreads ) do
    print( format( 'spread[%d]', i ) )
    for k, v in pairs( spread ) do print_kv( k, v, '  ' ) end
  end
end

-----------------------------------------------------------------
-- Reporting.
-----------------------------------------------------------------
local function print_output_tbls( name, outputs )
  print( name )
  print_bar()
  for i, output in ipairs( outputs.outputs ) do
    print( format( 'spread[%d]', i ) )
    for k, v in pairs( output ) do
      if k == 'spec' then goto continue end
      print_kv( k, v, '  ' )
      ::continue::
    end
  end
end

local function print_spread( output )
  local image = '|' ..
                    rep( output.spec.char, output.spec.size - 2 ) ..
                    '|'
  local visible_len = min( output.spec.size, output.space )
  local visible = image:sub( 0, visible_len )
  local space_len = max( output.space - output.spec.size, 0 )
  local space = rep( '.', space_len )
  for i = 1, output.count do
    if i < output.count then
      io.write( visible )
      io.write( space )
    else
      io.write( image )
    end
  end
end

local function print_output( output )
  print( 'Output' )
  print_bar( output.spec.bounds )
  print()
  local sep = ''
  for _, spread_output in pairs( output.outputs ) do
    if spread_output.count == 0 then goto continue end
    io.write( sep )
    sep = '..'
    print_spread( spread_output )
    ::continue::
  end
end

-----------------------------------------------------------------
-- Validation.
-----------------------------------------------------------------
local function validate_spec( spec )
  assert( spec.bounds )
  assert( spec.spreads )

  local total_size = 0
  for _, spread_spec in ipairs( spec.spreads ) do
    assert( spread_spec.char )
    assert( spread_spec.size )
    assert( spread_spec.size >= 2 )
    assert( spread_spec.count )
    total_size = total_size + spread_spec.size
  end

  assert( spec.bounds >= total_size )
end

-----------------------------------------------------------------
-- Algorithm.
-----------------------------------------------------------------
local function bounds_for_output( output )
  if output.count == 0 then return 0 end
  local spec = assert( output.spec )
  return (output.count - 1) * output.space + spec.size
end

local function compute_multi( spec )
  local output = { spec=spec, outputs={} }

  -- Initially set all to max count and space.
  for _, spread_spec in ipairs( spec.spreads ) do
    local spread_output = {
      spec=spread_spec,
      space=spread_spec.size + 1,
      count=spread_spec.count,
    }
    insert( output.outputs, spread_output )
  end

  local function total_bounds()
    local total = 0
    for _, spread_output in ipairs( output.outputs ) do
      total = total + bounds_for_output( spread_output )
    end
    total = total + 2 * max( #output.outputs - 1, 0 )
    return total
  end

  local function decrement_spacing_for_largest()
    local largest = nil
    local largest_idx = nil
    local largest_bounds = 0
    for i, spread_output in ipairs( output.outputs ) do
      local bounds = bounds_for_output( spread_output )
      if bounds > largest_bounds then
        largest_bounds = bounds
        largest = spread_output
        largest_idx = i
      end
    end
    assert( largest )
    if largest.space == 1 then
      if largest.count == 1 then return false end
      largest.count = largest.count - 1
      return true
    end
    print( format( 'decrementing %d', largest_idx ) )
    largest.space = largest.space - 1
    return true
  end

  print()
  print( 'Decrement loop:' )
  print_bar()
  while total_bounds() > spec.bounds do
    assert( decrement_spacing_for_largest() )
  end

  assert( total_bounds() <= spec.bounds )

  return output
end

-----------------------------------------------------------------
-- Main.
-----------------------------------------------------------------
local function main()
  local SPEC1 = { char='X', count=5, size=5 }
  local SPEC2 = { char='O', count=5, size=5 }
  local SPEC3 = { char='$', count=5, size=5 }
  local spec = { bounds=65, spreads={ SPEC1, SPEC2, SPEC3 } }

  validate_spec( spec )
  print_input( 'Input', spec )
  local output = compute_multi( spec )
  print()
  print_output_tbls( 'Output', output )
  print()
  print_output( output )
  return 0
end

return main()