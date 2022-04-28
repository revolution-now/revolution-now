--[[ ------------------------------------------------------------
|
| printer.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2022-02-19.
|
| Description: Pretty-prints Rds Lua data structures.
|
--]] ------------------------------------------------------------
local print_sumtype = function( name, tbl )
  print( 'sumtype.' .. name .. ' {' )
  for _, alt in ipairs( tbl ) do
    print( '  ' .. alt.name .. ' {' )
    for _, var in ipairs( alt.obj ) do
      if type( var ) == 'function' then
        -- This is a feature or template keyword.
        print( '    ' .. var().name .. ',' )
      else
        -- Normal alternative.
        print( '    ' .. var.name .. ' \'' .. var.obj .. '\',' )
      end
    end
    print( '  },' )
  end
  print( '}' )
end

local print_enum = function( name, tbl )
  print( 'enum.' .. name .. ' {' )
  for _, enum_val in ipairs( tbl ) do
    print( '  ' .. enum_val().name .. ',' )
  end
  print( '}' )
end

local print_struct = function( name, tbl )
  print( 'struct.' .. name .. ' {' )
  for _, var_tbl in ipairs( tbl ) do
    print( '  ' .. var_tbl.name .. ' \'' .. var_tbl.obj .. '\',' )
  end
  print( '}' )
end

local print_config = function( name, tbl )
  print( 'config.' .. name .. ' {}' )
end

local print_namespace = function( name )
  print( 'namespace \'' .. name .. '\'' )
end

local print_include = function( name )
  print( 'include "' .. name .. '"' )
end

local printers = {
  sumtype=print_sumtype,
  enum=print_enum,
  struct=print_struct,
  config=print_config
}

function print_rds()
  for _, item in ipairs( rds.includes ) do print_include( item ) end
  local ns
  for _, item in ipairs( rds.items ) do
    if ns ~= item.ns then
      print()
      print_namespace( item.ns )
      ns = item.ns
    end
    print()
    printers[item.type]( item.name, item.obj )
  end
end

return M
