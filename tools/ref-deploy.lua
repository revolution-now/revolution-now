-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local colors = require'moon.colors'

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local format = string.format

local NORMAL = colors.ANSI_NORMAL
local GREEN = colors.ANSI_GREEN
-- local BLUE = colors.ANSI_BLUE
local RED = colors.ANSI_RED
-- local YELLOW = colors.ANSI_YELLOW
-- local MAGENTA = colors.ANSI_MAGENTA

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local function printfln( fmt, ... ) print( format( fmt, ... ) ) end

-----------------------------------------------------------------
-- Buckets.
-----------------------------------------------------------------
-- For adding 10.
local BUCKETS = {
  { start=351, name='_2_2_2' }, --
  { start=141, name='_4_1_1' }, --
  { start=106, name='_3_1_1' }, --
  { start=82, name='_2_1_1' }, --
  { start=0, name='_2_1_0' }, --
}

-- local BUCKETS = {
--   { start=360, name='_2_2_2' }, --
--   { start=144, name='_4_1_1' }, --
--   { start=96, name='_3_1_1' }, --
--   { start=72, name='_2_1_1' }, --
--   { start=0, name='_2_1_0' }, --
-- }

-----------------------------------------------------------------
-- Units
-----------------------------------------------------------------
local soldier = {
  name='sl',
  combat=2, --
  veteran=false,
}

local dragoon = {
  name='dragoon',
  combat=3, --
  veteran=false,
}

local v_soldier = {
  name='v_soldier',
  combat=2, --
  veteran=true,
}

local v_dragoon = {
  name='v_dragoon',
  combat=3, --
  veteran=true,
}

local c_army = {
  name='c_army',
  combat=4, --
  veteran=false,
}

local c_cavalry = {
  name='c_cavalry',
  combat=5, --
  veteran=false,
}

local regular = {
  name='regular',
  combat=5, --
  veteran=false,
}

local cavalry = {
  name='cavalry',
  combat=6, --
  veteran=false,
}

local artillery = {
  name='artillery',
  combat=6, --
  veteran=false,
}

local d_artillery = {
  name='d_art',
  combat=4, --
  veteran=false,
}

-----------------------------------------------------------------
-- Strength computation.
-----------------------------------------------------------------
local function unit_strength( unit, bonuses )
  local strength = unit.combat
  local multiply = function( factor )
    strength = strength * factor
  end
  local add = function( term ) strength = strength + term end
  multiply( 8 )
  if unit.veteran then multiply( 1.5 ) end
  local fortification_bonus = (bonuses.colony and .5 or 0) + 0
  multiply( 1 + fortification_bonus )
  add( 10 )
  -- printfln( 'unit %s strength=%d', unit.name, strength )
  return strength
end

local function units_strength( units, bonuses )
  local strength = 0
  local add = function( term ) strength = strength + term end
  for _, unit in ipairs( units ) do
    add( unit_strength( unit, bonuses ) )
  end
  -- add( 10 )
  return strength
end

local function bucket_for( strength )
  for _, bucket in ipairs( BUCKETS ) do
    if strength >= bucket.start then return bucket.name end
  end
  error( 'bucket not found for strength=' .. strength )
end

local function compute_bucket( case )
  local strength = units_strength( case.units, case.bonuses )
  local bucket = bucket_for( strength )
  return strength, bucket
end

local function make_case( units )
  return { units=units, bonuses={ colony=true } }
end

local function count_of( unit, n )
  assert( unit )
  local units = {}
  for _ = 1, n do table.insert( units, unit ) end
  return make_case( units )
end

local function run_test( test )
  local strength, bucket = compute_bucket( test.input )
  local sep = '|'
  local label = sep
  for _, unit in ipairs( test.input.units ) do
    label = label .. unit.name .. sep
  end
  local matches = (bucket == test.expected)
  local color = matches and GREEN or RED
  printfln( '%-55s : %-5s : %s : %s : %s%s%s', label,
            tostring( strength ), bucket, test.expected, color,
            matches, NORMAL )
end

local tests = {
  -- cavalry
  { input=count_of( cavalry, 5 ), expected='_2_2_2' },
  { input=count_of( cavalry, 4 ), expected='_4_1_1' },
  { input=count_of( cavalry, 3 ), expected='_4_1_1' },
  { input=count_of( cavalry, 2 ), expected='_4_1_1' },
  { input=count_of( cavalry, 1 ), expected='_2_1_1' },

  -- artillery
  { input=count_of( artillery, 5 ), expected='_2_2_2' },
  { input=count_of( artillery, 4 ), expected='_4_1_1' },
  { input=count_of( artillery, 3 ), expected='_4_1_1' },
  { input=count_of( artillery, 2 ), expected='_4_1_1' },
  { input=count_of( artillery, 1 ), expected='_2_1_1' },

  -- regular
  { input=count_of( regular, 5 ), expected='_4_1_1' },
  { input=count_of( regular, 4 ), expected='_4_1_1' },
  { input=count_of( regular, 3 ), expected='_4_1_1' },
  { input=count_of( regular, 2 ), expected='_3_1_1' },
  { input=count_of( regular, 1 ), expected='_2_1_0' },

  -- c_cavalry
  { input=count_of( c_cavalry, 5 ), expected='_4_1_1' },
  { input=count_of( c_cavalry, 4 ), expected='_4_1_1' },
  { input=count_of( c_cavalry, 3 ), expected='_4_1_1' },
  { input=count_of( c_cavalry, 2 ), expected='_3_1_1' },
  { input=count_of( c_cavalry, 1 ), expected='_2_1_0' },

  -- c_army
  { input=count_of( c_army, 5 ), expected='_4_1_1' },
  { input=count_of( c_army, 4 ), expected='_4_1_1' },
  { input=count_of( c_army, 3 ), expected='_4_1_1' },
  { input=count_of( c_army, 2 ), expected='_3_1_1' },
  { input=count_of( c_army, 1 ), expected='_2_1_0' },

  -- d_artillery
  { input=count_of( d_artillery, 5 ), expected='_4_1_1' },
  { input=count_of( d_artillery, 4 ), expected='_4_1_1' },
  { input=count_of( d_artillery, 3 ), expected='_4_1_1' },
  { input=count_of( d_artillery, 2 ), expected='_3_1_1' },
  { input=count_of( d_artillery, 1 ), expected='_2_1_0' },

  -- v_dragoon
  { input=count_of( v_dragoon, 5 ), expected='_4_1_1' },
  { input=count_of( v_dragoon, 4 ), expected='_4_1_1' },
  { input=count_of( v_dragoon, 3 ), expected='_4_1_1' },
  { input=count_of( v_dragoon, 2 ), expected='_3_1_1' },
  { input=count_of( v_dragoon, 1 ), expected='_2_1_0' },

  -- v_soldier
  { input=count_of( v_soldier, 5 ), expected='_4_1_1' },
  { input=count_of( v_soldier, 4 ), expected='_3_1_1' },
  { input=count_of( v_soldier, 3 ), expected='_2_1_1' },
  { input=count_of( v_soldier, 2 ), expected='_2_1_0' },
  { input=count_of( v_soldier, 1 ), expected='_2_1_0' },

  -- dragoon
  { input=count_of( dragoon, 5 ), expected='_4_1_1' },
  { input=count_of( dragoon, 4 ), expected='_3_1_1' },
  { input=count_of( dragoon, 3 ), expected='_2_1_1' },
  { input=count_of( dragoon, 2 ), expected='_2_1_0' },
  { input=count_of( dragoon, 1 ), expected='_2_1_0' },

  -- soldier
  { input=count_of( soldier, 15 ), expected='_2_2_2' },
  { input=count_of( soldier, 14 ), expected='_4_1_1' },
  { input=count_of( soldier, 13 ), expected='_4_1_1' },
  { input=count_of( soldier, 5 ), expected='_4_1_1' },
  { input=count_of( soldier, 4 ), expected='_3_1_1' },
  { input=count_of( soldier, 3 ), expected='_2_1_1' },
  { input=count_of( soldier, 2 ), expected='_2_1_0' },
  { input=count_of( soldier, 1 ), expected='_2_1_0' }, --
  --
  -- mix
  -- LuaFormatter off
  { input=make_case{ soldier,soldier,v_dragoon,v_dragoon }, expected='_4_1_1' },
  { input=make_case{ soldier,dragoon,dragoon,v_dragoon }, expected='_4_1_1' },
  { input=make_case{ dragoon,dragoon,dragoon,dragoon }, expected='_3_1_1' },
  { input=make_case{ dragoon,dragoon,dragoon,v_soldier }, expected='_3_1_1' },
  { input=make_case{ dragoon,dragoon,v_soldier,v_soldier }, expected='_3_1_1' },
  { input=make_case{ dragoon,v_soldier,v_soldier,v_soldier }, expected='_3_1_1' },
  { input=make_case{ soldier,v_soldier,v_soldier,v_dragoon }, expected='_4_1_1' },
  { input=make_case{ v_soldier,v_soldier,v_soldier,v_soldier }, expected='_3_1_1' },
  { input=make_case{ soldier,soldier,v_soldier,v_dragoon }, expected='_4_1_1' },
  -- LuaFormatter on
}

printfln( '%-55s : %-5s : %s    : %s    : %s', 'Units', 'S',
          'Res', 'Exp', 'OK' )

for _, test in ipairs( tests ) do run_test( test ) end