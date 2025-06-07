-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local moontbl = require'moon.tbl'

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local deep_copy = moontbl.deep_copy
local on_ordered_kv = moontbl.on_ordered_kv
local insert = table.insert

-----------------------------------------------------------------
-- Config Utilities.
-----------------------------------------------------------------
local function flatten_configs( master )
  local res = { {} }
  on_ordered_kv( master, function( k, v )
    if type( v ) ~= 'table' then v = { v } end
    assert( #v > 0, 'empty key ' .. k ..
                ' will cause empty flattened configs.' )
    local new_res = {}
    for _, elem in ipairs( v ) do
      local copy = deep_copy( res )
      for _, config in ipairs( copy ) do
        config[k] = elem
        insert( new_res, config )
      end
    end
    res = new_res
  end )
  return res
end

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return { flatten_configs=flatten_configs }
