-- This lambda will run on multiple savs and will gather some
-- statistics on the distribution of "overlays", i.e. mountains,
-- hills, and forests.
-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local Q = require( 'lib.query' )
local json = require( 'moon.json' )
local plot = require( 'moon.plot' )
local tbl = require( 'moon.tbl' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local insert = table.insert
local format = string.format
local max = math.max
local log = math.log
local floor = math.floor
local deep_copy = assert( tbl.deep_copy )

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local function printfln( ... ) print( string.format( ... ) ) end

local PLOTS_DIR = 'overlays/empirical'

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

-----------------------------------------------------------------
-- Data.
-----------------------------------------------------------------
-- The data here spans multiple savs.
local D = {
  savs=0,
  tiles=0,
  land=0,
  land_arctic_row=0,
  land_ocean_adjacent=0,
  land_ocean_adjacent_cardinal=0,
  land_non_mounds=0,

  -- key=row
  land_by_row={},
  -- key=col
  land_by_col={},

  -- key=biome
  land_with_biome={},

  -- key=y, value={ key=biome, value=count }
  land_with_biome_by_row={},

  count={
    mountains=0, --
    hills=0, --
    forest=0, --

    mountains_squared=0, --
    hills_squared=0, --
    forest_squared=0, --
  },

  count_rivers_on_land=0, --

  count_arctic_rows={
    mountains=0, --
    hills=0, --
    forest=0, --
  },

  -- key=biome
  count_with_biome={
    mountains={}, --
    hills={}, --
    clearing={}, --
  },

  -- key=y, value={ key=biome, value=count }
  count_with_biome_by_row={
    mountains={}, --
    hills={}, --
    clearing={}, --
  },

  -- key=row
  count_by_row={
    mountains={}, --
    hills={}, --
    clearing={}, --
  },

  -- key=col
  count_by_col={
    mountains={}, --
    hills={}, --
    clearing={}, --
  },

  -- Note this includes singletons.
  num_ranges={
    mountains=0, --
    hills=0, --
    clearing=0, --
  },

  -- key=length
  count_range_length={
    mountains={}, --
    hills={}, --
    clearing={}, --
  },

  count_range_centers={
    mountains=0, --
    hills=0, --
    clearing=0, --
  },

  -- key=row
  count_range_centers_by_row={
    mountains={}, --
    hills={}, --
    clearing={}, --
  },

  -- key=col
  count_range_centers_by_col={
    mountains={}, --
    hills={}, --
    clearing={}, --
  },

  count_ocean_adjacent={
    mountains=0, --
    hills=0, --
    clearing=0, --
  },

  count_ocean_adjacent_cardinal={
    mountains=0, --
    hills=0, --
    clearing=0, --
  },

  count_large_range={
    mountains=0, --
    hills=0, --
    clearing=0, --
  },

  -- Count of tiles that are part of a large range that are adja-
  -- cent to water. key=row.
  count_large_range_by_row={
    mountains={}, --
    hills={}, --
    clearing={}, --
  },

  -- Count of tiles that are part of a large range that are adja-
  -- cent to water. key=col.
  count_large_range_by_col={
    mountains={}, --
    hills={}, --
    clearing={}, --
  },

  -- Count of tiles that are part of a large range that are adja-
  -- cent to water.
  count_large_range_ocean_adjacent={
    mountains=0, --
    hills=0, --
    clearing=0, --
  },

  count_large_range_ocean_adjacent_cardinal={
    mountains=0, --
    hills=0, --
    clearing=0, --
  },

  count_mountains_adjacent_to_hills=0,
  count_hills_adjacent_to_mountains=0,
}

-- Data that gets reset on each map.
local _D_TEMPLATE = {
  -- This per-map count is kept so that we can compute std. devi-
  -- ation of the densities.
  count={
    mountains=0, --
    hills=0, --
    forest=0, --
  },

  segments={
    mountains=0, --
    hills=0, --
    clearing=0, --
  },

  -- key=rastorized_tile, value=segment
  tile_to_segment={
    mountains={}, --
    hills={}, --
    clearing={}, --
  },

  -- key=segment, value=int
  segment_to_length={
    mountains={}, --
    hills={}, --
    clearing={}, --
  },

  -- key=segment, value=list
  segment_to_tiles={
    mountains={}, --
    hills={}, --
    clearing={}, --
  },

  -- Note this includes singletons.
  num_ranges={
    mountains=0, --
    hills=0, --
    clearing=0, --
  },
}

-- Data that gets reset on each map.
local _D

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local function terrain_at( J, tile )
  -- Can intercept here.
  return Q.terrain_at( J, tile )
end

local function ocean_adjacent( J, tile )
  local has = false
  Q.on_surrounding_tiles( tile, function( coord )
    local adjacent = terrain_at( J, coord )
    if adjacent.surface == 'water' then has = true end
  end )
  return has
end

local function ocean_adjacent_cardinal( J, tile )
  local has = false
  Q.on_surrounding_tiles_cardinal( tile, function( coord )
    local adjacent = terrain_at( J, coord )
    if adjacent.surface == 'water' then has = true end
  end )
  return has
end

-----------------------------------------------------------------
-- Connected Ranges.
-----------------------------------------------------------------
-- The number 5 is taken from the data itself; around length 5
-- there appears to be an inflection point (in the log graph).
local function is_large_range( length ) return length >= 5 end

local function assign_segment( J, kind, has_kind, start, segment )
  assert( has_kind( start ) )
  assert( type( segment ) == 'number' )
  assert( segment > 0 )
  local rastorized_start = Q.rastorize( start )
  assert( _D.tile_to_segment[kind][rastorized_start] == 0 )
  _D.tile_to_segment[kind][rastorized_start] = segment
  _D.segment_to_tiles[kind][segment] =
      _D.segment_to_tiles[kind][segment] or {}
  insert( _D.segment_to_tiles[kind][segment], start )
  assert( not Q.is_water( J, start ) )
  Q.on_non_arctic_surrounding_tiles_cardinal( start,
                                              function( coord )
    local rastorized_coord = Q.rastorize( coord )
    if _D.tile_to_segment[kind][rastorized_coord] and
        _D.tile_to_segment[kind][rastorized_coord] > 0 then
      assert( has_kind( coord ) )
      if _D.tile_to_segment[kind][rastorized_coord] ~= segment then
        assert( _D.tile_to_segment[kind][rastorized_coord] ==
                    segment )
      end
      return
    end
    if has_kind( coord ) then
      local is_surrounding_water = Q.is_water( J, coord )
      assert( not is_surrounding_water )
      assign_segment( J, kind, has_kind, coord, segment )
    end
  end )
end

local function find_center( segment, kind )
  assert( segment )
  assert( kind )
  local tiles = assert( _D.segment_to_tiles[kind][segment] )
  assert( #tiles > 0 )
  local x = 0
  local y = 0
  for _, tile in ipairs( tiles ) do
    x = x + tile.x
    y = y + tile.y
  end
  x = floor( x / #tiles )
  y = floor( y / #tiles )
  return { x=x, y=y }
end

local function find_connected( J, kind, has_kind )
  local rastorized_tiles = {}
  for rastorized_tile, _ in pairs( _D.tile_to_segment[kind] ) do
    insert( rastorized_tiles, rastorized_tile )
  end
  for _, rastorized_tile in ipairs( rastorized_tiles ) do
    local segment = assert(
                        _D.tile_to_segment[kind][rastorized_tile] )
    local tile = Q.unrastorize( rastorized_tile )
    -- printfln( 'exploring %s tile: %s', ,kind, tile )
    assert( has_kind( tile ) )
    if segment == 0 then
      _D.segments[kind] = _D.segments[kind] + 1
      assign_segment( J, kind, has_kind, tile, _D.segments[kind] )
    end
  end
  local segment_length = {}
  Q.on_all_non_arctic_tiles( function( tile )
    if not has_kind( tile ) then return end
    local rastorized = Q.rastorize( tile )
    local segment = assert( _D.tile_to_segment[kind][rastorized],
                            tostring(
                                _D.tile_to_segment[kind][rastorized] ) ..
                                ', ' ..
                                tostring(
                                    Q.unrastorize( rastorized ) ) )
    assert( segment > 0 )
    segment_length[segment] = segment_length[segment] or 0
    segment_length[segment] = segment_length[segment] + 1
  end )
  local max_length = 0
  for segment = 1, _D.segments[kind] do
    local length = assert( segment_length[segment],
                           tostring( segment ) )
    max_length = math.max( max_length, length )
    D.count_range_length[kind][length] =
        D.count_range_length[kind][length] + 1
    _D.segment_to_length[kind][segment] = length
    D.count_range_centers[kind] = D.count_range_centers[kind] + 1
    local center = assert( find_center( segment, kind ) )
    D.count_range_centers_by_row[kind][center.y] =
        D.count_range_centers_by_row[kind][center.y] + 1
    D.count_range_centers_by_col[kind][center.x] =
        D.count_range_centers_by_col[kind][center.x] + 1
  end
  D.num_ranges[kind] = D.num_ranges[kind] +
                           assert( _D.segments[kind] )
  _D.num_ranges[kind] = _D.segments[kind]
  Q.on_all_non_arctic_tiles( function( tile )
    local rastorized = Q.rastorize( tile )
    local segment = _D.tile_to_segment[kind][rastorized]
    if not segment then return end
    local length = assert( _D.segment_to_length[kind][segment] )
    if not is_large_range( length ) then return end
    -- We have a tile on a large range.
    D.count_large_range[kind] = D.count_large_range[kind] + 1
    D.count_large_range_by_row[kind][tile.y] =
        D.count_large_range_by_row[kind][tile.y] + 1
    D.count_large_range_by_col[kind][tile.x] =
        D.count_large_range_by_col[kind][tile.x] + 1
    local has_ocean_adjacent = ocean_adjacent( J, tile )
    local has_ocean_adjacent_cardinal =
        ocean_adjacent_cardinal( J, tile )
    if has_ocean_adjacent then
      D.count_large_range_ocean_adjacent[kind] =
          D.count_large_range_ocean_adjacent[kind] + 1
    end
    if has_ocean_adjacent_cardinal then
      D.count_large_range_ocean_adjacent_cardinal[kind] =
          D.count_large_range_ocean_adjacent_cardinal[kind] + 1
    end
  end )
end

-----------------------------------------------------------------
-- Lambda.
-----------------------------------------------------------------
local function lambda( J )
  _D = deep_copy( _D_TEMPLATE )
  D.savs = D.savs + 1

  -- Initialize.
  Q.on_all_tiles( function( tile )
    D.land_by_row[tile.y] = D.land_by_row[tile.y] or 0
    D.land_by_col[tile.x] = D.land_by_col[tile.x] or 0
    D.count_by_row.mountains[tile.y] =
        D.count_by_row.mountains[tile.y] or 0
    D.count_by_row.hills[tile.y] =
        D.count_by_row.hills[tile.y] or 0
    D.count_by_row.clearing[tile.y] =
        D.count_by_row.clearing[tile.y] or 0
    D.count_by_col.mountains[tile.x] =
        D.count_by_col.mountains[tile.x] or 0
    D.count_by_col.hills[tile.x] =
        D.count_by_col.hills[tile.x] or 0
    D.count_by_col.clearing[tile.x] =
        D.count_by_col.clearing[tile.x] or 0
    D.count_large_range_by_row.mountains[tile.y] =
        D.count_large_range_by_row.mountains[tile.y] or 0
    D.count_large_range_by_row.hills[tile.y] =
        D.count_large_range_by_row.hills[tile.y] or 0
    D.count_large_range_by_row.clearing[tile.y] =
        D.count_large_range_by_row.clearing[tile.y] or 0
    D.count_large_range_by_col.mountains[tile.x] =
        D.count_large_range_by_col.mountains[tile.x] or 0
    D.count_large_range_by_col.hills[tile.x] =
        D.count_large_range_by_col.hills[tile.x] or 0
    D.count_large_range_by_col.clearing[tile.x] =
        D.count_large_range_by_col.clearing[tile.x] or 0
    D.count_range_centers_by_row.mountains[tile.y] =
        D.count_range_centers_by_row.mountains[tile.y] or 0
    D.count_range_centers_by_row.hills[tile.y] =
        D.count_range_centers_by_row.hills[tile.y] or 0
    D.count_range_centers_by_row.clearing[tile.y] =
        D.count_range_centers_by_row.clearing[tile.y] or 0
    D.count_range_centers_by_col.mountains[tile.x] =
        D.count_range_centers_by_col.mountains[tile.x] or 0
    D.count_range_centers_by_col.hills[tile.x] =
        D.count_range_centers_by_col.hills[tile.x] or 0
    D.count_range_centers_by_col.clearing[tile.x] =
        D.count_range_centers_by_col.clearing[tile.x] or 0
    D.land_with_biome_by_row[tile.y] =
        D.land_with_biome_by_row[tile.y] or {}
    for _, biome in ipairs( BIOME_ORDERING ) do
      D.land_with_biome_by_row[tile.y][biome] =
          D.land_with_biome_by_row[tile.y][biome] or 0
    end
    D.count_with_biome_by_row.mountains[tile.y] =
        D.count_with_biome_by_row.mountains[tile.y] or {}
    D.count_with_biome_by_row.hills[tile.y] =
        D.count_with_biome_by_row.hills[tile.y] or {}
    D.count_with_biome_by_row.clearing[tile.y] =
        D.count_with_biome_by_row.clearing[tile.y] or {}
    for _, biome in ipairs( BIOME_ORDERING ) do
      D.count_with_biome_by_row.mountains[tile.y][biome] =
          D.count_with_biome_by_row.mountains[tile.y][biome] or 0
      D.count_with_biome_by_row.hills[tile.y][biome] =
          D.count_with_biome_by_row.hills[tile.y][biome] or 0
      D.count_with_biome_by_row.clearing[tile.y][biome] =
          D.count_with_biome_by_row.clearing[tile.y][biome] or 0
    end
  end )

  for i = 1, 200 do
    D.count_range_length.mountains[i] =
        D.count_range_length.mountains[i] or 0
    D.count_range_length.hills[i] =
        D.count_range_length.hills[i] or 0
    D.count_range_length.clearing[i] =
        D.count_range_length.clearing[i] or 0
  end

  for _, biome in ipairs( BIOME_ORDERING ) do
    D.land_with_biome[biome] = D.land_with_biome[biome] or 0
    D.count_with_biome.mountains[biome] =
        D.count_with_biome.mountains[biome] or 0
    D.count_with_biome.hills[biome] =
        D.count_with_biome.hills[biome] or 0
    D.count_with_biome.clearing[biome] =
        D.count_with_biome.clearing[biome] or 0
  end

  -- Gather data.
  Q.on_all_tiles( function( tile )
    D.tiles = D.tiles + 1
    local square = terrain_at( J, tile )
    local is_land = assert( square.surface ) == 'land'
    local is_arctic_row = tile.y == 1 or tile.y == 70
    if is_arctic_row then
      if is_land then
        D.land_arctic_row = D.land_arctic_row + 1
      end
      if Q.has_mountains( J, tile ) then
        assert( is_land )
        D.count_arctic_rows.mountains =
            D.count_arctic_rows.mountains + 1
      end
      if Q.has_hills( J, tile ) then
        assert( is_land )
        D.count_arctic_rows.hills = D.count_arctic_rows.hills + 1
      end
      if Q.has_forest( J, tile ) then
        assert( is_land )
        D.count_arctic_rows.forest =
            D.count_arctic_rows.forest + 1
      end
      return
    end
    local has_ocean_adjacent = ocean_adjacent( J, tile )
    local has_ocean_adjacent_cardinal =
        ocean_adjacent_cardinal( J, tile )
    if is_land then
      D.land = D.land + 1
      D.land_by_row[tile.y] = D.land_by_row[tile.y] + 1
      D.land_by_col[tile.x] = D.land_by_col[tile.x] + 1
      if has_ocean_adjacent then
        D.land_ocean_adjacent = D.land_ocean_adjacent + 1
      end
      if has_ocean_adjacent_cardinal then
        D.land_ocean_adjacent_cardinal =
            D.land_ocean_adjacent_cardinal + 1
      end
      D.land_with_biome[square.ground] =
          D.land_with_biome[square.ground] + 1
      D.land_with_biome_by_row[tile.y][square.ground] =
          D.land_with_biome_by_row[tile.y][square.ground] + 1
    end
    local has_hills_adjacent = false
    local has_mountains_adjacent = false
    Q.on_non_arctic_surrounding_tiles_cardinal( tile,
                                                function( coord )
      local adjacent = terrain_at( J, coord )
      if adjacent.surface == 'water' then return end
      if Q.has_mountains( J, coord ) then
        has_mountains_adjacent = true
      end
      if Q.has_hills( J, coord ) then
        has_hills_adjacent = true
      end
    end )
    if is_land and Q.has_river( J, tile ) then
      D.count_rivers_on_land = D.count_rivers_on_land + 1
    end
    if Q.has_mountains( J, tile ) then
      assert( is_land )
      D.count.mountains = D.count.mountains + 1
      _D.count.mountains = _D.count.mountains + 1
      D.count_by_row.mountains[tile.y] =
          D.count_by_row.mountains[tile.y] + 1
      D.count_by_col.mountains[tile.x] =
          D.count_by_col.mountains[tile.x] + 1
      if has_ocean_adjacent then
        D.count_ocean_adjacent.mountains =
            D.count_ocean_adjacent.mountains + 1
      end
      if has_ocean_adjacent_cardinal then
        D.count_ocean_adjacent_cardinal.mountains =
            D.count_ocean_adjacent_cardinal.mountains + 1
      end
      if has_hills_adjacent then
        D.count_mountains_adjacent_to_hills =
            D.count_mountains_adjacent_to_hills + 1
      end
      _D.tile_to_segment.mountains[Q.rastorize( tile )] = 0
      D.count_with_biome.mountains[square.ground] =
          D.count_with_biome.mountains[square.ground] + 1
      D.count_with_biome_by_row.mountains[tile.y][square.ground] =
          D.count_with_biome_by_row.mountains[tile.y][square.ground] +
              1
    end
    if Q.has_hills( J, tile ) then
      assert( is_land )
      D.count.hills = D.count.hills + 1
      _D.count.hills = _D.count.hills + 1
      D.count_by_row.hills[tile.y] =
          D.count_by_row.hills[tile.y] + 1
      D.count_by_col.hills[tile.x] =
          D.count_by_col.hills[tile.x] + 1
      if has_ocean_adjacent then
        D.count_ocean_adjacent.hills =
            D.count_ocean_adjacent.hills + 1
      end
      if has_ocean_adjacent_cardinal then
        D.count_ocean_adjacent_cardinal.hills =
            D.count_ocean_adjacent_cardinal.hills + 1
      end
      if has_mountains_adjacent then
        D.count_hills_adjacent_to_mountains =
            D.count_hills_adjacent_to_mountains + 1
      end
      _D.tile_to_segment.hills[Q.rastorize( tile )] = 0
      D.count_with_biome.hills[square.ground] =
          D.count_with_biome.hills[square.ground] + 1
      D.count_with_biome_by_row.hills[tile.y][square.ground] =
          D.count_with_biome_by_row.hills[tile.y][square.ground] +
              1
    end
    if Q.has_forest( J, tile ) then
      assert( is_land )
      D.count.forest = D.count.forest + 1
      _D.count.forest = _D.count.forest + 1
    end
    local has_overlays = Q.has_mountains( J, tile ) or
                             Q.has_hills( J, tile ) or
                             Q.has_forest( J, tile )
    local clearing = is_land and not has_overlays
    if clearing then
      D.count_by_row.clearing[tile.y] =
          D.count_by_row.clearing[tile.y] + 1
      D.count_by_col.clearing[tile.x] =
          D.count_by_col.clearing[tile.x] + 1
      if has_ocean_adjacent then
        D.count_ocean_adjacent.clearing =
            D.count_ocean_adjacent.clearing + 1
      end
      if has_ocean_adjacent_cardinal then
        D.count_ocean_adjacent_cardinal.clearing =
            D.count_ocean_adjacent_cardinal.clearing + 1
      end
      _D.tile_to_segment.clearing[Q.rastorize( tile )] = 0
      D.count_with_biome.clearing[square.ground] =
          D.count_with_biome.clearing[square.ground] + 1
      D.count_with_biome_by_row.clearing[tile.y][square.ground] =
          D.count_with_biome_by_row.clearing[tile.y][square.ground] +
              1
    end
    if is_land and not Q.has_mountains( J, tile ) and
        not Q.has_hills( J, tile ) then
      D.land_non_mounds = D.land_non_mounds + 1
    end
  end )

  local function is_mountains( tile )
    return Q.has_mountains( J, tile )
  end
  local function is_hills( tile ) return Q.has_hills( J, tile ) end
  local function is_clearing( tile )
    local is_land = Q.is_land( J, tile )
    local has_overlays = Q.has_mountains( J, tile ) or
                             Q.has_hills( J, tile ) or
                             Q.has_forest( J, tile )
    return is_land and not has_overlays
  end

  find_connected( J, 'mountains', is_mountains )
  find_connected( J, 'hills', is_hills )
  find_connected( J, 'clearing', is_clearing )

  D.count.mountains_squared = D.count.mountains_squared +
                                  _D.count.mountains ^ 2
  D.count.hills_squared =
      D.count.hills_squared + _D.count.hills ^ 2
  D.count.forest_squared = D.count.forest_squared +
                               _D.count.forest ^ 2
end

local function stddev( s1, s2, denominator )
  return (s2 / denominator - (s1 / denominator) ^ 2) ^ .5
end

local DATA_KEY_ORDER = {
  'savs', --
  'tiles', --
  'land', --
  'land_arctic_row', --
  'land_ocean_adjacent', --
  'land_ocean_adjacent_cardinal', --
  'land_non_mounds', --
  'count', --
  'stddev', --
  'density', --
  'forest_density_non_mounds', --
  'count_hills_plus_land_rivers', --
  'density_hills_plus_land_rivers', --
  'count_arctic_rows', --
  'num_ranges', --
  'num_ranges_1', --
  'num_ranges_1_per_land', --
  'num_ranges_per_land', --
  'count_range_centers', --
  'density_range_centers', --
  'count_ocean_adjacent', --
  'count_ocean_adjacent_cardinal', --
  'density_ocean_adjacent', --
  'density_ocean_adjacent_cardinal', --
  'count_large_range', --
  'count_large_range_ocean_adjacent', --
  'count_large_range_ocean_adjacent_cardinal', --
  'density_large_range', --
  'density_large_range_ocean_adjacent', --
  'density_large_range_ocean_adjacent_cardinal', --
  'count_mountains_adjacent_to_hills', --
  'count_hills_adjacent_to_mountains', --
  'land_with_biome', --
  'count_with_biome', --
  'density_on_biome', --
}

local OVERLAY_ORDER_CLEARING = {
  'mountains', --
  'hills', --
  'clearing', --
}

local OVERLAY_ORDER_FOREST = {
  'mountains', --
  'hills', --
  'forest', --
}

local function finished( mode )
  do
    local path = format( '%s/%s.json', PLOTS_DIR, mode )
    printfln( 'writing file %s...', path )
    local o = { __key_order=DATA_KEY_ORDER }
    o.savs = D.savs
    o.tiles = D.tiles
    o.land = D.land / D.savs
    o.land_arctic_row = D.land_arctic_row / D.savs
    o.land_ocean_adjacent = D.land_ocean_adjacent / D.savs
    o.land_ocean_adjacent_cardinal =
        D.land_ocean_adjacent_cardinal / D.savs
    o.land_non_mounds = D.land_non_mounds / D.savs
    o.count = { __key_order=OVERLAY_ORDER_FOREST }
    o.count.mountains = D.count.mountains / D.savs
    o.count.hills = D.count.hills / D.savs
    o.count.forest = D.count.forest / D.savs
    o.stddev = { __key_order=OVERLAY_ORDER_FOREST }
    o.stddev.mountains = stddev( D.count.mountains,
                                 D.count.mountains_squared,
                                 D.savs )
    o.stddev.hills = stddev( D.count.hills,
                             D.count.hills_squared, D.savs )
    o.stddev.forest = stddev( D.count.forest,
                              D.count.forest_squared, D.savs )
    o.density = { __key_order=OVERLAY_ORDER_FOREST }
    o.density.mountains = D.count.mountains / D.land
    o.density.hills = D.count.hills / D.land
    o.density.forest = D.count.forest / D.land
    o.forest_density_non_mounds =
        D.count.forest / D.land_non_mounds
    o.count_hills_plus_land_rivers =
        D.count_rivers_on_land + D.count.hills
    o.density_hills_plus_land_rivers =
        (D.count_rivers_on_land + D.count.hills) / D.land
    o.count_arctic_rows = { __key_order=OVERLAY_ORDER_FOREST }
    o.count_arctic_rows.mountains =
        D.count_arctic_rows.mountains / D.savs
    o.count_arctic_rows.hills = D.count_arctic_rows.hills /
                                    D.savs
    o.count_arctic_rows.forest =
        D.count_arctic_rows.forest / D.savs
    o.num_ranges = { __key_order=OVERLAY_ORDER_CLEARING }
    o.num_ranges.mountains = D.num_ranges.mountains / D.savs
    o.num_ranges.hills = D.num_ranges.hills / D.savs
    o.num_ranges.clearing = D.num_ranges.clearing / D.savs
    o.num_ranges_1 = { __key_order=OVERLAY_ORDER_CLEARING }
    o.num_ranges_1.mountains =
        D.count_range_length.mountains[1] / D.savs
    o.num_ranges_1.hills = D.count_range_length.hills[1] / D.savs
    o.num_ranges_1.clearing = D.count_range_length.clearing[1] /
                                  D.savs
    o.num_ranges_1_per_land = {
      __key_order=OVERLAY_ORDER_CLEARING,
    }
    o.num_ranges_1_per_land.mountains =
        D.count_range_length.mountains[1] / D.land
    o.num_ranges_1_per_land.hills =
        D.count_range_length.hills[1] / D.land
    o.num_ranges_1_per_land.clearing =
        D.count_range_length.clearing[1] / D.land
    o.num_ranges_per_land =
        { __key_order=OVERLAY_ORDER_CLEARING }
    o.num_ranges_per_land.mountains =
        D.num_ranges.mountains / D.land
    o.num_ranges_per_land.hills = D.num_ranges.hills / D.land
    o.num_ranges_per_land.clearing =
        D.num_ranges.clearing / D.land
    o.count_range_centers =
        { __key_order=OVERLAY_ORDER_CLEARING }
    o.count_range_centers.mountains =
        D.count_range_centers.mountains / D.savs
    o.count_range_centers.hills =
        D.count_range_centers.hills / D.savs
    o.count_range_centers.clearing =
        D.count_range_centers.clearing / D.savs
    o.density_range_centers = {
      __key_order=OVERLAY_ORDER_CLEARING,
    }
    o.density_range_centers.mountains =
        D.count_range_centers.mountains / D.land
    o.density_range_centers.hills =
        D.count_range_centers.hills / D.land
    o.density_range_centers.clearing =
        D.count_range_centers.clearing / D.land
    o.count_ocean_adjacent =
        { __key_order=OVERLAY_ORDER_CLEARING }
    o.count_ocean_adjacent.mountains =
        D.count_ocean_adjacent.mountains / D.savs
    o.count_ocean_adjacent.hills =
        D.count_ocean_adjacent.hills / D.savs
    o.count_ocean_adjacent.clearing =
        D.count_ocean_adjacent.clearing / D.savs

    o.count_ocean_adjacent_cardinal = {
      __key_order=OVERLAY_ORDER_CLEARING,
    }
    o.count_ocean_adjacent_cardinal.mountains =
        D.count_ocean_adjacent_cardinal.mountains / D.savs
    o.count_ocean_adjacent_cardinal.hills =
        D.count_ocean_adjacent_cardinal.hills / D.savs
    o.count_ocean_adjacent_cardinal.clearing =
        D.count_ocean_adjacent_cardinal.clearing / D.savs
    o.density_ocean_adjacent = {
      __key_order=OVERLAY_ORDER_CLEARING,
    }
    o.density_ocean_adjacent.mountains =
        D.count_ocean_adjacent.mountains / D.land_ocean_adjacent
    o.density_ocean_adjacent.hills =
        D.count_ocean_adjacent.hills / D.land_ocean_adjacent
    o.density_ocean_adjacent.clearing =
        D.count_ocean_adjacent.clearing / D.land_ocean_adjacent
    o.density_ocean_adjacent_cardinal = {
      __key_order=OVERLAY_ORDER_CLEARING,
    }
    o.density_ocean_adjacent_cardinal.mountains =
        D.count_ocean_adjacent_cardinal.mountains /
            D.land_ocean_adjacent_cardinal
    o.density_ocean_adjacent_cardinal.hills =
        D.count_ocean_adjacent_cardinal.hills /
            D.land_ocean_adjacent_cardinal
    o.density_ocean_adjacent_cardinal.clearing =
        D.count_ocean_adjacent_cardinal.clearing /
            D.land_ocean_adjacent_cardinal
    o.count_large_range = { __key_order=OVERLAY_ORDER_CLEARING }
    o.count_large_range.mountains =
        D.count_large_range.mountains / D.savs
    o.count_large_range.hills = D.count_large_range.hills /
                                    D.savs
    o.count_large_range.clearing =
        D.count_large_range.clearing / D.savs
    o.count_large_range_ocean_adjacent = {
      __key_order=OVERLAY_ORDER_CLEARING,
    }
    o.count_large_range_ocean_adjacent.mountains =
        D.count_large_range_ocean_adjacent.mountains / D.savs
    o.count_large_range_ocean_adjacent.hills =
        D.count_large_range_ocean_adjacent.hills / D.savs
    o.count_large_range_ocean_adjacent.clearing =
        D.count_large_range_ocean_adjacent.clearing / D.savs
    o.count_large_range_ocean_adjacent_cardinal = {
      __key_order=OVERLAY_ORDER_CLEARING,
    }
    o.count_large_range_ocean_adjacent_cardinal.mountains =
        D.count_large_range_ocean_adjacent_cardinal.mountains /
            D.savs
    o.count_large_range_ocean_adjacent_cardinal.hills =
        D.count_large_range_ocean_adjacent_cardinal.hills /
            D.savs
    o.count_large_range_ocean_adjacent_cardinal.clearing =
        D.count_large_range_ocean_adjacent_cardinal.clearing /
            D.savs
    o.density_large_range =
        { __key_order=OVERLAY_ORDER_CLEARING }
    o.density_large_range.mountains =
        D.count_large_range.mountains / D.land
    o.density_large_range.hills =
        D.count_large_range.hills / D.land
    o.density_large_range.clearing =
        D.count_large_range.clearing / D.land
    o.density_large_range_ocean_adjacent = {
      __key_order=OVERLAY_ORDER_CLEARING,
    }
    o.density_large_range_ocean_adjacent.mountains =
        D.count_large_range_ocean_adjacent.mountains /
            D.land_ocean_adjacent
    o.density_large_range_ocean_adjacent.hills =
        D.count_large_range_ocean_adjacent.hills /
            D.land_ocean_adjacent
    o.density_large_range_ocean_adjacent.clearing =
        D.count_large_range_ocean_adjacent.clearing /
            D.land_ocean_adjacent
    o.density_large_range_ocean_adjacent_cardinal = {
      __key_order=OVERLAY_ORDER_CLEARING,
    }
    o.density_large_range_ocean_adjacent_cardinal.mountains =
        D.count_large_range_ocean_adjacent_cardinal.mountains /
            D.land_ocean_adjacent_cardinal
    o.density_large_range_ocean_adjacent_cardinal.hills =
        D.count_large_range_ocean_adjacent_cardinal.hills /
            D.land_ocean_adjacent_cardinal
    o.density_large_range_ocean_adjacent_cardinal.clearing =
        D.count_large_range_ocean_adjacent_cardinal.clearing /
            D.land_ocean_adjacent_cardinal
    o.count_mountains_adjacent_to_hills =
        D.count_mountains_adjacent_to_hills / D.savs
    o.count_hills_adjacent_to_mountains =
        D.count_hills_adjacent_to_mountains / D.savs
    o.land_with_biome = { __key_order=BIOME_ORDERING }
    for _, biome in ipairs( BIOME_ORDERING ) do
      o.land_with_biome[biome] =
          assert( D.land_with_biome[biome] )
    end
    o.count_with_biome = { __key_order=OVERLAY_ORDER_CLEARING }
    for _, kind in ipairs( OVERLAY_ORDER_CLEARING ) do
      o.count_with_biome[kind] = { __key_order=BIOME_ORDERING }
      for _, biome in ipairs( BIOME_ORDERING ) do
        o.count_with_biome[kind][biome] = assert(
                                              D.count_with_biome[kind][biome] )
      end
    end
    o.density_on_biome = { __key_order=OVERLAY_ORDER_CLEARING }
    for _, kind in ipairs( OVERLAY_ORDER_CLEARING ) do
      o.density_on_biome[kind] = { __key_order=BIOME_ORDERING }
      for _, biome in ipairs( BIOME_ORDERING ) do
        if D.land_with_biome[biome] > 0 then
          o.density_on_biome[kind][biome] =
              D.count_with_biome[kind][biome] /
                  D.land_with_biome[biome]
        else
          o.density_on_biome[kind][biome] = 0
        end
      end
    end
    json.write_file( path, o, 2 )
  end

  do
    local opts = {
      title=format(
          'Range Length Histogram (empirical) (%s) [%d]', mode,
          D.savs ),
      x_label='Length (cardinal adjacent)',
      y_label='Frequency',
      x_range='1:30',
      y_range='-20:0',
    }

    local csv_data = {
      header={ 'length', 'mountains', 'hills', 'clearing' },
      rows={},
    }

    local max_length = 0
    local total_ranges = 0
    for _, kind in ipairs( OVERLAY_ORDER_CLEARING ) do
      total_ranges = total_ranges + D.num_ranges[kind]
      for length, count in pairs( D.count_range_length[kind] ) do
        if count > 0 then
          max_length = max( max_length, length )
        end
      end
    end
    local length_1_val = {
      mountains=1, --
      hills=1, --
      clearing=1, --
    }
    for i = 1, max_length do
      local row = { i, 0, 0, 0 }
      row[2] = log( D.count_range_length.mountains[i] /
                        total_ranges )
      row[3] =
          log( D.count_range_length.hills[i] / total_ranges )
      row[4] = log( D.count_range_length.clearing[i] /
                        total_ranges )
      if i == 1 then
        length_1_val.mountains = row[2]
        length_1_val.hills = row[3]
        length_1_val.clearing = row[4]
      end
      row[2] = row[2] - length_1_val.mountains
      row[3] = row[3] - length_1_val.hills
      row[4] = row[4] - length_1_val.clearing
      table.insert( csv_data.rows, row )
    end

    local path =
        format( '%s/%s.lengths.gnuplot', PLOTS_DIR, mode )
    plot.line_graph_to_file( path, csv_data, opts )
  end

  do
    local opts = {
      title=format( 'Row Based Densities (empirical) (%s) [%d]',
                    mode, D.savs ),
      x_label='Y (row)',
      y_label='Density',
      x_range='1:70',
      y_range='0:.2',
    }

    local csv_data = {
      header={ 'y', 'mountains', 'hills', 'clearing' },
      rows={},
    }

    for y = 2, 69 do
      local row = { y, 0, 0, 0 }
      row[2] = D.count_by_row.mountains[y] / D.land_by_row[y]
      row[3] = D.count_by_row.hills[y] / D.land_by_row[y]
      row[4] = D.count_by_row.clearing[y] / D.land_by_row[y]
      insert( csv_data.rows, row )
    end

    local path = format( '%s/%s.rows.any.gnuplot', PLOTS_DIR,
                         mode )
    plot.line_graph_to_file( path, csv_data, opts )
  end

  do
    local opts = {
      title=format(
          'Column Based Densities (empirical) (%s) [%d]', mode,
          D.savs ),
      x_label='X (column)',
      y_label='Density',
      x_range='1:56',
      y_range='0:.2',
    }

    local csv_data = {
      header={ 'x', 'mountains', 'hills', 'clearing' },
      rows={},
    }

    for x = 1, 56 do
      local row = { x, 0, 0, 0 }
      row[2] = D.count_by_col.mountains[x] / D.land_by_col[x]
      row[3] = D.count_by_col.hills[x] / D.land_by_col[x]
      row[4] = D.count_by_col.clearing[x] / D.land_by_col[x]
      insert( csv_data.rows, row )
    end

    local path = format( '%s/%s.cols.any.gnuplot', PLOTS_DIR,
                         mode )
    plot.line_graph_to_file( path, csv_data, opts )
  end

  do
    local opts = {
      title=format(
          'Row Based Range Center Densities (empirical) (%s) [%d]',
          mode, D.savs ),
      x_label='Y (row)',
      y_label='Density',
      x_range='1:70',
      y_range='0:.1',
    }

    local csv_data = {
      header={ 'y', 'mountains', 'hills', 'clearing' },
      rows={},
    }

    for y = 2, 69 do
      local row = { y, 0, 0, 0 }
      row[2] = D.count_range_centers_by_row.mountains[y] /
                   D.land_by_row[y]
      row[3] = D.count_range_centers_by_row.hills[y] /
                   D.land_by_row[y]
      row[4] = D.count_range_centers_by_row.clearing[y] /
                   D.land_by_row[y]
      insert( csv_data.rows, row )
    end

    local path = format( '%s/%s.rows.centers.gnuplot', PLOTS_DIR,
                         mode )
    plot.line_graph_to_file( path, csv_data, opts )
  end

  do
    local opts = {
      title=format(
          'Column Based Range Center Densities (empirical) (%s) [%d]',
          mode, D.savs ),
      x_label='X (column)',
      y_label='Density',
      x_range='1:56',
      y_range='0:.1',
    }

    local csv_data = {
      header={ 'x', 'mountains', 'hills', 'clearing' },
      rows={},
    }

    for x = 1, 56 do
      local row = { x, 0, 0, 0 }
      row[2] = D.count_range_centers_by_col.mountains[x] /
                   D.land_by_col[x]
      row[3] = D.count_range_centers_by_col.hills[x] /
                   D.land_by_col[x]
      row[4] = D.count_range_centers_by_col.clearing[x] /
                   D.land_by_col[x]
      insert( csv_data.rows, row )
    end

    local path = format( '%s/%s.cols.centers.gnuplot', PLOTS_DIR,
                         mode )
    plot.line_graph_to_file( path, csv_data, opts )
  end

  do
    local opts = {
      title=format(
          'Row Based Large Densities (empirical) (%s) [%d]',
          mode, D.savs ),
      x_label='Y (row)',
      y_label='Density',
      x_range='1:70',
      y_range='0:.005',
    }

    local csv_data = {
      header={ 'y', 'mountains/10', 'hills', 'clearing/10' },
      rows={},
    }

    for y = 2, 69 do
      local row = { y, 0, 0, 0 }
      row[2] = D.count_large_range_by_row.mountains[y] /
                   D.land_by_row[y] / 10
      row[3] = D.count_large_range_by_row.hills[y] /
                   D.land_by_row[y]
      row[4] = D.count_large_range_by_row.clearing[y] /
                   D.land_by_row[y] / 10
      insert( csv_data.rows, row )
    end

    local path = format( '%s/%s.rows.large.gnuplot', PLOTS_DIR,
                         mode )
    plot.line_graph_to_file( path, csv_data, opts )
  end

  do
    local opts = {
      title=format(
          'Column Based Large Densities (empirical) (%s) [%d]',
          mode, D.savs ),
      x_label='X (column)',
      y_label='Density',
      x_range='1:56',
      y_range='0:.005',
    }

    local csv_data = {
      header={ 'x', 'mountains/10', 'hills', 'clearing/10' },
      rows={},
    }

    for x = 1, 56 do
      local row = { x, 0, 0, 0 }
      row[2] = D.count_large_range_by_col.mountains[x] /
                   D.land_by_col[x] / 10
      row[3] = D.count_large_range_by_col.hills[x] /
                   D.land_by_col[x]
      row[4] = D.count_large_range_by_col.clearing[x] /
                   D.land_by_col[x] / 10
      insert( csv_data.rows, row )
    end

    local path = format( '%s/%s.cols.large.gnuplot', PLOTS_DIR,
                         mode )
    plot.line_graph_to_file( path, csv_data, opts )
  end

  for _, type in ipairs{ 'mountains', 'hills', 'clearing' } do
    local name = assert( ({
      mountains='Mountains',
      hills='Hills',
      clearing='Clearing',
    })[type] )
    do
      local opts = {
        title=format( '%s on Biome by Row (empirical) (%s) [%d]',
                      name, mode, D.savs ),
        x_label='Y', --
        y_label='Count per Row', --
        x_range='1:70', --
        y_range='0:2', --
      }
      local csv_data = { header={ 'y' }, rows={} }
      for _, biome in ipairs( BIOME_ORDERING ) do
        insert( csv_data.header, biome )
      end
      for y = 1, 70 do
        local row = { y }
        for _, biome in ipairs( BIOME_ORDERING ) do
          local val = D.count_with_biome_by_row[type][y][biome] /
                          D.savs
          insert( row, val )
        end
        insert( csv_data.rows, row )
      end
      local path = format( '%s/%s.biome.counts.rows.%s.gnuplot',
                           PLOTS_DIR, mode, type )
      plot.line_graph_to_file( path, csv_data, opts )
    end

    do
      local opts = {
        title=format(
            '%s Density on Biome by Row (empirical) (%s) [%d]',
            name, mode, D.savs ),
        x_label='Y', --
        y_label='Density', --
        x_range='1:70', --
        y_range='0:.3', --
      }
      local csv_data = { header={ 'y' }, rows={} }
      for _, biome in ipairs( BIOME_ORDERING ) do
        insert( csv_data.header, biome )
      end
      for y = 1, 70 do
        local row = { y }
        for _, biome in ipairs( BIOME_ORDERING ) do
          if D.land_with_biome_by_row[y][biome] > 0 then
            local val =
                D.count_with_biome_by_row[type][y][biome] /
                    D.land_with_biome_by_row[y][biome]
            insert( row, val )
          else
            insert( row, 0 )
          end
        end
        insert( csv_data.rows, row )
      end
      local path = format( '%s/%s.biome.density.rows.%s.gnuplot',
                           PLOTS_DIR, mode, type )
      plot.line_graph_to_file( path, csv_data, opts )
    end
  end
end

-----------------------------------------------------------------
-- Collection stage.
-----------------------------------------------------------------
-- This gets run once after all modes are run (which happen in
-- separate processes) to collect all of their results.
local function collect()
  print( 'collecting...' )
  local MODES = {
    'bbtt', 'bbmm', 'bbbb', 'bbtm', 'bbbm', 'bbmt', 'bbmb',
    'tmmm', 'mmmm', 'bmmm', 'mtmm', 'mbmm', 'new',
  }
  local KEY_ORDER = {
    'savs', --
    'density', --
    'forest_density_non_mounds', --
    'density_hills_plus_land_rivers', --
    'num_ranges_1_per_land', --
    'num_ranges_per_land', --
    'density_ocean_adjacent', --
    'density_ocean_adjacent_cardinal', --
    'density_large_range', --
    'density_large_range_ocean_adjacent', --
    'density_large_range_ocean_adjacent_cardinal', --
    'density_on_biome', --
  }
  local o = {
    __key_order={
      'properties', --
      'modes', --
    },
  }
  o.modes = { __key_order=MODES }
  o.properties = {
    __key_order={
      'mountain_density', --
      'hills_density', --
      'forest_density', --
      'forest_density_non_mounds', --
    },
    mountain_density={ __key_order=MODES },
    hills_density={ __key_order=MODES },
    forest_density={ __key_order=MODES },
    forest_density_non_mounds={ __key_order=MODES },
  }
  for _, mode in ipairs( MODES ) do
    do
      local path = format( '%s/%s.json', PLOTS_DIR, mode )
      printfln( 'reading file %s...', path )
      local F = json.read_file( path )
      o.modes[mode] = { __key_order=KEY_ORDER }
      local om = o.modes[mode]
      for _, key in ipairs( KEY_ORDER ) do
        om[key] = assert( F[key] )
        if type( om[key] ) == 'table' and om[key].clearing then
          om[key].__key_order = OVERLAY_ORDER_CLEARING
        end
        if type( om[key] ) == 'table' and om[key].forest then
          om[key].__key_order = OVERLAY_ORDER_FOREST
        end
      end
      om.density_on_biome.mountains.__key_order = BIOME_ORDERING
      om.density_on_biome.hills.__key_order = BIOME_ORDERING
      om.density_on_biome.clearing.__key_order = BIOME_ORDERING
    end
    local op = o.properties
    op.mountain_density[mode] = o.modes[mode].density.mountains
    op.hills_density[mode] = o.modes[mode].density.hills
    op.forest_density[mode] = o.modes[mode].density.forest
    op.forest_density_non_mounds[mode] = o.modes[mode]
                                             .forest_density_non_mounds
  end
  local path = format( '%s/overlays.json', PLOTS_DIR )
  json.write_file( path, o, 2 )
end

-----------------------------------------------------------------
-- Lambda.
-----------------------------------------------------------------
return { lambda=lambda, finished=finished, collect=collect }