-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local moontbl = require'moon.tbl'

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local deep_copy = moontbl.deep_copy
local insert = table.insert

-----------------------------------------------------------------
-- Config Utilities.
-----------------------------------------------------------------
local function flatten_configs( master )
  local res = { {} }
  for k, v in pairs( master ) do
    if type( v ) ~= 'table' then v = { v } end
    local new_res = {}
    for _, elem in ipairs( v ) do
      local copy = deep_copy( res )
      for _, config in ipairs( copy ) do
        config[k] = elem
        insert( new_res, config )
      end
    end
    res = new_res
  end
  return res
end

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return { flatten_configs=flatten_configs }
