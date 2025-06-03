-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local xdotool = require'lib.xdotool'
local logger = require'moon.logger'
local time = require'moon.time'
local dosbox = require'lib.dosbox'
local readwrite = require'lib.readwrite-sav'

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local info = logger.info
local sleep = time.sleep
local action_api = xdotool.action_api
local press_return_to_exit = xdotool.press_return_to_exit
local find_dosbox = dosbox.find_dosbox
local sav_file_for_slot = readwrite.sav_file_for_slot

local dosbox_actions = action_api( dosbox.window() )
local seq = dosbox_actions.seq
local up = dosbox_actions.up
local down = dosbox_actions.down
local enter = dosbox_actions.enter

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local function press_keys( ... )
  return xdotool.press_keys( dosbox.window(), ... )
end

-----------------------------------------------------------------
-- Game specific composite xdotool commands.
-----------------------------------------------------------------
local function game_menu() press_keys( 'alt+g' ) end

-- Loads the game from the given slot from the game menu.
local function load_game( n )
  assert( type( n ) == 'number' )
  info( 'loading %s...', sav_file_for_slot( n ) )
  press_keys( 'slash' ) -- requires configuration in MENU.TXT
  for _ = 0, n - 1 do down() end
  enter() -- Load from current slot.
  sleep( .5 ) -- Wait for game to load.
  enter() -- Close popup.
end

-- Saves the game to the given slot from the game menu.
local function save_game( n )
  assert( type( n ) == 'number' )
  info( 'saving %s...', sav_file_for_slot( n ) )
  press_keys( 'apostrophe' ) -- requires configuration in MENU.TXT
  for _ = 0, n - 1 do down() end
  enter() -- Save to current slot.
  sleep( 1 ) -- Wait for game to save.
  enter() -- Close popup.
end

local function exit_game()
  -- Do nothing if the DOSBox window is not open.
  if not find_dosbox() then return end
  info( 'exiting game.' )
  game_menu()
  seq{ up, enter } -- Select "Exit to DOS".
  down() -- Highlight 'Yes'.
  press_return_to_exit( dosbox.window() )
end

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return {
  game_menu=game_menu,
  load_game=load_game,
  save_game=save_game,
  exit_game=exit_game,
}
