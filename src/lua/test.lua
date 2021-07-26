--[[ ------------------------------------------------------------
|
| test.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2021-03-06.
|
| Description: Used for testing.
|
--]] ------------------------------------------------------------
M = {}

local waitable = require( 'waitable' )

local auto_await = waitable.auto_await
local await = waitable.await
local create_coroutine = waitable.create_coroutine

local function message_box_format( ... )
  local msg = string.format( ... )
  log.info( 'message box: ' .. msg )
  return lua_ui.message_box( msg )
end

local message_box = auto_await( message_box_format )
local ok_cancel = auto_await( lua_ui.ok_cancel )
local str_input_box = auto_await( lua_ui.str_input_box )
local wait_for_micros = auto_await( frame.wait_for_micros )

local function multiply( n )
  local m = n * 5
  message_box( 'The final result is %d*5 = %d.', n, m )
  return m
end

function timer_routine( seconds )
  local wait_micros = seconds * 1000 * 1000
  local n = 1
  while true do
    log.debug( string.format( 'waiting for %d seconds...',
                              wait_micros / 1000000 ) )
    local msg =
        string.format( 'waiting, iteration number %d.', n )
    local win<close> = lua_ui.message_box( msg )
    local actual = wait_for_micros( wait_micros )
    n = n + 1
    -- Don't await for `win` here because the idea of this loop
    -- is that we want to auto-close the window after sleeping.
  end
end
local timer_routine_coro = create_coroutine( timer_routine )

function M.some_ui_routine( n )
  log.info( 'start of some_ui_routine: ' .. tostring( n ) )

  do
    local timer<close> = timer_routine_coro( 5 )
    -- We want to catch any errors that happen in `timer`, but we
    -- can't await on it because it runs forever, so we'll just
    -- check at the exit points of this function with assertions.
    message_box( 'You will now be asked to enter a string.' )
    if ok_cancel( 'Would you like to proceed?' ) == 'cancel' then
      assert( not timer:error(), timer:error() )
      return
    end
    log.info( 'proceeding.' )
    assert( not timer:error(), timer:error() )
  end

  local n
  do
    -- Run message box concurrently. We don't want to use the
    -- wrapped version here because that will await it.
    local outter<close> = lua_ui.message_box( 'Outter Window' )
    local count = 1
    while true do
      -- Run message box concurrently. Again, no wrapped version.
      local _<close> = message_box_format(
                           'Inner Background: %d', count )
      n = str_input_box( 'User Input',
                         'Please enter an even number', '2' )
      if n == nil then
        message_box( 'Press enter to cancel.' )
        return
      end
      n = n + 0
      log.info( 'received ' .. tostring( n ) )
      if n % 2 == 0 then break end
      message_box( 'The number must be even!' )
      log.info( 'retrying...' )
      count = count + 1
    end
    await( outter )
  end
  if n == 42 then error( 'you cannot select 42.' ) end

  log.info( 'almost done...' )

  local s = str_input_box( '?', 'Ok, you selected ' .. n ..
                               '.  Enter a string', '' )
  if s == nil then
    message_box( 'Press enter to cancel.' )
    return
  end
  message_box( 'The string "%s" has length %d.', s, #s )
  local m = n + #s
  message_box( 'n (=%d) + #"%s" is %d.', n, s, m )

  return multiply( m )
end

return M