local EMPIRICAL_FILE = 'empirical/bbmm.json'
local GENERATED_FILE = 'generated/bbmm.data.json'
local INPUTS_FILE = 'generated/bbmm.inputs.json'

FORMATION_ORDER = { 'hills', 'mountains', 'clearing' }

local BIOME_ORDERING = {
  'savannah', --
  'grassland', --
  'tundra', --
  'plains', --
  'prairie', --
  'desert', --
  'swamp', --
  'marsh', --
  'arctic', --
}

local json = require'moon.json'

local empirical = json.read_file( EMPIRICAL_FILE )
local generated = json.read_file( GENERATED_FILE )
local inputs = json.read_file( INPUTS_FILE )

-- print()
-- print( 'empirical:' )
-- for _, biome in ipairs( BIOME_ORDERING ) do
--   local v = empirical.density_on_biome[focus][biome]
--   print( biome, v )
-- end
--
-- print()
-- print( 'generated:' )
-- for _, biome in ipairs( BIOME_ORDERING ) do
--   local v = generated.density_on_biome[focus][biome]
--   print( biome, v )
-- end
--
-- print()
-- print( 'inputs:' )
-- for _, biome in ipairs( BIOME_ORDERING ) do
--   local v = inputs.density_on_biome[focus][biome]
--   print( biome, v )
-- end
--
-- print()
-- print( 'results:' )

for _, focus in ipairs( FORMATION_ORDER ) do
  print()
  for _, biome in ipairs( BIOME_ORDERING ) do
    local e = empirical.density_on_biome[focus][biome]
    local g = generated.density_on_biome[focus][biome]
    local i = inputs.density_on_biome[focus][biome]
    local p = 100 * g / e;
    if biome == 'arctic' then
      i = 0
      e = 1
      g = 1
      p = 100
    end
    print( string.format( '        %-23s%.5f  # %5s%%',
                          string.format( '%s.probability:', biome ),
                          i * e / g, string.format( '%.1f', p ) ) )
  end
end