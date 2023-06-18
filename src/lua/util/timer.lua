--[[ ------------------------------------------------------------
|
| timer.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2022-06-19.
|
| Description: Functions related to time.
|
--]] ------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Globals.
-----------------------------------------------------------------
local function global( name ) return assert( _G[name] ) end

local time = global( 'time' )

-----------------------------------------------------------------
-- Methods.
-----------------------------------------------------------------
local function now() return time.current_epoch_time_micros() end

local function format_duration( d )
  local us = d
  if us < 1000 then return tostring( us ) .. 'us' end
  local ms = us // 1000
  us = us % 1000
  if ms < 1000 then
    local res = tostring( ms ) .. 'ms'
    if ms < 10 then res = res .. ' ' .. tostring( us ) .. 'us' end
    return res
  end
  local s = ms // 1000
  ms = ms % 1000
  if s < 60 then
    local res = tostring( s ) .. 's'
    if s < 10 then res = res .. ' ' .. tostring( ms ) .. 'ms' end
    return res
  end
  local mins = s // 60
  s = s % 60
  if mins < 60 then
    local res = tostring( mins ) .. 'mins'
    if mins < 10 then res = res .. ' ' .. tostring( s ) .. 's' end
    return res
  end
  local hours = mins // 60
  mins = mins % 60
  if hours < 60 then
    local res = tostring( hours ) .. 'hrs'
    if hours < 10 then
      res = res .. ' ' .. tostring( mins ) .. 'mins'
    end
    return res
  end
  local days = hours // 24
  hours = hours % 24
  local res = tostring( days ) .. 'days'
  if days < 10 then
    res = res .. ' ' .. tostring( hours ) .. 'hours'
  end
  return res
end

-- Runs the function and returns the duration as the first re-
-- sult, followed by any results of the function.
function M.run_timed( f, ... )
  local start_time = now()
  local results = table.pack( f( ... ) )
  local end_time = now()
  local delta = end_time - start_time
  return delta, table.unpack( results, 1, results.n )
end

-- Runs the function and logs the duration, then returns anything
-- returned by the function.
function M.log_time( name, f, ... )
  local res = table.pack( M.run_timed( f, ... ) )
  local formatted = format_duration( res[1] )
  log.debug( name .. ' took ' .. formatted .. '.' )
  table.remove( res, 1 )
  return table.unpack( res, 1, res.n - 1 )
end

-- Returns a function that, when run, will run f and log the
-- time.
function M.timed( name, f )
  return function( ... ) return M.log_time( name, f, ... ) end
end

return M
