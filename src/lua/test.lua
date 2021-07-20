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

local wrap = waitable.wrap

local function message_box_format( ... )
  return lua_ui.message_box( string.format( ... ) )
end

local message_box = wrap( message_box_format )
local ok_cancel = wrap( lua_ui.ok_cancel )
local str_input_box = wrap( lua_ui.str_input_box )

local function multiply( n )
  local m = n * 5
  message_box( 'The final result is %d*5 = %d.', n, m )
  return m
end

function M.some_ui_routine( n )
  log.info( 'start of some_ui_routine: ' .. tostring( n ) )

  message_box( 'You will now be asked to enter a string.' )
  if ok_cancel( 'Would you like to proceed?' ) == 'cancel' then
    return nil
  end
  log.info( 'proceeding.' )

  local n
  while true do
    n = str_input_box( 'User Input',
                       'Please enter an even number', '2' )
    n = n + 0
    log.info( 'received ' .. tostring( n ) )
    if n % 2 == 0 then break end
    message_box( 'The number must be even!' )
    log.info( 'retrying...' )
  end
  if n == 42 then error( 'you cannot select 42.' ) end

  log.info( 'almost done...' )

  local s = str_input_box( '?', 'Ok, you selected ' .. n ..
                               '.  Enter a string', '' )
  message_box( 'The string "%s" has length %d.', s, #s )
  local m = n + #s
  message_box( 'n (=%d) + #"%s" is %d.', n, s, m )

  return multiply( m )
end

return M