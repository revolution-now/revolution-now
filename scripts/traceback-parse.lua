--[[ ------------------------------------------------------------
|
| traceback-parse.lua
|
| Created by dsicilia on 2021-04-08.
|
| Description: Parses ASan tracebacks and makes them readable.
|
--]] ------------------------------------------------------------

-- Append the following to the LSAN_OPTIONS in the file
-- env-vars.mk in order to get more stack frames:
--
--   :fast_unwind_on_malloc=0:malloc_context_size=200
--

--[[-------------------------------------------------------------
|                         Things to Tweak
--]]-------------------------------------------------------------

remove_std_library = false

to_replace_in_line = {
  ['\t']                      = ' ',
  [' +']                      = ' ',
  ['<std::monostate>']        = '<>',
  ['<monostate>']             = '<>',
  ['__n4861::']               = '',
  ["'lambda'"]                = 'lambda',
  ['std::']                   = '',
  ['rn::']                    = '',
  ['base::']                  = '',
  ['detail::']                = '',
  ['.anonymous namespace.::'] = '',
  [' const']                  = '',
  [' >']                      = '>',
  [' [^ ]+/dsicilia/dev/revolution.now/.builds/current/../../'] = ' ',
  [' [^ ]+/dsicilia/dev/tools/gcc.current/include/'] = ' ',
}

to_replace_by_line = {
  ['freed by thread.*']       = 'Freed Here:',
  ['previously allocated']    = 'Previously Allocated Here:',
  ['double.free']             = 'Double Free:',
}

remove_lines_containing = {
  '======',
  'single_include',
  'compiler.rt',
  'is located.*bytes inside',
  'libc_start_main',
  'unique.func.class',
  'unique.func.hpp',
}

--[[-------------------------------------------------------------
|                         Implementation
--]]-------------------------------------------------------------
function append( t, e )
  t[#t] = e
end

if remove_std_library then
  append( remove_lines_containing, 'include/c++' )
end

for line in io.lines() do
  for _,v in pairs( remove_lines_containing ) do
    if line:find( v )  then
      goto loop_end
    end
  end
  for k, v in pairs( to_replace_by_line ) do
    if line:match( k ) then
      print( v )
      goto loop_end
    end
  end
  for k, v in pairs( to_replace_in_line ) do
    line = line:gsub( k, v )
  end
  words = {}
  for w in line:gmatch("%S+") do words[#words+1] = w end
  if #words == 0 then
    print( '' )
    goto loop_end
  end
  file = words[#words]
  file, lineno = file:match( '([^:]+):(%d+):.*' )
  if file == nil then goto loop_end end
  lineno = lineno or '?'
  words[#words] = nil
  line = table.concat( words, ' ' )
  num, line = line:match( ' *#(%d+) 0x[0-9a-f]+ in (.*)' )
  line = line or ''
  for k, v in pairs( to_replace_in_line ) do
    line = line:gsub( k, v )
  end
  print( string.format( '%4s  %-35s %5s  %s', num, file, lineno, line ) )
::loop_end::
end