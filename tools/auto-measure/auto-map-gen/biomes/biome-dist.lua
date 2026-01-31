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
local MAX_ITERATIONS = 100000

-- Ignore the curve this close to the edges since it is noisy.
local BUFFER = 5

local RATIO_TOLERANCE = 0.0005

local DIFF_CUTOFF = 0.10

local MODES = {
  'bbtt', --
  'bbtm', --
  'bbtb', --
  'bbmt', --
  'bbmm', --
  'bbmb', --
  'bbbt', --
  'bbbm', --
  'bbbb', --
}

-- All of these values (except for `sub`) are placeholders; they
-- will be adjusted on each iteration to fit the experimental
-- data. They are given initial values in the valid range so that
-- the iteration is able to get started.
local SPEC_INIT = {
  savannah={ weight=1.0, curve={ c=40, w=10, sub=.025 } },
  grassland={ weight=1.0, curve={ c=40, w=10, sub=0 } },
  tundra={ weight=1.0, curve={ c=40, w=10, sub=0 } },
  plains={ weight=1.0, curve={ c=40, w=10, sub=0 } },
  prairie={ weight=1.0, curve={ c=40, w=10, sub=0 } },
  desert={ weight=1.0, curve={ c=40, w=10, sub=0 } },
  swamp={ weight=1.0, curve={ c=40, w=10, sub=.025 } },
  marsh={ weight=1.0, curve={ c=40, w=10, sub=0 } },
}

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

local function normalize_weights( SPEC )
  local total = 0
  for _, curve in ipairs( ORDERING ) do
    local spec = assert( SPEC[curve] )
    total = total + assert( spec.weight )
  end
  for _, curve in ipairs( ORDERING ) do
    local spec = assert( SPEC[curve] )
    spec.weight = spec.weight / total
  end
end

local function print_spec( spec, mode, emit )
  assert( mode )
  emit( '%s {\n', mode )
  for _, curve in ipairs( ORDERING ) do
    local curve_spec = assert( spec[curve] )
    local half = 70 - 35.5
    local center = (assert( curve_spec.curve.c ) - 35.5) / half
    local stddev = assert( curve_spec.curve.w ) / half
    emit( '  %-9s {', curve )
    emit( ' weight: %.4f,', assert( curve_spec.weight ) )
    emit( ' center: %.4f,', center )
    emit( ' stddev: %.4f,', stddev )
    emit( ' sub: %.4f', assert( curve_spec.curve.sub ) )
    emit( ' }\n' )
  end
  emit( '}\n' )
end

local function print_spec_diffs( spec, mode, emit )
  assert( mode )
  emit( '%s {\n', mode )
  local function fmt( f )
    if abs( f ) < DIFF_CUTOFF then return '-' end
    return format( '%.4f', assert( f ) )
  end
  for _, curve in ipairs( ORDERING ) do
    local curve_spec = assert( spec[curve] )
    local weight = assert( curve_spec.weight )
    local center = assert( curve_spec.curve.c )
    local stddev = assert( curve_spec.curve.w )
    emit( '  %-9s { weight: %7s, stddev: %7s, center: %7s }\n',
          curve, fmt( weight ), fmt( stddev ), fmt( center ) )
  end
  emit( '}\n' )
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
local function gaussian( center, stddev, sub, y )
  local value = exp( -((y - center) / (stddev * 1.4142)) ^ 2 )
  value = value - sub
  return value
end

local function evaluate_mirrored_curves( curve, y )
  return assert( gaussian( curve.c, curve.w, curve.sub, y ) ) +
             assert(
                 gaussian( 70 - curve.c, curve.w, curve.sub, y ) )
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
    local sample = function( y )
      if y >= 70 then return graph[70] end
      local lo = math.floor( y )
      local hi = math.ceil( y )
      local e = y - lo
      return graph[lo] * e + graph[hi] * (1 - e)
    end
    for y = 36, 70 - BUFFER, .1 do
      local delta = y - 35.5
      local val = sample( y )
      moment0 = moment0 + val * delta ^ 0
      moment1 = moment1 + val * delta ^ 1
      moment2 = moment2 + val * delta ^ 2
    end
    metrics[name] = metrics[name] or {}
    metrics[name].area = moment0
    metrics[name].avg = moment1 / moment0
    local var = moment2 / moment0
    local mean = moment1 / moment0
    metrics[name].stddev = (var - mean ^ 2) ^ .5
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

local function tweak_spec( spec, curve, metric_name, ratios )
  assert( spec )
  local curve_metric_ratios = assert( ratios[curve] )
  local curve_spec = assert( spec[curve] )
  local params = curve_spec.curve
  -- Below, instead of dividing by the ratio directly (which
  -- would make most sense) bring it closer to 1 first since oth-
  -- erwise it seems to overshoot causing convergence to take
  -- longer. Not sure why this is.
  if metric_name == 'area' then
    local area = assert( curve_metric_ratios.area )
    curve_spec.weight = curve_spec.weight / ((area + 1) / 2)
  elseif metric_name == 'avg' then
    local avg = assert( curve_metric_ratios.avg )
    local delta = abs( params.c - 35.5 )
    delta = delta / ((avg + 1) / 2)
    if params.c < 35.5 then
      params.c = 35.5 - delta
    else
      params.c = delta + 35.5
    end
    params.c = max( params.c, 35.5 )
  elseif metric_name == 'stddev' then
    local stddev = assert( curve_metric_ratios.stddev )
    params.w = params.w / ((stddev + 1) / 2)
  else
    error( format( 'unrecognized metric name: %s',
                   tostring( metric_name ) ) )
  end
end

local function check_ratios( ratios )
  assert( ratios )
  local metric_names = { 'area', 'avg', 'stddev' }
  for _, curve in ipairs( ORDERING ) do
    for _, metric_name in ipairs( metric_names ) do
      local val = ratios[curve][metric_name]
      if abs( 1.0 - val ) >= RATIO_TOLERANCE then
        return false
      end
    end
  end
  return true
end

-----------------------------------------------------------------
-- Graph evaluation.
-----------------------------------------------------------------
local function evaluate_row( spec, y )
  assert( y )
  local res = {}
  for _, name in ipairs( ORDERING ) do
    local weight = assert( spec[name].weight )
    local curve = assert( spec[name].curve )
    res[name] = res[name] or 0
    res[name] = res[name] + weight *
                    evaluate_mirrored_curves( curve, y )
    res[name] = max( res[name], 0 )
  end
  return res
end

local function compute_model_graphs( spec )
  local graphs = {}
  for y = 36, 70 do
    local numbers = assert( evaluate_row( spec, y ) )
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
-- Diffs.
-----------------------------------------------------------------
local function diff_values( l, r )
  assert( l )
  assert( r )
  return (r - l) / l
end

local function compute_diff( l, r )
  assert( l )
  assert( r )
  local diff = {}
  for _, name in ipairs( ORDERING ) do
    diff[name] = {}
    local d = diff[name]
    assert( l[name].weight )
    assert( r[name].weight )
    assert( l[name].curve.c )
    assert( r[name].curve.c )
    assert( l[name].curve.w )
    assert( r[name].curve.w )
    d.weight = diff_values( l[name].weight, r[name].weight )
    d.curve = {}
    d.curve.c = diff_values( l[name].curve.c, r[name].curve.c )
    d.curve.w = diff_values( l[name].curve.w, r[name].curve.w )
    d.curve.sub = diff_values( l[name].curve.sub,
                               r[name].curve.sub )
  end
  return diff
end

local function compute_diffs( specs )
  assert( specs )
  local diffs = {}
  diffs.temperature_t = compute_diff( specs.bbmm, specs.bbtm )
  diffs.temperature_b = compute_diff( specs.bbmm, specs.bbbm )
  diffs.climate_t = compute_diff( specs.bbmm, specs.bbmt )
  diffs.climate_b = compute_diff( specs.bbmm, specs.bbmb )
  diffs.temp_clim_tt = compute_diff( specs.bbmm, specs.bbtt )
  diffs.temp_clim_bb = compute_diff( specs.bbmm, specs.bbbb )
  return diffs
end

-----------------------------------------------------------------
-- Driver.
-----------------------------------------------------------------
local function generate_mode( mode )
  printfln( 'generating for mode %s', mode )
  local spec = deep_copy( SPEC_INIT )
  local _, ref_metrics = read_reference_plot( mode )
  local model, model_metrics = compute_model_graphs( spec )
  local ratios = metrics_ratios( model_metrics, ref_metrics )
  local metric_names = { 'area', 'avg', 'stddev' }
  local iteration = 0
  while true do
    for _, curve in ipairs( ORDERING ) do
      for _, metric_name in ipairs( metric_names ) do
        iteration = iteration + 1
        tweak_spec( spec, curve, metric_name, ratios )
        model, model_metrics = compute_model_graphs( spec )
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
    -- print_metrics( emit, ratios, '# ' )
    normalize_weights( spec )
    print_spec( spec, mode, emit )
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

  return spec
end

local function generate()
  local line = ''
  local final_specs = {}
  for _, mode in ipairs( MODES ) do
    io.write( line )
    line = '\n'
    final_specs[mode] = assert( generate_mode( mode ) )
  end

  local diffs = compute_diffs( final_specs )
  do
    local filename = format( 'plots/diffs.rcl' )
    printfln( 'writing diff results to %s', filename )
    local f<close> = assert( io.open( filename, 'w' ) )
    local function emit( fmt, ... )
      f:write( format( fmt, ... ) )
      io.write( format( fmt, ... ) )
    end
    local order = {
      'temperature_t', --
      'climate_t', --
      'temp_clim_tt', --
      'temperature_b', --
      'climate_b', --
      'temp_clim_bb', --
    }
    for _, key in ipairs( order ) do
      print_spec_diffs( assert( diffs[key] ), key, emit )
    end
  end
end

-----------------------------------------------------------------
-- main
-----------------------------------------------------------------
local function main( _ ) generate() end

os.exit( main( ... ) )