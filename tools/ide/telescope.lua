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
function M.pick_module( modules, on_selection )
  local opts = require( 'telescope.themes' ).get_dropdown{}
  return pickers.new( opts, {
    prompt_title='Modules',
    finder=finders.new_table{ results=modules },
    sorter=conf.generic_sorter( opts ),
    attach_mappings=function( _, _ )
      local function open_it( prompt_bufnr )
        actions.close( prompt_bufnr )
        local module = action_state.get_selected_entry()[1]
        on_selection( module )
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
