-----------------------------------------------------------------
-- Telescope pickers.
-----------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local pickers = require'telescope.pickers'
local finders = require'telescope.finders'
local conf = require( 'telescope.config' ).values
local actions = require( 'telescope.actions' )
local action_state = require( 'telescope.actions.state' )

-----------------------------------------------------------------
-- Exports.
-----------------------------------------------------------------
local function get_input()
  -- If the user's input starts with a ':' then we just take
  -- whatever was typed as the result, as opposed to taking the
  -- selected entry which will be the closes match. This allows
  -- the user to force the input of something that is not in the
  -- result list (even if it partially matches something in the
  -- result), which is useful when e.g. creating new modules.
  local input = action_state.get_current_line()
  if input:sub( 1, 1 ) == ':' then return input:sub( 2, -1 ) end
  if action_state.get_selected_entry() then
    return action_state.get_selected_entry()[1]
  end
  return input
end

function M.pick_module( modules, on_selection )
  local opts = require( 'telescope.themes' ).get_dropdown{}
  return pickers.new( opts, {
    prompt_title='Modules',
    finder=finders.new_table{ results=modules },
    sorter=conf.generic_sorter( opts ),
    attach_mappings=function( _, _ )
      local function open_it( prompt_bufnr )
        actions.close( prompt_bufnr )
        on_selection( get_input() )
      end
      actions.select_default:replace( open_it )
      actions.select_vertical:replace( open_it )
      actions.select_horizontal:replace( open_it )
      actions.select_tab:replace( open_it )
      return true -- keep all default keyboard mappings.
    end,
  } ):find()
end

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return M
