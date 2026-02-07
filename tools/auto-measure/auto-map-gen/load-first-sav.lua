-----------------------------------------------------------------
-- Package path.
-----------------------------------------------------------------
package.path = package.path ..
                   ';/home/dsicilia/dev/revolution-now/tools/auto-measure/?.lua'

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local xdotool = require( 'lib.xdotool' )
local dosbox = require( 'lib.dosbox' )
local logger = require( 'moon.logger' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local action_api = xdotool.action_api

-----------------------------------------------------------------
-- Script.
-----------------------------------------------------------------
logger.level = logger.levels.WARNING

local actions = action_api( dosbox.window() )

actions.press_keys( 'slash' ) -- Load game.
actions.enter() -- Select first.
actions.enter() -- Close popup box.