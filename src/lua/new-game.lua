--[[ ------------------------------------------------------------
|
| new-game.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2022-05-22.
|
| Description: Creates a new game.
|
--]] ------------------------------------------------------------
local M = {}

local map_gen = require( 'map-gen' )

-----------------------------------------------------------------
-- Options
-----------------------------------------------------------------
function M.default_options()
  return {
    difficulty='discoverer',
    -- This determines which nations are enabled and some proper-
    -- ties. If initial ship position is null then the randomly
    -- generated one will be used.
    --
    -- FIXME: this needs to be a list instead of a dict because
    -- iteration order on a table in lua is non-deterministic.
    nations={
      ['english']={ ship_pos=nil },
      ['french']={ ship_pos=nil },
      ['spanish']={ ship_pos=nil },
      ['dutch']={ ship_pos=nil }
    },
    human_nation=nil, -- no humans.
    map={} -- use default map options.
  }
end

-----------------------------------------------------------------
-- Settings
-----------------------------------------------------------------
local function set_default_settings( options, settings )
  settings.difficulty = options.difficulty
  -- FIXME: these are in config/rn... get them from there.
  settings.game_options.flags.show_indian_moves = true
  settings.game_options.flags.show_foreign_moves = true
  settings.game_options.flags.fast_piece_slide = false
  settings.game_options.flags.end_of_turn = false
  settings.game_options.flags.autosave = true
  settings.game_options.flags.combat_analysis = true
  settings.game_options.flags.water_color_cycling = true
  settings.game_options.flags.tutorial_hints = false
  settings.game_options.flags.show_fog_of_war = true

  if options.difficulty == 'discoverer' then
    settings.game_options.flags.tutorial_hints = true
  end
end

-----------------------------------------------------------------
-- Units
-----------------------------------------------------------------
local function build_unit_type( type, base_type )
  if base_type == nil then
    return unit_composer.UnitComposition.create_with_type_obj(
               unit_type.UnitType.create( type ) )
  else
    return unit_composer.UnitComposition.create_with_type_obj(
               unit_type.UnitType.create_with_base( type,
                                                    base_type ) )
  end
end

-- TODO: in the OG, the ships are not actually created on the
-- map; they are created on the high seas setting sail for the
-- new world, such that on their first turn they will be on the
-- map. So the initial ship position should simply be used to
-- initialize the player.last_high_seas field so that the ships
-- will be placed where we want them.
local function create_initial_units_for_nation(options, nation,
                                               root )
  local player = root.players.players:get( nation )
  local coord = assert( options.nations[nation].ship_pos )

  -- Ship.
  local ship_type
  if nation == 'dutch' then
    ship_type = build_unit_type( 'merchantman' )
  else
    ship_type = build_unit_type( 'caravel' )
  end
  local ship_unit = unit_mgr.create_unit_on_map( nation,
                                                 ship_type, coord )

  -- Soldier.
  local soldier_type
  if nation == 'spanish' or options.difficulty == 'discoverer' then
    soldier_type = build_unit_type( 'veteran_soldier' )
  else
    soldier_type = build_unit_type( 'soldier' )
  end

  -- Pioneer.
  local pioneer_type
  if nation == 'french' then
    pioneer_type = build_unit_type( 'hardy_pioneer' )
  else
    pioneer_type = build_unit_type( 'pioneer' )
  end

  unit_mgr.create_unit_in_cargo( nation, soldier_type,
                                 ship_unit:id() )
  unit_mgr.create_unit_in_cargo( nation, pioneer_type,
                                 ship_unit:id() )
  player.starting_position = coord
end

local function create_initial_units( options, root )
  for nation, _ in pairs( options.nations ) do
    create_initial_units_for_nation( options, nation, root )
  end
end

local function create_battlefield_units( options, root )
  local nation1
  local nation2
  for nation, _ in pairs( options.nations ) do
    nation1 = nation2
    nation2 = nation
    if nation1 then break end
  end
  assert( nation1 )
  assert( nation2 )
  local veteran_dragoon = build_unit_type( 'veteran_dragoon' )

  local size = root.terrain:size()

  local coord = { x=size.w // 2 - 1, y=size.h // 2 - 1 }
  unit_mgr.create_unit_on_map( nation1, veteran_dragoon, coord )
  coord.y = coord.y + 1
  unit_mgr.create_unit_on_map( nation1, veteran_dragoon, coord )
  coord.y = coord.y + 1

  coord = { x=size.w // 2, y=size.h // 2 - 1 }
  unit_mgr.create_unit_on_map( nation2, veteran_dragoon, coord )
  coord.y = coord.y + 1
  unit_mgr.create_unit_on_map( nation2, veteran_dragoon, coord )
  coord.y = coord.y + 1
end

-- FIXME: temporary
local function create_all_units( options, root )
  local nation1
  for nation, _ in pairs( options.nations ) do
    nation1 = nation
    if nation1 then break end
  end
  assert( nation1 )

  local size = ROOT.terrain:size()
  local origin = { x=size.w // 2 - 8, y=size.h // 2 - 4 }

  local land_units = {
    'petty_criminal', 'indentured_servant', 'free_colonist',
    'native_convert', 'soldier', 'dragoon', 'pioneer',
    'missionary', 'scout', 'expert_farmer', 'expert_fisherman',
    'expert_sugar_planter', 'expert_tobacco_planter',
    'expert_cotton_planter', 'expert_fur_trapper',
    'expert_lumberjack', 'expert_ore_miner',
    'expert_silver_miner', 'master_carpenter',
    'master_distiller', 'master_tobacconist', 'master_weaver',
    'master_fur_trader', 'master_blacksmith', 'master_gunsmith',
    'elder_statesman', 'firebrand_preacher', 'hardy_colonist',
    'jesuit_colonist', 'seasoned_colonist', 'veteran_colonist',
    'veteran_soldier', 'veteran_dragoon', 'continental_army',
    'continental_cavalry', 'regular', 'cavalry', 'hardy_pioneer',
    'jesuit_missionary', 'seasoned_scout', 'artillery',
    'damaged_artillery', 'wagon_train', 'treasure'
  }

  local function create( where, unit_name )
    local unit = unit_mgr.create_unit_on_map( nation1,
                                              build_unit_type(
                                                  unit_name ),
                                              where )
    unit:fortify()
  end

  for i, unit_name in ipairs( land_units ) do
    local coord = {
      x=origin.x + (i - 1) % 7,
      y=origin.y + (i - 1) // 7
    }
    create( coord, unit_name )
  end

  origin = { x=size.w // 2 + 2, y=size.h // 2 - 1 }

  -- Ships
  local ships = {
    'caravel', 'merchantman', 'galleon', 'privateer', 'frigate',
    'man_o_war'
  }

  for i, unit_name in ipairs( ships ) do
    local coord = {
      x=origin.x + (i - 1) % 3,
      y=origin.y + (i - 1) // 3
    }
    create( coord, unit_name )
  end
end

-----------------------------------------------------------------
-- Players State
-----------------------------------------------------------------
local function create_old_world_state( settings, player )
  local old_world = player.old_world
  -- Immigrants state.
  old_world.immigration.immigrants_pool[1] =
      immigration.pick_next_unit_for_pool( player, settings )
  old_world.immigration.immigrants_pool[2] =
      immigration.pick_next_unit_for_pool( player, settings )
  old_world.immigration.immigrants_pool[3] =
      immigration.pick_next_unit_for_pool( player, settings )

  -- Tax rate.
  old_world.taxes.tax_rate = 0

  -- Expeditionary force.
  old_world.expeditionary_force.regulars = 3
  old_world.expeditionary_force.cavalry = 2
  old_world.expeditionary_force.artillery = 2
  old_world.expeditionary_force.men_of_war = 3
end

local function init_non_processed_goods_prices( options, players )
  -- Initializes the same commodity for all players to the same
  -- value.
  local init_commodity = function( comm, bid_price )
    local limits = market.starting_price_limits( comm )
    local min = assert( limits.bid_price_start_min )
    local max = assert( limits.bid_price_start_max )
    assert( min <= max )
    local bid_price = math.random( min, max )
    for nation, tbl in pairs( options.nations ) do
      local player = players:get( nation )
      player.old_world.market.commodities[comm].bid_price =
          bid_price
    end
  end

  init_commodity( 'food' )
  init_commodity( 'sugar' )
  init_commodity( 'tobacco' )
  init_commodity( 'cotton' )
  init_commodity( 'fur' )
  init_commodity( 'lumber' )
  init_commodity( 'ore' )
  init_commodity( 'silver' )
  init_commodity( 'horses' )
  -- Skip processed goods.
  init_commodity( 'trade_goods' )
  init_commodity( 'tools' )
  init_commodity( 'muskets' )
end

local function init_processed_goods_prices(options, players, root )
  -- This will create a price group object which will randomly
  -- initialize the starting volumes in the correct way and allow
  -- us to get the initial prices. Then we assign those same
  -- prices to all players.
  local group = price_group.ProcessedGoodsPriceGroup
                    .new_with_random_volumes()
  local eq_ask_prices = group:equilibrium_prices()
  for _, comm in ipairs{ 'rum', 'cigars', 'cloth', 'coats' } do
    for nation, tbl in pairs( options.nations ) do
      local player = players:get( nation )
      local c = player.old_world.market.commodities[comm]
      local spread = market.bid_ask_spread( comm )
      c.bid_price = eq_ask_prices[comm] - spread
      c.intrinsic_volume = 0 -- not used.
    end
    root.players.global_market_state.commodities[comm]
        .intrinsic_volume = group:intrinsic_volume( comm )
  end
end

-- This needs to be done for all players together, since all of
-- their markets should start out with the same (random) prices.
local function init_prices( options, root )
  local players = root.players.players
  init_non_processed_goods_prices( options, players )
  init_processed_goods_prices( options, players, root )
end

local STARTING_GOLD = {
  discoverer=1000,
  explorer=300,
  conquistador=0,
  governor=0,
  viceroy=0
}

local function create_player_state( settings, nation, player )
  player.nation = nation
  player.money = assert( STARTING_GOLD[settings.difficulty] )
  create_old_world_state( settings, player )
end

local function create_nations( options, root )
  local players = root.players.players
  local settings = root.settings
  for nation, tbl in pairs( options.nations ) do
    local player = players:reset_player( nation )
    create_player_state( settings, nation, player )
  end
  root.players.human = options.human_nation
  init_prices( options, root )
end

local function create_player_maps( options, root )
  local terrain = root.terrain
  for nation, tbl in pairs( options.nations ) do
    terrain:initialize_player_terrain( nation, --[[visible=]]
                                       false )
  end
end

-----------------------------------------------------------------
-- Turn State
-----------------------------------------------------------------
local function create_turn_state( turns_state )
  turns_state.time_point.year = 1492
  turns_state.time_point.season = 'spring'
end

-----------------------------------------------------------------
-- Testing
-----------------------------------------------------------------
local function add_testing_options( options )
  options.nations = {
    -- english={ ship_pos=nil },
    french={ ship_pos=nil }
    -- spanish={ ship_pos=nil },
    -- dutch={ ship_pos=nil },
  }
  options.human_nation = 'french'
  options.difficulty = 'conquistador'
  -- options.map.type = 'land-partition'
  -- options.map.world_size = { w=4, h=4 }
end

-----------------------------------------------------------------
-- Creates a new Game
-----------------------------------------------------------------
-- This should be called af the player decides the parameters of
-- the game, which should be passed in as options here. Also, the
-- save-game state should be default-constructed before calling
-- this.
function M.create( options )
  options = options or {}
  -- Merge the options with the default ones so that any missing
  -- fields will have their default values.
  for k, v in pairs( M.default_options() ) do
    if options[k] == nil then options[k] = v end
  end

  add_testing_options( options )

  local root = ROOT

  set_default_settings( options, root.settings )

  create_turn_state( root.turn )

  create_nations( options, root )

  -- Do this as late as possible because it's slow and we want to
  -- catch errors in the other parts of the process as quickly as
  -- possible.
  map_gen.generate( options.map )

  -- Needs to be done after the map is generated because we need
  -- to know the dimensions.
  do
    -- These random ship positions should not be accessed di-
    -- rectly hereafter; we should use the ones in the options,
    -- just in case the user overrided it.
    local random_ship_positions = map_gen.initial_ships_pos()
    for nation, info in pairs( options.nations ) do
      if info.ship_pos == nil then
        info.ship_pos = random_ship_positions[nation]
      end
    end
  end

  -- Initializes the maps that track what each player can see.
  create_player_maps( options, root )

  local world_size = root.terrain:size()
  root.land_view.viewport:set_world_size_tiles( world_size )

  if options.map.type == 'battlefield' then
    create_battlefield_units( options, root )
  elseif options.map.type == 'half_and_half' then
    -- FIXME: temporary
    create_all_units( options, root )
  else
    create_initial_units( options, root )
  end

  -- Temporary.
  local all_nations = { 'english', 'french', 'spanish', 'dutch' }
  for _, nation in ipairs( all_nations ) do
    if options.nations[nation] then
      if options.nations[nation].human then
        local coord = assert( options.nations[nation].ship_pos )
        root.land_view.viewport:center_on_tile( coord )
        break
      end
    end
  end
end

return M
