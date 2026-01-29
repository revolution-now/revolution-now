-- Generates biome distributions for fitting data.
-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local csv = require( 'ftcsv' )
local tbl = require( 'moon.tbl' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local deep_copy = tbl.deep_copy

local format = string.format
local max = math.max
local exp = math.exp
local abs = math.abs

-----------------------------------------------------------------
-- Spec.
-----------------------------------------------------------------
local MAX_ITERATIONS = 10000

-- Ignore the curve this close to the edges since it is noisy.
local BUFFER = 5

local RATIO_TOLERANCE = 0.002

local MODES = {
  'bbtm', --
  'bbbm', --
  'bbmt', --
  'bbmb', --
  'bbmm', --
}

-- Some of these are just starter values; they will be adjusted
-- on each iteration to fit the experimental data.
local SPEC_INIT = {
  savannah={ weight=1.0, curve={ c=35.5, w=10, sub=.025 } },
  grassland={ weight=1.0, curve={ c=35.5, w=10, sub=0 } },
  tundra={ weight=1.0, curve={ c=60, w=10, sub=0 } },
  plains={ weight=1.0, curve={ c=60, w=10, sub=0 } },
  prairie={ weight=1.0, curve={ c=60, w=10, sub=0 } },
  desert={ weight=1.0, curve={ c=35.5, w=10, sub=0 } },
  swamp={ weight=1.0, curve={ c=35.5, w=10, sub=.025 } },
  marsh={ weight=1.0, curve={ c=35.5, w=10, sub=0 } },
}

local SPEC

local ORDERING = {
  'savannah', --
  'grassland', --
  'tundra', --
  'plains', --
  'prairie', --
  'desert', --
  'swamp', --
  'marsh', --
}

local function print_spec( emit )
  for _, curve in ipairs( ORDERING ) do
    local spec = assert( SPEC[curve] )
    emit( '%s {\n', curve )
    emit( '  weight: %.3f\n', assert( spec.weight ) )
    emit( '  center: %.3f\n', assert( spec.curve.c ) )
    emit( '  width: %.3f\n', assert( spec.curve.w ) )
    emit( '  sub: %.3f\n', assert( spec.curve.sub ) )
    emit( '}\n' )
  end
end

-----------------------------------------------------------------
-- GNU plot template.
-----------------------------------------------------------------
local GNUPLOT_FILE_TEMPLATE = [[
#!/usr/bin/env -S gnuplot -p
set title "Terrain Distribution ({{MODE}})"
set datafile separator ","
set key outside right
set grid
set xlabel "Map Row (Y)"
set ylabel "value"

# Use the first row as column headers for titles.
set key autotitle columnhead

set yrange [0:0.7]

# Plot: x is column 1, then plot columns 2..N as separate lines.
plot for [col=2:*] "{{CSV}}" using 1:col with lines lw 2
]]

local function print_gnuplot_file( emit, mode )
  assert( emit )
  assert( mode )
  local body = GNUPLOT_FILE_TEMPLATE
  local subs = {
    { key='{{CSV}}', val=format( '%s.csv', mode ) }, --
    { key='{{MODE}}', val=mode }, --
  }
  for _, p in ipairs( subs ) do
    body = body:gsub( assert( p.key ), assert( p.val ) )
  end
  emit( '%s', body )
end

-----------------------------------------------------------------
-- Curves.
-----------------------------------------------------------------
local function gaussian( params, y )
  local center = assert( params.c )
  local width = assert( params.w )
  local sub = assert( params.sub )
  local value = exp( -((y - center) / (width / 1.4142)) ^ 2 )
  value = value - sub
  return value
end

local function evaluate_curve( curve, y )
  return assert( gaussian( curve, y ) )
end

-----------------------------------------------------------------
-- Printing.
-----------------------------------------------------------------
local function printfln( fmt, ... ) print( format( fmt, ... ) ) end

local function print_csv( emit, graphs )
  -- Emit header.
  emit( 'y' )
  for _, name in ipairs( ORDERING ) do emit( ',%s', name ) end
  emit( '\n' )
  for y = 1, 70 do
    emit( '%d', y )
    for _, name in ipairs( ORDERING ) do
      emit( ',%f', assert( graphs[name][y] ) )
    end
    emit( '\n' )
  end
end

local function print_metrics( emit, metrics, prefix )
  assert( emit )
  assert( metrics )
  prefix = prefix or ''
  emit( '%s%-10s', prefix, 'curve' )
  local metric_names = { 'area', 'avg', 'stddev' }
  for _, name in ipairs( metric_names ) do emit( '%10s', name ) end
  emit( '\n' )
  for _, curve in ipairs( ORDERING ) do
    emit( '%s%-10s', prefix, curve )
    for _, metric_name in ipairs( metric_names ) do
      local curve_metrics = assert( metrics[curve] )
      emit( '%10s', format( '%.4f',
                            assert( curve_metrics[metric_name] ) ) )
    end
    emit( '\n' )
  end
end

local function print_metrics_stdout( metrics )
  assert( metrics )
  local function emit( fmt, ... ) io.write( format( fmt, ... ) ) end
  print_metrics( emit, metrics )
end

-----------------------------------------------------------------
-- Metrics.
-----------------------------------------------------------------
local function compute_metrics( graphs )
  local metrics = {}
  for _, name in ipairs( ORDERING ) do
    local graph = assert( graphs[name] )
    local moment0 = 0.0
    local moment1 = 0.0
    local moment2 = 0.0
    for y = 36, 70 - BUFFER do
      local delta = y - 35.5
      moment0 = moment0 + assert( graph[y] ) * delta ^ 0
      moment1 = moment1 + assert( graph[y] ) * delta ^ 1
      moment2 = moment2 + assert( graph[y] ) * delta ^ 2
    end
    metrics[name] = metrics[name] or {}
    metrics[name].area = moment0
    metrics[name].avg = moment1 / moment0
    metrics[name].stddev = (moment2 - moment0 ^ 2) ^ .5 / moment0
  end
  return metrics
end

local function metrics_ratios( m1, m2 )
  assert( m1 )
  assert( m2 )
  local ratios = {}
  local metric_names = { 'area', 'avg', 'stddev' }
  for _, curve in ipairs( ORDERING ) do
    ratios[curve] = {}
    for _, metric_name in ipairs( metric_names ) do
      ratios[curve][metric_name] =
          assert( m1[curve][metric_name] ) /
              assert( m2[curve][metric_name] )
    end
  end
  return ratios
end

local function tweak_spec( curve, metric_name, ratios )
  local curve_metric_ratios = assert( ratios[curve] )
  local spec = assert( SPEC[curve] )
  local params = spec.curve
  if metric_name == 'area' then
    local area = assert( curve_metric_ratios.area )
    spec.weight = spec.weight / area
  elseif metric_name == 'avg' then
    local avg = assert( curve_metric_ratios.avg )
    local delta = abs( params.c - 35.5 )
    delta = delta / avg
    if params.c < 35.5 then
      params.c = 35.5 - delta
    else
      params.c = delta + 35.5
    end
  elseif metric_name == 'stddev' then
    local stddev = assert( curve_metric_ratios.stddev )
    params.w = params.w / stddev
  else
    error( format( 'unrecognized metric name: %s',
                   tostring( metric_name ) ) )
  end
end

local function check_ratios( ratios )
  assert( ratios )
  -- Leave out "avg" here because we can't fit that at the same
  -- time that we fit stddev, and our algo seems to prefer fit-
  -- ting stdev.
  local metric_names = { 'area', 'stddev' }
  for _, curve in ipairs( ORDERING ) do
    for _, metric_name in ipairs( metric_names ) do
      local val = ratios[curve][metric_name]
      if abs( 1.0 - val ) > RATIO_TOLERANCE then
        return false
      end
    end
  end
  return true
end

-----------------------------------------------------------------
-- Graph evaluation.
-----------------------------------------------------------------
local function evaluate_row( y )
  assert( y )
  local res = {}
  for _, name in ipairs( ORDERING ) do
    local weight = assert( SPEC[name].weight )
    local curve = assert( SPEC[name].curve )
    res[name] = res[name] or 0
    res[name] = res[name] +
                    max( weight * evaluate_curve( curve, y ), 0 )
  end
  return res
end

local function compute_model_graphs()
  local graphs = {}
  for y = 36, 70 do
    local numbers = assert( evaluate_row( y ) )
    local row_total = 0
    for _, name in ipairs( ORDERING ) do
      row_total = row_total + assert( numbers[name] )
    end
    for _, name in ipairs( ORDERING ) do
      graphs[name] = graphs[name] or {}
      if row_total == 0 then
        graphs[name][y] = 0
        graphs[name][71 - y] = 0
      else
        graphs[name][y] = numbers[name] / row_total
        graphs[name][71 - y] = numbers[name] / row_total
      end
    end
  end
  return graphs, compute_metrics( graphs )
end

-----------------------------------------------------------------
-- Reference plot.
-----------------------------------------------------------------
local function read_reference_plot( mode )
  assert( mode )
  local filename = format( '../gamegen/plots/%s.csv', mode )
  local rows, headers = csv.parse( filename )
  headers = headers -- use me
  local ref = {}
  for _, row in ipairs( rows ) do
    for _, name in ipairs( ORDERING ) do
      local y = assert( tonumber( assert( row.y ) ) )
      local val = assert( row[name] )
      ref[name] = ref[name] or {}
      ref[name][y] = val
    end
  end
  return ref, compute_metrics( ref )
end

-----------------------------------------------------------------
-- Driver.
-----------------------------------------------------------------
local function generate_mode( mode )
  printfln( 'generating for mode %s', mode )
  SPEC = deep_copy( SPEC_INIT )
  local _, ref_metrics = read_reference_plot( mode )
  local model, model_metrics = compute_model_graphs()
  local ratios = metrics_ratios( model_metrics, ref_metrics )
  local metric_names = { 'area', 'avg', 'stddev' }
  local iteration = 0
  while true do
    for _, curve in ipairs( ORDERING ) do
      for _, metric_name in ipairs( metric_names ) do
        iteration = iteration + 1
        tweak_spec( curve, metric_name, ratios )
        model, model_metrics = compute_model_graphs()
        ratios = metrics_ratios( model_metrics, ref_metrics )
        -- print_metrics_stdout( ratios )
        if check_ratios( ratios ) then goto done end
      end
    end
    if iteration > MAX_ITERATIONS then break end
  end
  ::done::

  printfln( 'final ratios (%d iterations):', iteration )
  print_metrics_stdout( ratios )

  assert( check_ratios( ratios ),
          'ratios not within tolerance for mode ' .. mode )

  do
    local filename = format( 'plots/%s.csv', mode )
    printfln( 'writing csv result to %s', filename )
    local f<close> = assert( io.open( filename, 'w' ) )
    local function emit( fmt, ... )
      f:write( format( fmt, ... ) )
    end
    print_csv( emit, model )
  end

  do
    local filename = format( 'plots/%s.spec.rcl', mode )
    printfln( 'writing spec result to %s', filename )
    local f<close> = assert( io.open( filename, 'w' ) )
    local function emit( fmt, ... )
      f:write( format( fmt, ... ) )
    end
    print_metrics( emit, ratios, '# ' )
    print_spec( emit )
  end

  do
    local filename = format( 'plots/%s.gnuplot', mode )
    printfln( 'writing gnuplot file to %s', filename )
    local f<close> = assert( io.open( filename, 'w' ) )
    local function emit( fmt, ... )
      f:write( format( fmt, ... ) )
    end
    print_gnuplot_file( emit, mode )
  end
end

local function generate()
  local line = ''
  for _, mode in ipairs( MODES ) do
    io.write( line )
    line = '\n'
    generate_mode( mode )
  end
end

-----------------------------------------------------------------
-- main
-----------------------------------------------------------------
local function main( _ ) generate() end

os.exit( main( ... ) )