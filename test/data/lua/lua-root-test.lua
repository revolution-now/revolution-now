local ENABLE_PRINT = false

local function print( ... )
  if ENABLE_PRINT then _ENV.print( ... ) end
end

local root = assert( _G['root'] )

-----------------------------------------------------------------
-- LuaFormatter off
-----------------------------------------------------------------

assert( root ~= nil, 'root' )

print( 'root.version: ' .. tostring( root.version ) )
assert( root.version ~= nil, 'root.version' )
print( 'root.version.major: ' .. tostring( root.version.major ) )
assert( root.version.major ~= nil, 'root.version.major' )
print( 'root.version.minor: ' .. tostring( root.version.minor ) )
assert( root.version.minor ~= nil, 'root.version.minor' )
print( 'root.version.patch: ' .. tostring( root.version.patch ) )
assert( root.version.patch ~= nil, 'root.version.patch' )

print( 'root.events: ' .. tostring( root.events ) )
assert( root.events ~= nil, 'root.events' )
root.events.war_of_succession_done = 'french'
print( 'root.events.war_of_succession_done: ' .. tostring( root.events.war_of_succession_done ) )
assert( root.events.war_of_succession_done ~= nil, 'root.events.war_of_succession_done' )
print( 'root.events.tutorial_hints: ' .. tostring( root.events.tutorial_hints ) )
assert( root.events.tutorial_hints ~= nil, 'root.events.tutorial_hints' )
print( 'root.events.one_time_help: ' .. tostring( root.events.one_time_help ) )
assert( root.events.one_time_help ~= nil, 'root.events.one_time_help' )
print( 'root.events.one_time_help.showed_no_sail_high_seas_during_war: ' .. tostring( root.events.one_time_help.showed_no_sail_high_seas_during_war ) )
assert( root.events.one_time_help.showed_no_sail_high_seas_during_war ~= nil, 'root.events.one_time_help.showed_no_sail_high_seas_during_war' )
print( 'root.events.ref_captured_colony: ' .. tostring( root.events.ref_captured_colony ) )
assert( root.events.ref_captured_colony ~= nil, 'root.events.ref_captured_colony' )

root.map.depletion.counters[{ x=0, y=1 }] = 3

print( 'root.map: ' .. tostring( root.map ) )
assert( root.map ~= nil, 'root.map' )
print( 'root.map.depletion: ' .. tostring( root.map.depletion ) )
assert( root.map.depletion ~= nil, 'root.map.depletion' )
print( 'root.map.depletion.counters: ' .. tostring( root.map.depletion.counters ) )
assert( root.map.depletion.counters ~= nil, 'root.map.depletion.counters' )
print( 'root.map.depletion.counters.size: ' .. tostring( root.map.depletion.counters.size ) )
print( 'root.map.depletion.counters:size(): ' .. tostring( root.map.depletion.counters:size() ) )
assert( root.map.depletion.counters:size() == 1 )
print( 'root.map.depletion.counters[{ x=0, y=1 }]: ' .. tostring( root.map.depletion.counters[{ x=0, y=1 }] ) )
print( 'root.map.depletion.counters[{ x=0, y=2 }]: ' .. tostring( root.map.depletion.counters[{ x=0, y=2 }] ) )
assert( root.map.depletion.counters[{ x=0, y=1 }] == 3 )
assert( root.map.depletion.counters[{ x=0, y=2 }] == nil )
root.map.depletion.counters[{ x=8, y=1 }] = 7
root.map.depletion.counters[{ x=9, y=2 }] = 8
root.map.depletion.counters[{ x=3, y=4 }] = 5
assert( root.map.depletion.counters[{ x=8, y=1 }] == 7 )
assert( root.map.depletion.counters[{ x=9, y=2 }] == 8 )
assert( root.map.depletion.counters[{ x=3, y=4 }] == 5 )
print( 'root.map.depletion.counters: ' .. tostring( root.map.depletion.counters ) )
assert( root.map.depletion.counters ~= nil, 'root.map.depletion.counters' )
print( 'root.map.depletion.counters:size(): ' .. tostring( root.map.depletion.counters:size() ) )
assert( root.map.depletion.counters:size() == 4 )

print( 'root.settings.in_game_options.game_menu_options.autosave: ' .. tostring( root.settings.in_game_options.game_menu_options.autosave ) )
assert( root.settings.in_game_options.game_menu_options.autosave == false )
root.settings.in_game_options.game_menu_options.autosave = true
print( 'root.settings.in_game_options.game_menu_options.autosave: ' .. tostring( root.settings.in_game_options.game_menu_options.autosave ) )
assert( root.settings.in_game_options.game_menu_options.autosave == true )
root.settings.in_game_options.game_menu_options.autosave = false
print( 'root.settings.in_game_options.game_menu_options.autosave: ' .. tostring( root.settings.in_game_options.game_menu_options.autosave ) )
assert( root.settings.in_game_options.game_menu_options.autosave == false )

print( 'root.land_view.minimap.origin: ' .. tostring( root.land_view.minimap.origin ) )
print( 'root.land_view.minimap.origin.y: ' .. tostring( root.land_view.minimap.origin.y ) )
assert( root.land_view.minimap.origin.y == 0 )
root.land_view.minimap.origin.y = 2.3
print( 'root.land_view.minimap.origin.y: ' .. tostring( root.land_view.minimap.origin.y ) )
assert( root.land_view.minimap.origin.y == 2.3 )

print( 'root.land_view.map_revealed: ' .. tostring( root.land_view.map_revealed ) )
print( 'root.land_view.map_revealed.no_special_view: ' .. tostring( root.land_view.map_revealed.no_special_view ) )
print( 'root.land_view.map_revealed.entire: ' .. tostring( root.land_view.map_revealed.entire ) )
print( 'root.land_view.map_revealed.player: ' .. tostring( root.land_view.map_revealed.player ) )
assert( root.land_view.map_revealed.no_special_view ~= nil )
assert( root.land_view.map_revealed.entire == nil )
assert( root.land_view.map_revealed.player == nil )
print( 'setting player...' )
root.land_view.map_revealed.select_player()
root.land_view.map_revealed.player.type = 'french'
assert( root.land_view.map_revealed.no_special_view == nil )
assert( root.land_view.map_revealed.entire == nil )
assert( root.land_view.map_revealed.player ~= nil )
print( 'root.land_view.map_revealed.no_special_view: ' .. tostring( root.land_view.map_revealed.no_special_view ) )
print( 'root.land_view.map_revealed.entire: ' .. tostring( root.land_view.map_revealed.entire ) )
print( 'root.land_view.map_revealed.player: ' .. tostring( root.land_view.map_revealed.player ) )
print( 'root.land_view.map_revealed.player.type: ' .. tostring( root.land_view.map_revealed.player.type ) )
assert( root.land_view.map_revealed.player.type == 'french' )

print( 'root.turn.time_point.season: ' .. tostring( root.turn.time_point.season ) )
assert( root.turn.time_point.season == 'winter' )
print( 'root.turn.cycle.player: ' .. tostring( root.turn.cycle.player ) )
assert( root.turn.cycle.player == nil )
root.turn.cycle.select_player()
assert( root.turn.cycle.player ~= nil )
print( 'root.turn.cycle.player: ' .. tostring( root.turn.cycle.player ) )
print( 'root.turn.cycle.player.st.units: ' .. tostring( root.turn.cycle.player.st.units ) )
assert( root.turn.cycle.player.st.units == nil )
assert( not pcall( function()
  root.turn.cycle.player.st.select_unitsx()
end ) )
root.turn.cycle.player.st.select_units()
assert( root.turn.cycle.player.st.units ~= nil )
print( 'root.turn.cycle.player.st.units.skip_eot: ' .. tostring( root.turn.cycle.player.st.units.skip_eot ) )
assert( root.turn.cycle.player.st.units.skip_eot == false )
root.turn.cycle.player.st.units.skip_eot = true
print( 'root.turn.cycle.player.st.units.skip_eot: ' .. tostring( root.turn.cycle.player.st.units.skip_eot ) )
assert( root.turn.cycle.player.st.units.skip_eot == true )
print( 'root.turn.cycle.player.st.units.q:size(): ' .. tostring( root.turn.cycle.player.st.units.q:size() ) )
assert( root.turn.cycle.player.st.units.q:size() == 0 )

print( 'root.players.players: ' .. tostring( root.players.players ) )
print( 'root.players.players.french: ' .. tostring( root.players.players.french ) )
assert( not root.players.players.french:has_value() )
root.players.players.french:reset()
assert( not root.players.players.french:has_value() )
local french = root.players.players.french:emplace()
print( 'root.players.players.french.type: ' .. tostring( root.players.players.french.type ) )
print( 'root.players.players.french.nation: ' .. tostring( root.players.players.french.nation ) )
assert( root.players.players.french.type == 'english' )
assert( root.players.players.french.nation == 'english' )
french.type = 'french'
french.nation = 'french'
print( 'root.players.players.french:has_value(): ' .. tostring( root.players.players.french:has_value() ) )
assert( root.players.players.french:has_value() )
assert( root.players.players.french.type == 'french' )
assert( root.players.players.french.nation == 'french' )
assert( not pcall( function()
  root.players.players.spanish.nation = 'spanish'
end ) )
assert( not pcall( function()
  local _ =  root.players.players.spanish.nation == 'spanish'
end ) )
assert( root.players.players.french:value().nation == 'french' )
assert( not pcall( function()
  local _ =  root.players.players.spanish:value().nation == 'french'
end ) )

assert( root.trade_routes )
print( 'root.trade_routes: ' .. tostring( root.trade_routes ) )
assert( not root.trade_routes.routes[1] )
print( 'root.trade_routes[1]: ' .. tostring( root.trade_routes[1] ) )
local route_1 = assert( root.trade_routes.routes:make( 1 ) )
print( 'route_1: ' .. tostring( route_1 ) )
route_1.id = 5
print( 'route_1.id: ' .. tostring( route_1.id ) )
assert( route_1.id == 5 )
print( 'route_1.name: ' .. tostring( route_1.name ) )
route_1.name = 'hello'
assert( route_1.name == 'hello' )
print( 'route_1.name: ' .. tostring( route_1.name ) )
route_1.player = 'spanish'
print( 'route_1.player: ' .. tostring( route_1.player ) )
assert( route_1.player == 'spanish' )
route_1.type = 'sea'
print( 'route_1.type: ' .. tostring( route_1.type ) )
assert( route_1.type == 'sea' )
assert( route_1.stops:size() == 0 )
local stop_1 = route_1.stops:add()
assert( route_1.stops:size() == 1 )
print( 'stop_1: ' .. tostring( stop_1 ) )
print( 'stop_1.target: ' .. tostring( stop_1.target ) )
print( 'stop_1.loads: ' .. tostring( stop_1.loads ) )
print( 'stop_1.unloads: ' .. tostring( stop_1.unloads ) )
print( 'stop_1.target: ' .. tostring( stop_1.target ) )
local target_colony = stop_1.target.select_colony()
assert( stop_1.target.colony )
print( 'stop_1.target: ' .. tostring( stop_1.target ) )
target_colony.colony_id = 7
print( 'stop_1.target: ' .. tostring( stop_1.target ) )
assert( stop_1.target.colony.colony_id == 7 )
local harbor = stop_1.target.select_harbor()
assert( harbor )
assert( stop_1.target.harbor )
print( 'stop_1.target: ' .. tostring( stop_1.target ) )
print( 'stop_1.loads: ' .. tostring( stop_1.loads ) )
assert( stop_1.loads:size() == 0 )
stop_1.loads:add()
assert( stop_1.loads:size() == 1 )
print( 'stop_1.loads: ' .. tostring( stop_1.loads ) )
stop_1.loads[1] = 'lumber'
print( 'stop_1.loads: ' .. tostring( stop_1.loads ) )
assert( stop_1.loads[1] == 'lumber' )
assert( not pcall( function()
  stop_1.loads[2] = 'tools'
end ) )
assert( stop_1.loads:size() == 1 )
print( 'stop_1.loads: ' .. tostring( stop_1.loads ) )
stop_1.loads:add()
assert( stop_1.loads:size() == 2 )
stop_1.loads[2] = 'tools'
assert( stop_1.loads:size() == 2 )
print( 'stop_1.loads: ' .. tostring( stop_1.loads ) )
assert( stop_1.loads[2] == 'tools' )
stop_1.loads:clear()
assert( not pcall( function()
  stop_1.loads[1] = 'tools'
end ) )
assert( stop_1.loads:size() == 0 )
print( 'stop_1.loads: ' .. tostring( stop_1.loads ) )

local old_world = root.players.old_world.french
assert( old_world )
print( 'old_world.harbor_state.selected_unit: ' .. tostring( old_world.harbor_state.selected_unit ) )
assert( old_world.harbor_state.selected_unit == nil )
old_world.harbor_state.selected_unit = 5
print( 'old_world.harbor_state.selected_unit: ' .. tostring( old_world.harbor_state.selected_unit ) )
assert( old_world.harbor_state.selected_unit == 5 )
old_world.harbor_state.selected_unit = nil
print( 'old_world.harbor_state.selected_unit: ' .. tostring( old_world.harbor_state.selected_unit ) )
assert( old_world.harbor_state.selected_unit == nil )

local immigration = assert( old_world.immigration )
print( 'immigration.immigrants_pool: ' .. tostring( immigration.immigrants_pool ) )
local pool = immigration.immigrants_pool
assert( pool )
print( 'pool:size(): ' .. pool:size() )
assert( pool:size() == 3 )
assert( pool[2] == 'petty_criminal' )
assert( not pcall( function()
  pool[2] = 'veteran_dragoonx'
end) )
pool[2] = 'veteran_dragoon'
assert( pool:size() == 3 )
assert( pool[2] == 'veteran_dragoon' )
print( 'pool: ' .. tostring( pool ) )
assert( not pcall( function()
  pool[4] = 'free_colonist'
end ) )

local market = assert( old_world.market )
assert( not pcall( function()
  local _ = market.commodities.orex.boycott == false
end ) )
print( 'market.commodities.ore.boycott: ' .. tostring( market.commodities.ore.boycott ) )
assert( market.commodities.ore.boycott == false )
market.commodities.ore.boycott = true
print( 'market.commodities.ore.boycott: ' .. tostring( market.commodities.ore.boycott ) )
assert( market.commodities.ore.boycott == true )

french = root.players.players.french
assert( french )
print( 'french.new_world_name: ' .. tostring( french.new_world_name ) )
assert( french.new_world_name == nil )
french.new_world_name = 'my world'
print( 'french.new_world_name: ' .. tostring( french.new_world_name ) )
assert( french.new_world_name == 'my world' )

print( 'french.starting_position: ' .. tostring( french.starting_position ) )
assert( french.starting_position )
assert( french.starting_position.x == 0 )
assert( french.starting_position.y == 0 )
french.starting_position = { x=4, y=5 }
assert( french.starting_position.x == 4 )
assert( french.starting_position.y == 5 )
assert( french.starting_position == { x=4, y=5 } )
assert( french.starting_position ~= { x=5, y=5 } )
french.starting_position.y = 9
assert( french.starting_position.y == 5 ) -- !! because starting_position pops as a a lua table.
print( 'french.starting_position: ' .. tostring( french.starting_position ) )

print( 'french.last_high_seas: ' .. tostring( french.last_high_seas ) )
assert( french.last_high_seas == nil )
french.last_high_seas = { x=6, y=7 }
assert( french.last_high_seas == { x=6, y=7 } )
assert( french.last_high_seas.x == 6 )
assert( french.last_high_seas.y == 7 )
french.last_high_seas.y = 8
assert( french.last_high_seas.y == 7 ) -- !! because maybe<Coord> pops as a Coord which pops as a lua table.
print( 'french.last_high_seas: ' .. tostring( french.last_high_seas ) )
french.last_high_seas = nil
assert( french.last_high_seas == nil )

-----------------------------------------------------------------
-- LuaFormatter on
-----------------------------------------------------------------