-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local xdotool = require'lib.xdotool'

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local find_window_named = xdotool.find_window_named
local action_api = xdotool.action_api

-----------------------------------------------------------------
-- DOSBox
-----------------------------------------------------------------
local __dosbox = nil

local function find_dosbox()
  local success, res = pcall( find_window_named,
                              'DOSBox.*VICEROY' )
  if success then return res end
end

local function window()
  if not __dosbox then
    __dosbox = find_dosbox()
    if not __dosbox then
      error( 'cannot find open DOSBox window.' )
    end
  end
  return __dosbox
end

local function pause() action_api( window() ).alt_pause() end

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return { find_dosbox=find_dosbox, window=window, pause=pause }
