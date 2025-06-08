-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local colors = require'moon.colors'
local file = require'moon.file'
local printer = require'moon.printer'
local list = require'moon.list'

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local rep = string.rep
local insert = table.insert
local concat = table.concat
local floor = math.floor

local NORMAL = colors.ANSI_NORMAL
local GREEN = colors.ANSI_GREEN
local RED = colors.ANSI_RED

local read_file_lines = file.read_file_lines
local tsplit = list.tsplit
local split = list.split
local printfln = printer.printfln

-----------------------------------------------------------------
-- Buckets.
-----------------------------------------------------------------
local BUCKET_MULT = 1.0

local BUCKETS = {
  { start=489 * BUCKET_MULT, name='2/2/2' }, --
  { start=212 * BUCKET_MULT, name='4/1/1' }, --
  { start=130 * BUCKET_MULT, name='3/1/1' }, --
  { start=95 * BUCKET_MULT, name='2/1/1' }, --
  { start=0 * BUCKET_MULT, name='2/1/0' }, --
}

local BUCKET_COUNTS = {
  { name='2/2/2', max=1000 }, --
  { name='4/1/1', max=14 }, --
  { name='3/1/1', max=4 }, --
  { name='2/1/1', max=4 }, --
  { name='2/1/0', max=3 }, --
}

-----------------------------------------------------------------
-- Units
-----------------------------------------------------------------
local units = {
  soldier={
    name='soldier',
    combat=2, --
    veteran=false,
  },

  dragoon={
    name='dragoon',
    combat=3, --
    veteran=false,
  },

  veteran_soldier={
    name='veteran_soldier',
    combat=2, --
    veteran=false,
  },

  veteran_dragoon={
    name='veteran_dragoon',
    combat=3, --
    veteran=false,
  },

  continental_army={
    name='continental_army',
    combat=4, --
    veteran=false,
  },

  continental_cavalry={
    name='continental_cavalry',
    combat=5, --
    veteran=false,
  },

  artillery={
    name='artillery',
    combat=5, --
    veteran=false,
  },

  damaged_artillery={
    name='damaged_artillery',
    combat=3, --
    veteran=false,
  },
}

-----------------------------------------------------------------
-- Strength computation.
-----------------------------------------------------------------
local function bonus_for_fortification( _, fortification )
  assert( fortification )
  local from_colony = {
    none=0.5,
    stockade=1.0,
    fort=1.5,
    fortress=2.0,
  }
  local res = assert( from_colony[fortification] )
  -- if orders == 'fortified' then res = res + .5 end
  res = 1 + res
  return res
end

local function additive_fudge_per_unit( case )
  local res = {
    conquistador=10, --
    viceroy=0, --
  }
  return assert( res[case.difficulty] )
end

local function unit_strength( unit, case )
  local strength = unit.combat
  local multiply = function( factor )
    strength = strength * factor
  end
  local add = function( term ) strength = strength + term end
  multiply( 8 )
  if unit.veteran then multiply( 1.5 ) end
  local fortification_bonus = bonus_for_fortification(
                                  case.orders, case.fortification )
  multiply( fortification_bonus )
  -- multiply( .5 ) -- tory penalty
  add( additive_fudge_per_unit( case ) )
  -- printfln( 'unit %s strength=%d', unit.name, strength )
  return strength
end

local function units_strength( case )
  local strength = 0
  local add = function( term ) strength = strength + term end
  local multiply = function( factor )
    strength = strength * factor
  end
  for _, unit in ipairs( case.unit_set ) do
    add( unit_strength( units[unit], case ) )
  end
  -- assert( type( case.muskets ) == 'number', format(
  --             'case.muskets has type %s', type( case.muskets ) ) )
  add( 10 * (case.muskets / 50) )
  -- multiply( .7 )
  return strength
end

-----------------------------------------------------------------
-- Test runner.
-----------------------------------------------------------------
local function bucket_for( strength )
  for idx, bucket in ipairs( BUCKETS ) do
    if strength >= bucket.start then return idx end
  end
  error( 'bucket not found for strength=' .. strength )
end

local function compute_bucket( case )
  local strength = units_strength( case )
  local bucket = bucket_for( strength )
  if #case.unit_set > BUCKET_COUNTS[bucket].max then
    for i, _ in ipairs( BUCKET_COUNTS ) do
      bucket = i
      if not BUCKET_COUNTS[i + 1] then break end
      if #case.unit_set > BUCKET_COUNTS[i + 1].max then
        break
      end
    end
  end
  assert( #case.unit_set <= BUCKET_COUNTS[bucket].max )
  return strength, assert( BUCKETS[bucket].name )
end

local LABEL_LEN = 220

local function print_range( name, rg )
  printfln( 'range for %s:', name )
  for _, bucket in ipairs( BUCKETS ) do
    if rg[bucket.name] then
      printfln( '  %s: (%d, %d)', bucket.name,
                floor( rg[bucket.name].min ),
                floor( rg[bucket.name].max ) )
    end
  end
end

local ranges_passed = {}
local ranges_failed = {}
local ranges_all = {}
local num_passed = 0

local function run_test( test )
  local strength, bucket = compute_bucket( test.info )
  local matches = (bucket == test.expected)
  num_passed = num_passed + (matches and 1 or 0);
  local function add_range( tbl )
    local b = test.expected
    tbl[b] = tbl[b] or { min=10 ^ 6, max=-10 ^ 6 }
    tbl[b].min = math.min( tbl[b].min, strength )
    tbl[b].max = math.max( tbl[b].max, strength )
  end
  if matches then
    add_range( ranges_passed )
  else
    add_range( ranges_failed )
  end
  add_range( ranges_all )
  -- if matches then return end
  local color = matches and GREEN or RED
  local label = { test.label }
  insert( label, rep( ' ', LABEL_LEN - #test.label ) )
  printfln( '%s : %-5s : %s : %s : %s%s%s', concat( label ),
            tostring( strength ), bucket, test.expected, color,
            matches, NORMAL )
end

-- The line is of the form: 'xxx=yyy|aaa=bbb|ccc=ddd\thello=2/1/0'.
local function parse_test_case( line )
  local label, result = tsplit( line, '\t' )
  local keyval_pairs = split( label, '|' )
  local keyvals = {}
  for _, pair in ipairs( keyval_pairs ) do
    local k, v = tsplit( pair, '=' )
    keyvals[k] = v
  end
  local ref_selection, delivery = tsplit( result, '=' )
  assert( ref_selection == 'ref_selection' )
  assert( keyvals.unit_set )
  if keyvals.unit_set:match( '%*' ) then
    local unit, count = tsplit( keyvals.unit_set, '*' )
    count = assert( tonumber( count ) )
    keyvals.unit_set = {}
    for _ = 1, count do insert( keyvals.unit_set, unit ) end
    assert( #keyvals.unit_set == count )
  else
    keyvals.unit_set = split( keyvals.unit_set, '-' )
  end
  keyvals.muskets = tonumber( keyvals.muskets )
  keyvals.horses = tonumber( keyvals.horses )
  return { info=keyvals, label=label, expected=delivery }
end

local function create_test_cases()
  local lines = read_file_lines(
                    'auto-measure/ref-selection/results.txt' )
  local tests = {}
  for _, line in ipairs( lines ) do
    local test = parse_test_case( line )
    -- FIXME: skips
    if test.info.muskets > 0 or test.info.horses > 0 then
      goto continue
    end
    if test.info.fortification ~= 'none' then goto continue end
    if test.info.orders == 'none' then goto continue end
    insert( tests, parse_test_case( line ) )
    ::continue::
  end
  return tests
end

local tests = create_test_cases()

-- print( to_json_oneline( tests ) )

local buf = rep( ' ', LABEL_LEN - 4 )
local bar = rep( '-', LABEL_LEN + 32 )

local function print_header()
  printfln( 'Test%s : %-5s : %s  : %s  : %s', buf, 'S', 'Have',
            'Need', 'OK' )
end

print_header()
print( bar )

for _, test in ipairs( tests ) do run_test( test ) end

print( bar )
print_header()

print_range( 'passed', ranges_passed )
print_range( 'failed', ranges_failed )
print_range( 'all', ranges_all )

print()
printfln( '%d%% passed.', floor( 100.0 * num_passed / #tests ) )