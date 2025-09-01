-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local colors = require'moon.colors'
local file = require'moon.file'
local printer = require'moon.printer'
local list = require'moon.list'
local logger = require'moon.logger'
local console = require'moon.console'
local mmath = require'moon.math'

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local rep = string.rep
local insert = table.insert
local floor = math.floor
local format = string.format

local NORMAL = colors.ANSI_NORMAL
local GREEN = colors.ANSI_GREEN
local RED = colors.ANSI_RED

local read_file_lines = file.read_file_lines
local tsplit = list.tsplit
local split = list.split
local printfln = printer.printfln
local bar = printer.bar
local clamp = mmath.clamp

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
-- Constants.
-----------------------------------------------------------------
local L1 = 0
local L2 = 1

local REG = 'regulars'
local CAV = 'cavalry'
local ART = 'artillery'

-----------------------------------------------------------------
-- Parameters.
-----------------------------------------------------------------
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

local FORTIFICATION_MUL = {
  none=1.0, --
  stockade=1.0, --
  fort=1.5, --
  fortress=2.0, --
}

local FORTIFICATION_ARTILLERY = {
  none=0, --
  stockade=0, --
  fort=1, --
  fortress=2, --
}

local MIN_UNIT_COUNT = 3
local MAX_UNIT_COUNT = 6

local LEVELS = {
  [L1]={ CAV, ART, REG, REG, REG, REG },
  [L2]={ CAV, CAV, ART, ART, REG, REG },
}

-- When the metric reaches this value then we always get six
-- units and also get bumped to the L2 sequence.
local L2_CUTOFF = 30

-----------------------------------------------------------------
-- Formation computation.
-----------------------------------------------------------------
local function compute_metric( case )
  local metric = 0
  local add = function( to ) metric = metric + to end
  local mul = function( by ) metric = floor( metric * by ) end
  add( 1 )
  for _, unit in ipairs( case.unit_set ) do
    add( assert( UNIT_WEIGHT[unit] ) )
  end
  add( case.muskets // 50 )
  mul( FORTIFICATION_MUL[case.fortification] )
  add( FORTIFICATION_ARTILLERY[case.fortification] )
  return metric
end

local function compute_unit_count( case, metric )
  -- This is a hack; strange that it is needed. Perhaps it is a
  -- bug or strange rounding behavior in the OG. We will not
  -- replicate it.
  if case.fortification == 'fort' or case.fortification ==
      'fortress' then
    if metric == 8 or metric == 9 then return 4 end
  end
  return clamp( metric // 2 + 1, MIN_UNIT_COUNT, MAX_UNIT_COUNT )
end

local function compute_sequence( metric )
  if metric >= L2_CUTOFF then return LEVELS[L2] end
  return LEVELS[L1]
end

local function compute_formation( landed, n_tiles, n_units, seq )
  assert( n_units > 0 )
  assert( n_tiles > 0 )
  assert( n_tiles <= 4 )
  local res = { regulars=0, cavalry=0, artillery=0 }
  if not landed then
    for _ = 1, n_tiles do
      if n_units == 0 then return res end
      n_units = n_units - 1
      res.regulars = res.regulars + 1
    end
  end
  for i = 1, n_units do
    assert( i <= #seq )
    res[seq[i]] = res[seq[i]] + 1
  end
  return res
end

-----------------------------------------------------------------
-- Test runner.
-----------------------------------------------------------------
local function name_for_formation( formation )
  assert( formation.regulars )
  assert( formation.cavalry )
  assert( formation.artillery )
  return format( '%d/%d/%d', formation.regulars,
                 formation.cavalry, formation.artillery )
end

local function compute_bucket( case )
  local metric = compute_metric( case )
  local seq = assert( compute_sequence( metric ) )
  local n_units = assert( compute_unit_count( case, metric ) )
  local landed = case.landed
  assert( landed ~= nil )
  assert( case.n_tiles )
  local formation = compute_formation( landed, case.n_tiles,
                                       n_units, seq )
  return metric, name_for_formation( formation )
end

local num_passed = 0

local function run_test( test )
  local metric, bucket = compute_bucket( test.info )
  local matches = (bucket == test.expected)
  num_passed = num_passed + (matches and 1 or 0);
  if matches and not SHOW_PASSED then return end
  local color = matches and GREEN or RED
  local label = test.label
  if COMPACT_VIEW then
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
  printfln( '%s : %-5s : %s : %s : %s%s%s', label,
            tostring( metric ), bucket, test.expected, color,
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
  elseif #keyvals.unit_set > 0 then
    keyvals.unit_set = split( keyvals.unit_set, '-' )
  else
    keyvals.unit_set = {}
  end
  keyvals.muskets = tonumber( keyvals.muskets )
  keyvals.horses = tonumber( keyvals.horses )
  keyvals.n_tiles = tonumber( keyvals.n_tiles )
  assert( ({ ['true']=true, ['false']=true })[keyvals.landed] )
  keyvals.landed = keyvals.landed == 'true'
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
  print_header()
  for _, test in ipairs( tests ) do run_test( test ) end
  local perfect = num_passed == #tests
  local pc_color = perfect and GREEN or RED
  local passed_color = perfect and GREEN or NORMAL
  if not perfect then print_header() end
  printfln( '%s%d%%%s %spassed%s [%d/%d].', pc_color,
            floor( 100.0 * num_passed / #tests ), NORMAL,
            passed_color, NORMAL, num_passed, #tests )
end

os.exit( main{ ... } )