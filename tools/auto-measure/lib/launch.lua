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
local format = string.format

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
  -- This is very subtle here... we need to run this process in
  -- the background, but we cannot simply use io.popen because
  -- that will return a file handle which, if we drop it, will
  -- later get garbage collected at a random time and cause the
  -- program to hang because finalizing that handle entails
  -- closing the pipes that popen opens, which are held open by
  -- the child process (DOSBox) which will not be closing (until
  -- we close it manually), thus hanging the program. There are
  -- ways around that such as to store the returned handle in a
  -- global or preventing popen from opening pipes, but the fol-
  -- lowing is probably the simplest, namely just using os.exe-
  -- cute and detaching the process. NOTE: we need to redirect
  -- stdout/stdin here to /dev/null otherwise it will write the
  -- output to a nohup log file in the current directory.
  assert( os.execute( format( 'nohup %q 1>/dev/null 1>&2 &',
                              START_SH ) ) )
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
