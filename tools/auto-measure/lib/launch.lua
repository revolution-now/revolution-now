-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local cmd = require'moon.cmd'
local logger = require'moon.logger'
local dosbox = require'lib.dosbox'

-----------------------------------------------------------------
-- Aliases
-----------------------------------------------------------------
local command = cmd.command
local info = logger.info
local forget_dosbox_window = dosbox.forget_dosbox_window

-----------------------------------------------------------------
-- Constants.
-----------------------------------------------------------------
local START_SH = '/home/dsicilia/games/colonization/start.sh'
local DOSBOX_BINARY_NAME = 'dosbox_x86_64'

-----------------------------------------------------------------
-- xdotool
-----------------------------------------------------------------
local function is_dosbox_running()
  return pcall( command, 'pgrep', DOSBOX_BINARY_NAME )
end

local function launch_start()
  assert( not is_dosbox_running() )
  info( 'starting DOSBox...' )
  return assert( io.popen( START_SH ) )
end

local function kill_dosbox()
  assert( is_dosbox_running() )
  info( 'killing DOSBox...' )
  command( 'pkill', DOSBOX_BINARY_NAME )
  forget_dosbox_window()
end

local launcher_mt = {
  __close=function()
    if is_dosbox_running() then
      info( 'killing %s...', DOSBOX_BINARY_NAME )
      kill_dosbox()
    end
  end,
  __metatable=false,
}

local function colonization_launcher()
  launch_start()
  local o = {}
  setmetatable( o, launcher_mt )
  return o
end

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return {
  is_dosbox_running=is_dosbox_running, --
  launch_start=launch_start, --
  kill_dosbox=kill_dosbox, --
  colonization_launcher=colonization_launcher, --
}
