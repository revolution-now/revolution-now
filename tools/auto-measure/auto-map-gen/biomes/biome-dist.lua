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

local RATIO_TOLERANCE = 0.00001

local DIFF_CUTOFF = 0.00

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
-- data. They are given initial values that were determined em-
-- pirically to lead to convergence in the shortest amount of
-- time (just to reduce script runtime).
local SPEC_INIT = {
  savannah={ weight=2.0, curve={ e=2, c=46, w=3, sub=.3 } },
  grassland={ weight=1.0, curve={ e=2, c=46, w=3, sub=0 } },
  tundra={ weight=1.0, curve={ e=2, c=46, w=3, sub=0 } },
  plains={ weight=1.0, curve={ e=2, c=46, w=3, sub=0 } },
  prairie={ weight=1.0, curve={ e=2, c=46, w=3, sub=0 } },
  desert={ weight=4.0, curve={ e=4, c=46, w=10, sub=0 } },
  swamp={ weight=2.0, curve={ e=2, c=46, w=3, sub=.3 } },
  marsh={ weight=1.0, curve={ e=2, c=46, w=3, sub=0 } },
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

local MODE_NAMES = {
  bbtt='cool_arid', --
  bbtm='cool_normal', --
  bbtb='cool_wet', --
  bbmt='temperate_arid', --
  bbmm='temperate_normal', --
  bbmb='temperate_wet', --
  bbbt='warm_arid', --
  bbbm='warm_normal', --
  bbbb='warm_wet', --
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
  local name = MODE_NAMES[mode] or mode
  assert( name )
  emit( '%s {  # %s\n', name, mode )
  for _, curve in ipairs( ORDERING ) do
    local curve_spec = assert( spec[curve] )
    local half = 70 - 35.5
    local center = (assert( curve_spec.curve.c ) - 35.5) / half
    local stddev = assert( curve_spec.curve.w ) / half
    emit( '  %-9s {', curve )
    if curve_spec.curve.e then
      emit( ' exp: %d,', assert( curve_spec.curve.e ) )
    end
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
    local sub = assert( curve_spec.curve.sub )
    emit(
        '  %-9s { weight: %7s, center: %7s, stddev: %7s, sub: %7s }\n',
        curve, fmt( weight ), fmt( center ), fmt( stddev ),
        fmt( sub ) )
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

local function gaussian4( center, stddev, sub, y )
  local value = exp( -((y - center) / (stddev * 1.4142)) ^ 4 )
  value = value - sub
  return value
end

local function evaluate_mirrored_curves( curve, y )
  local fns = { [2]=gaussian, [4]=gaussian4 }
  local fn = assert( fns[curve.e] )
  return assert( fn( curve.c, curve.w, curve.sub, y ) ) +
             assert( fn( 70 - curve.c, curve.w, curve.sub, y ) )
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
  emit( '%s----------------------------------------\n', prefix )
  local max_deviation = 0
  for _, curve in ipairs( ORDERING ) do
    emit( '%s%-10s', prefix, curve )
    local curve_metrics = assert( metrics[curve] )
    for _, metric_name in ipairs( metric_names ) do
      local val = assert( curve_metrics[metric_name] )
      emit( '%10s', format( '%.4f', val ) )
      max_deviation = max( abs( val - 1 ), max_deviation )
    end
    emit( '\n' )
  end
  emit( '%s----------------------------------------\n', prefix )
  emit( '%smax deviation: %.4f\n', prefix, max_deviation )
  return max_deviation
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
  if l == 0 then return 0 end
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

local function compute_spec_sum( l, r )
  assert( l )
  assert( r )
  local sum = {}
  for _, name in ipairs( ORDERING ) do
    sum[name] = {}
    local s = sum[name]
    s.weight = l[name].weight + r[name].weight
    s.curve = {}
    s.curve.c = l[name].curve.c + r[name].curve.c
    s.curve.w = l[name].curve.w + r[name].curve.w
    s.curve.sub = l[name].curve.sub + r[name].curve.sub
  end
  return sum
end

local function compute_spec_sub( l, r )
  assert( l )
  assert( r )
  local sum = {}
  for _, name in ipairs( ORDERING ) do
    sum[name] = {}
    local s = sum[name]
    s.weight = r[name].weight - l[name].weight
    s.curve = {}
    s.curve.c = r[name].curve.c - l[name].curve.c
    s.curve.w = r[name].curve.w - l[name].curve.w
    s.curve.sub = r[name].curve.sub - l[name].curve.sub
  end
  return sum
end

local function compute_spec_abs_avg( l, r )
  assert( l )
  assert( r )
  local abs_avg = {}
  for _, name in ipairs( ORDERING ) do
    abs_avg[name] = {}
    local s = abs_avg[name]
    s.weight = (abs( r[name].weight ) + abs( l[name].weight )) /
                   2
    if l[name].weight < 0 then s.weight = -s.weight end
    s.curve = {}
    s.curve.c =
        (abs( r[name].curve.c ) + abs( l[name].curve.c )) / 2
    if l[name].curve.c < 0 then s.curve.c = -s.curve.c end
    s.curve.w =
        (abs( r[name].curve.w ) + abs( l[name].curve.w )) / 2
    if l[name].curve.w < 0 then s.curve.w = -s.curve.w end
    s.curve.sub = (abs( r[name].curve.sub ) +
                      abs( l[name].curve.sub )) / 2
    if l[name].curve.sub < 0 then s.curve.sub = -s.curve.sub end
  end
  return abs_avg
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
  diffs.temp_sum = compute_spec_sum( diffs.temperature_t,
                                     diffs.temperature_b )
  diffs.clim_sum = compute_spec_sum( diffs.climate_t,
                                     diffs.climate_b )
  diffs.temp_clim_sum = compute_spec_sum( diffs.temp_clim_tt,
                                          diffs.temp_clim_bb )
  diffs.temp_clim_tt_sub = compute_spec_sub(
                               compute_spec_sum(
                                   diffs.temperature_t,
                                   diffs.climate_t ),
                               diffs.temp_clim_tt )
  diffs.temp_clim_bb_sub = compute_spec_sub(
                               compute_spec_sum(
                                   diffs.temperature_b,
                                   diffs.climate_b ),
                               diffs.temp_clim_bb )
  return diffs
end

-----------------------------------------------------------------
-- Interpolation.
-----------------------------------------------------------------
local function flip_interp( i )
  assert( i )
  local flipped = {}
  for _, name in ipairs( ORDERING ) do
    flipped[name] = {}
    local s = flipped[name]
    s.weight = -i[name].weight
    s.curve = {}
    s.curve.c = -i[name].curve.c
    s.curve.w = -i[name].curve.w
    s.curve.sub = -i[name].curve.sub
  end
  return flipped
end

local function compute_spec_interp( spec, temp_delta, clim_delta )
  assert( spec )
  local interp = {}
  for _, name in ipairs( ORDERING ) do
    interp[name] = deep_copy( assert( spec[name] ) )
    local s = interp[name]
    if temp_delta then
      s.weight = s.weight * (1 + temp_delta[name].weight)
      s.curve.c = s.curve.c * (1 + temp_delta[name].curve.c)
      s.curve.w = s.curve.w * (1 + temp_delta[name].curve.w)
      s.curve.sub = s.curve.sub *
                        (1 + temp_delta[name].curve.sub)
    end
    if clim_delta then
      s.weight = s.weight * (1 + clim_delta[name].weight)
      s.curve.c = s.curve.c * (1 + clim_delta[name].curve.c)
      s.curve.w = s.curve.w * (1 + clim_delta[name].curve.w)
      s.curve.sub = s.curve.sub *
                        (1 + clim_delta[name].curve.sub)
    end
  end
  return interp
end

local function compute_interp( bbmm_spec, interps )
  local temp_up = assert( interps.temperature_up )
  local clim_up = assert( interps.climate_up )
  local temp_down = assert( flip_interp( temp_up ) )
  local clim_down = assert( flip_interp( clim_up ) )
  local function emit( fmt, ... ) io.write( format( fmt, ... ) ) end
  print_spec_diffs( temp_up, 'TEMP_DELTA_UP', emit )
  print_spec_diffs( clim_up, 'CLIM_DELTA_UP', emit )
  print_spec_diffs( temp_down, 'TEMP_DELTA_DOWN', emit )
  print_spec_diffs( clim_down, 'CLIM_DELTA_DOWN', emit )
  local res = {}
  local function f( ... )
    return compute_model_graphs(
               compute_spec_interp( bbmm_spec, ... ) )
  end
  res.bbtt = f( temp_up, clim_up )
  res.bbtm = f( temp_up, nil )
  res.bbtb = f( temp_up, clim_down )
  res.bbmt = f( nil, clim_up )
  res.bbmm = f( nil, nil )
  res.bbmb = f( nil, clim_down )
  res.bbbt = f( temp_down, clim_up )
  res.bbbm = f( temp_down, nil )
  res.bbbb = f( temp_down, clim_down )
  return res
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
    local filename = format( 'plots/%s.gnuplot', mode )
    printfln( 'writing gnuplot file to %s', filename )
    local f<close> = assert( io.open( filename, 'w' ) )
    local function emit( fmt, ... )
      f:write( format( fmt, ... ) )
    end
    print_gnuplot_file( emit, mode )
  end

  normalize_weights( spec )
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

  local interps = {}
  do
    local filename = format( 'plots/specs.rcl' )
    printfln( 'writing final spec results to %s', filename )
    local f<close> = assert( io.open( filename, 'w' ) )
    local function emit( fmt, ... )
      f:write( format( fmt, ... ) )
    end
    for _, mode in ipairs( MODES ) do
      print_spec( assert( final_specs[mode] ), mode, emit )
    end
    local bbtm_diff = compute_diff( final_specs.bbmm,
                                    final_specs.bbtm )
    local bbbm_diff = compute_diff( final_specs.bbmm,
                                    final_specs.bbbm )
    local bbmt_diff = compute_diff( final_specs.bbmm,
                                    final_specs.bbmt )
    local bbmb_diff = compute_diff( final_specs.bbmm,
                                    final_specs.bbmb )
    interps.temperature_up = assert(
                                 compute_spec_abs_avg( bbtm_diff,
                                                       bbbm_diff ) )
    interps.climate_up = assert(
                             compute_spec_abs_avg( bbmt_diff,
                                                   bbmb_diff ) )
  end

  local diffs = compute_diffs( final_specs )
  do
    local filename = format( 'plots/diffs.rcl' )
    printfln( 'writing diff results to %s', filename )
    local f<close> = assert( io.open( filename, 'w' ) )
    local function emit( fmt, ... )
      f:write( format( fmt, ... ) )
    end
    local order = {
      'temperature_t', --
      'temperature_b', --
      'temp_sum', --
      'climate_t', --
      'climate_b', --
      'clim_sum', --
      'temp_clim_tt', --
      'temp_clim_bb', --
      'temp_clim_sum', --
      'temp_clim_tt_sub', --
      'temp_clim_bb_sub', --
    }
    for _, key in ipairs( order ) do
      print_spec_diffs( assert( diffs[key] ), key, emit )
    end
  end

  printfln( 'generating interpolated graphs...' )
  local generated_interps = assert(
                                compute_interp( final_specs.bbmm,
                                                interps ) )
  do
    for _, mode in ipairs( MODES ) do
      local interp = assert( generated_interps[mode], mode )
      local filename = format( 'interpolated/%s.csv', mode )
      printfln( 'writing interpolated results to %s', filename )
      local f<close> = assert( io.open( filename, 'w' ) )
      local function emit( fmt, ... )
        f:write( format( fmt, ... ) )
      end
      print_csv( emit, interp )
    end
  end
  do
    for _, mode in ipairs( MODES ) do
      local filename = format( 'interpolated/%s.gnuplot', mode )
      local f<close> = assert( io.open( filename, 'w' ) )
      local function emit( fmt, ... )
        f:write( format( fmt, ... ) )
      end
      print_gnuplot_file( emit, mode )
    end
  end

  printfln( 'writing interpolated specs...' )
  do
    local filename = format( 'interpolated/specs.rcl' )
    local f<close> = assert( io.open( filename, 'w' ) )
    local function emit( fmt, ... )
      f:write( format( fmt, ... ) )
    end
    print_spec( assert( final_specs.bbmm ), 'bbmm', emit )
    emit( '\n' )
    print_spec_diffs( interps.temperature_up,
                      'temperature_gradient', emit )
    emit( '\n' )
    print_spec_diffs( interps.climate_up, 'climate_gradient',
                      emit )
  end

  printfln( 'writing interpolated spec accuracy...' )
  do
    local filename = format( 'interpolated/accuracy.txt' )
    local f<close> = assert( io.open( filename, 'w' ) )
    local function emit( fmt, ... )
      f:write( format( fmt, ... ) )
    end
    local global_max_deviation = 0
    for _, mode in ipairs( MODES ) do
      local _, ref_metrics = read_reference_plot( mode )
      local interp = assert( generated_interps[mode], mode )
      local interp_metrics = compute_metrics( interp )
      local ratios =
          metrics_ratios( interp_metrics, ref_metrics )
      emit( 'mode: %s\n', mode )
      local prefix = '  '
      local max_deviation = print_metrics( emit, ratios, prefix )
      global_max_deviation = max( max_deviation,
                                  global_max_deviation )
      emit( '\n' )
    end
    emit( '\n' )
    emit( 'Global Max Deviation: %.4f\n', global_max_deviation )
  end
end

-----------------------------------------------------------------
-- main
-----------------------------------------------------------------
local function main( _ ) generate() end

os.exit( main( ... ) )