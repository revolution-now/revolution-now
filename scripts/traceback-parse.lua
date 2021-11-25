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

show_std_library = false

rn    = '/home/dsicilia/dev/revolution.now'
tools = '/home/dsicilia/dev/tools'

to_replace_in_line = {
  ['\t'                                      ] = ' ',
  [' +'                                      ] = ' ',
  ['<std::monostate>'                        ] = '<>',
  ['<monostate>'                             ] = '<>',
  ['__n4861::'                               ] = '',
  ["'lambda(%d+)'"                           ] = 'lambda-%1',
  ["'lambda'"                                ] = 'lambda',
  ['std::'                                   ] = '',
  ['rn::'                                    ] = '',
  ['base::'                                  ] = '',
  ['detail::'                                ] = '',
  ['.anonymous namespace.::'                 ] = '',
  [' const'                                  ] = '',
  [' >'                                      ] = '>',
  ['basic_string'                            ] = 'string',
  ['__cxx11::'                               ] = '',
  ['string<>'                                ] = 'string',
  ['.abi:cxx11.'                             ] = '',
  [rn    .. '/.builds/current/../../'        ] = ' ',
  [tools .. '/gcc.current/include/'          ] = ' ',
  ['____C_A_T_C_H____T_E_S_T____%d+%(%)'     ] = '(unit test)',
  ['char, char_traits<char>, allocator<char>'] = '',
}

to_replace_by_line = {
  ['freed by thread.*'            ] = "Free'd Here:",
  ['previously allocated.*'       ] = 'Previously Allocated Here:',
  ['double.free'                  ] = 'Double Free:',
  ['WRITE of size (%d+).*'        ] = 'Write of Size %1:',
  ['Indirect leak of (%d+) byte.*'] = 'Leak of %1 bytes:',
  ['.*SEGV on unknown.*'          ] = 'Segmentation Fault:',
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

keep_lines_identical_containing = {
  '^$',
  'heap.use.after.free',
}

--[[-------------------------------------------------------------
|                         Implementation
--]]-------------------------------------------------------------
function append( t, e ) t[#t] = e end

function printf( ... ) print( string.format( ... ) ) end

if not show_std_library then
  append( remove_lines_containing, 'include/c++' )
end

function replace_inside_lines( line )
  for k, v in pairs( to_replace_in_line ) do
    line = line:gsub( k, v )
  end
  return line
end

function split( s )
  local words = {}
  for w in s:gmatch("%S+") do words[#words+1] = w end
  return words
end

function iterate( f, arg )
  repeat
    local prev = arg
    arg = f( arg )
  until arg == prev
  return arg
end

function freeze_table( tbl )
  setmetatable( tbl, {
    __newindex = function( _, k, v )
      error( 'attempt to add key "' .. tostring( k ) .. '" to frozen table.', 2 )
    end
  } )
end

-- Prevent creation of new globals.
freeze_table( _G )

for line in io.lines() do
  for _,v in pairs( keep_lines_identical_containing ) do
    if line:find( v )  then
      print( line )
      goto loop_end
    end
  end

  for _,v in pairs( remove_lines_containing ) do
    if line:find( v )  then
      goto loop_end
    end
  end

  for k, v in pairs( to_replace_by_line ) do
    if line:match( k ) then
      line = line:gsub( k, v )
      print( line )
      goto loop_end
    end
  end

  -- Repeat application until it converges since some matches can
  -- only apply after others have been applied.
  line = iterate( replace_inside_lines, line )

  local words = split( line )
  local file = words[#words]
  local lineno
  file, lineno = file:match( '([^:]+):(%d+):.*' )
  if file == nil then goto loop_end end
  lineno = lineno or '?'
  words[#words] = nil
  line = table.concat( words, ' ' )
  local num
  num, line = line:match( ' *(#%d+) 0x[0-9a-f]+ in (.*)' )
  line = line or ''
  printf( '  %-4s  %-35s %5s  %s', num, file, lineno, line )
::loop_end::
end