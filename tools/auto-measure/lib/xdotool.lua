-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local cmd = require'moon.cmd'
local logger = require'moon.logger'
local str = require'moon.str'
local time = require'moon.time'

-----------------------------------------------------------------
-- Aliases
-----------------------------------------------------------------
local command = cmd.command
local info = logger.info
local trim = str.trim
local sleep = time.sleep
local insert = table.insert
local unpack = table.unpack

-----------------------------------------------------------------
-- Constants.
-----------------------------------------------------------------
local KEY_DELAY = 50

-----------------------------------------------------------------
-- xdotool
-----------------------------------------------------------------
local function xdotool( ... )
  return trim( command( 'xdotool', ... ) )
end

local function find_window_named( regex )
  info( 'searching for window with name ' .. regex )
  return xdotool( 'search', '--name', regex )
end

-----------------------------------------------------------------
-- General X commands.
-----------------------------------------------------------------
local function press_alt_and_key( window, key )
  assert( tonumber( window ) )
  local args = {}

  insert( args, 'keydown' )
  insert( args, '--window=' .. window )
  insert( args, 'alt' )

  insert( args, 'key' )
  insert( args, '--delay=' .. KEY_DELAY )
  insert( args, '--window=' .. window )
  insert( args, key )

  insert( args, 'keyup' )
  insert( args, '--window=' .. window )
  insert( args, 'alt' )

  xdotool( unpack( args ) )
  -- Need this additional delay because often we call press_keys
  -- multiple times to press multiple keys.
  sleep( KEY_DELAY / 1000 )
end

local function press_keys( window, ... )
  assert( tonumber( window ) )
  assert( #{ ... } > 0 )
  local args = {}
  for _, key in ipairs{ ... } do
    insert( args, 'key' )
    insert( args, '--delay=' .. KEY_DELAY )
    insert( args, '--window=' .. window )
    insert( args, key )
  end
  xdotool( unpack( args ) )
  -- Need this additional delay because often we call press_keys
  -- multiple times to press multiple keys.
  sleep( KEY_DELAY / 1000 )
end

local function press_return_to_exit( window )
  assert( tonumber( window ) )
  -- Somehow, even though this succeeds in ending the program,
  -- the xdotool returns an error.
  pcall( xdotool, 'key', '--window', window, 'Return', '2>&1' )
end

-----------------------------------------------------------------
-- Individual keys.
-----------------------------------------------------------------
local function action_api( window )
  return {
    press_keys=function( ... ) press_keys( window, ... ) end,
    down=function() press_keys( window, 'Down' ) end,
    up=function() press_keys( window, 'Up' ) end,
    left=function() press_keys( window, 'Left' ) end,
    right=function() press_keys( window, 'Right' ) end,
    enter=function() press_keys( window, 'Return' ) end,
    space=function() press_keys( window, 'space' ) end,
    alt_pause=function() press_alt_and_key( window, 'Pause' ) end,
    seq=function( tbl )
      for _, action in ipairs( tbl ) do action() end
    end,
  }
end

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return {
  xdotool=xdotool,
  find_window_named=find_window_named,
  press_keys=press_keys,
  press_return_to_exit=press_return_to_exit,
  action_api=action_api,
}