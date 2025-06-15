-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local colors = require'moon.colors'
local file = require'moon.file'
local printer = require'moon.printer'
local list = require'moon.list'
local logger = require'moon.logger'
local console = require'moon.console'

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local rep = string.rep
local insert = table.insert
local floor = math.floor
local huge = math.huge
local ceil = math.ceil
local format = string.format

local NORMAL = colors.ANSI_NORMAL
local GREEN = colors.ANSI_GREEN
local RED = colors.ANSI_RED

local read_file_lines = file.read_file_lines
local tsplit = list.tsplit
local split = list.split
local printfln = printer.printfln
local bar = printer.bar
local min, max = math.min, math.max

-----------------------------------------------------------------
-- Global Init.
-----------------------------------------------------------------
logger.level = logger.levels.INFO

-----------------------------------------------------------------
-- View config.
-----------------------------------------------------------------
local COMPACT_VIEW = not console.is_wide_terminal()
local LABEL_LEN = COMPACT_VIEW and 120 or 220
local SHOW_PASSED = false

-----------------------------------------------------------------
-- Parameters.
-----------------------------------------------------------------
local BUCKETS = {
  -- LuaFormatter off
  none={
    { start=  30, name='2/2/2' },
    { start=  10, name='4/1/1' },
    { start=   8, name='3/1/1' },
    { start=   6, name='2/1/1' },
    { start=   4, name='2/1/0' },
    { start=   2, name='2/1/0' },
    { start=   0, name='2/1/0' },
  },
  stockade={
    { start=  30, name='2/2/2' },
    { start=  10, name='4/1/1' },
    { start=   8, name='3/1/1' },
    { start=   6, name='2/1/1' },
    { start=   4, name='2/1/0' },
    { start=   2, name='2/1/0' },
    { start=   0, name='2/1/0' },
  },
  fort={
    { start=  20, name='2/2/2' },
    { start=  10, name='4/1/1' },
    { start=   8, name='4/1/1' },
    { start=   6, name='4/1/1' },
    { start=   4, name='2/1/1' },
    { start=   2, name='2/1/0' },
    { start=   0, name='2/1/0' },
  },
  fortress={
    { start=  14, name='2/2/2' },
    { start=  10, name='4/1/1' },
    { start=   8, name='4/1/1' },
    { start=   6, name='4/1/1' },
    { start=   4, name='4/1/1' },
    { start=   2, name='2/1/1' },
    { start=   0, name='2/1/0' },
  },
  -- LuaFormatter on
}

for _, bucket in pairs( BUCKETS ) do
  insert( bucket, { start=0, name='1/1/1' } )
end

local UNIT_WEIGHT = {
  soldier=2,
  veteran_soldier=2,
  dragoon=2,
  veteran_dragoon=2,

  continental_army=4,
  continental_cavalry=4,
  damaged_artillery=4,

  artillery=6,
}

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local FORTIFICATION_ORDER = {
  'none', 'stockade', 'fort', 'fortress',
}

local function idx_for_bucket( case, target )
  for i, bucket in ipairs( BUCKETS[case.fortification] ) do
    if bucket.name == target then return i end
  end
  error( format( 'cannot find bucket index for bucket %s',
                 tostring( target ) ) )
end

-----------------------------------------------------------------
-- metric computation.
-----------------------------------------------------------------
local function compute_metric( case )
  local metric = 0
  local add = function( term ) metric = metric + term end
  add( 1 )
  for _, unit in ipairs( case.unit_set ) do
    add( UNIT_WEIGHT[unit] )
  end
  add( case.muskets // 50 )
  return metric
end

-----------------------------------------------------------------
-- Test runner.
-----------------------------------------------------------------
local function bucket_for( case, metric )
  local buckets = assert( BUCKETS[case.fortification] )
  for _, bucket in ipairs( buckets ) do
    if metric >= bucket.start then return bucket end
  end
  error( 'bucket not found for metric=' .. metric )
end

local function compute_bucket( case )
  local metric = compute_metric( case )
  local bucket = bucket_for( case, metric )
  assert( bucket )
  if assert( bucket.name ) == '2/1/0' and case.already_landed then
    return metric, '1/1/1'
  end
  return metric, assert( bucket.name )
end

local num_passed = 0

local function run_test( test, ranges_out )
  local metric, bucket = compute_bucket( test.info )
  local matches = (bucket == test.expected)
  local got_bucket_idx = assert(
                             idx_for_bucket( test.info, bucket ) )
  local need_bucket_idx = assert(
                              idx_for_bucket( test.info,
                                              test.expected ) )
  local rg = ranges_out[test.info.fortification][need_bucket_idx]
  rg.min = min( rg.min, metric )
  rg.max = max( rg.max, metric )
  num_passed = num_passed + (matches and 1 or 0);
  if matches and not SHOW_PASSED then return end
  local color = matches and GREEN or RED
  local label = test.label
  if COMPACT_VIEW then
    label = label:gsub( 'already_landed', 'landed' )
    label = label:gsub( 'difficulty', 'diff' )
    label = label:gsub( 'conquistador', 'conq' )
    label = label:gsub( 'fortification', 'fortn' )
    label = label:gsub( 'fortified', 'fortd' )
    label = label:gsub( 'orders', 'or' )
    label = label:gsub( 'unit_set', 'us' )
    label = label:gsub( 'veteran_', 'v_' )
    label = label:gsub( 'continental_', 'c_' )
    label = label:gsub( 'damaged_', 'd_' )
    label = label:gsub( 'artillery', 'art' )
    label = label:gsub( 'horses', 'hs' )
    label = label:gsub( 'muskets', 'ms' )
    label = label:gsub( 'dragoon', 'dg' )
    label = label:gsub( 'soldier', 'sl' )
  end
  label = label .. rep( ' ', LABEL_LEN - #label )
  local direction = (got_bucket_idx < need_bucket_idx) and '>' or
                        (got_bucket_idx > need_bucket_idx) and
                        '<' or ' '
  printfln( '%s : %-5s : %s : %s : %s : %s%s%s', label,
            tostring( metric ), bucket, test.expected, direction,
            color, matches, NORMAL )
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
  assert(
      ({ ['true']=true, ['false']=true })[keyvals.already_landed] )
  keyvals.already_landed = keyvals.already_landed == 'true'
  return { info=keyvals, label=label, expected=delivery }
end

local function skip( _ )
  return false --
end

local function create_test_cases()
  local lines = read_file_lines(
                    'auto-measure/ref-selection/results.txt' )
  local tests = {}
  for _, line in ipairs( lines ) do
    local test = parse_test_case( line )
    if skip( test.info ) then goto continue end
    insert( tests, test )
    ::continue::
  end
  return tests
end

local function print_header()
  local buf = rep( ' ', LABEL_LEN - 4 )
  bar()
  printfln( 'Test%s : %-5s : %s  : %s  : %s : %s', buf, 'S',
            'Have', 'Need', 'D', 'OK' )
  bar()
end

local function main( _ )
  local tests = create_test_cases()
  local ranges = {}
  for k, _ in pairs( BUCKETS ) do
    ranges[k] = ranges[k] or {}
    for _, _ in ipairs( BUCKETS.none ) do
      insert( ranges[k], { min=huge, max=-huge } )
    end
  end
  print_header()
  for _, test in ipairs( tests ) do run_test( test, ranges ) end
  local perfect = num_passed == #tests
  local pc_color = perfect and GREEN or RED
  local passed_color = perfect and GREEN or NORMAL
  if not perfect then print_header() end
  print()
  printfln( '%s%d%%%s %spassed%s [%d/%d].', pc_color,
            floor( 100.0 * num_passed / #tests ), NORMAL,
            passed_color, NORMAL, num_passed, #tests )
  print()
  print( 'local BUCKETS = {' )
  print( '  -- LuaFormatter off' )
  for _, ftn in ipairs( FORTIFICATION_ORDER ) do
    printfln( '  %s={', ftn )
    for i, bucket in ipairs( BUCKETS[ftn] ) do
      if ranges[ftn][i] then
        local v = ranges[ftn][i]
        local l = v.min
        local r = v.max
        -- if i == #BUCKETS[ftn] then l = 0 end
        l = (v.min == huge) and 'huge' or l
        r = (v.max == -huge) and 'huge' or r
        printfln( '    { start=%4s, finish=%4s, name=\'%s\' },',
                  l, r, bucket.name )
      end
    end
    print( '  },' )
  end
  print( '  -- LuaFormatter on' )
  print( '}' )
end

os.exit( main{ ... } )