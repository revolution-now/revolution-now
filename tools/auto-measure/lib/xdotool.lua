-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local cmd = require'moon.cmd'
local logger = require'moon.logger'
local str = require'moon.str'

-----------------------------------------------------------------
-- Aliases
-----------------------------------------------------------------
local command = cmd.command
local info = logger.info
local trim = str.trim

-----------------------------------------------------------------
-- Constants.
-----------------------------------------------------------------
local KEY_DELAY = 500

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
local function press_keys( window, ... )
  assert( tonumber( window ) )
  assert( #{ ... } > 0 )
  xdotool( 'key', '--delay=' .. KEY_DELAY, '--window', window,
           ... )
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