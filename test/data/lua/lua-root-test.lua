local ENABLE_PRINT = false

local function print( ... )
  if ENABLE_PRINT then _ENV.print( ... ) end
end

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
root.land_view.map_revealed.player = {}
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
root.turn.cycle.player = {}
assert( root.turn.cycle.player ~= nil )
print( 'root.turn.cycle.player: ' .. tostring( root.turn.cycle.player ) )
print( 'root.turn.cycle.player.st.units: ' .. tostring( root.turn.cycle.player.st.units ) )
assert( root.turn.cycle.player.st.units == nil )
root.turn.cycle.player.st.units = {}
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
print( 'root.players.players.french.has_value(): ' .. tostring( root.players.players.french:has_value() ) )
assert( root.players.players.french:has_value() )
assert( root.players.players.french.type == 'french' )
assert( root.players.players.french.nation == 'french' )
assert( not pcall( function()
  root.players.players.spanish.nation = 'spanish'
end ) )
assert( not pcall( function()
  local _ = root.players.players.spanish.nation == 'spanish'
end ) )

-----------------------------------------------------------------
-- LuaFormatter on
-----------------------------------------------------------------