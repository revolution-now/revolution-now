-----------------------------------------------------------------
-- NAMES.TXT parser.
-----------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local list = require'moon.list'
local file = require'moon.file'
local logger = require'moon.logger'

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local split_trim = list.split_trim
local read_file_lines = file.read_file_lines
local insert = table.insert
local trace = logger.trace
local info = logger.info

-----------------------------------------------------------------
-- Raw parsing (no semantics).
-----------------------------------------------------------------
local function parse_row( line ) return split_trim( line, ',' ) end

local function remove_comment( line )
  return split_trim( line, ';' )[1]
end

local function try_parse_header( line )
  if line:sub( 1, 1 ) == '@' then return line:sub( 2 ) end
end

local function parse_raw( path )
  local lines = read_file_lines( path )
  local names = {}
  local section
  for _, line in ipairs( lines ) do
    line = remove_comment( line )
    if not line or #line == 0 then goto continue end
    local header = try_parse_header( line )
    if header then
      section = header
      trace( 'new section: %s', section )
      goto continue
    end
    trace( 'parsing line: %s', line )
    assert( section )
    names[section] = names[section] or {}
    insert( names[section], parse_row( line ) )
    ::continue::
  end
  return names
end

-----------------------------------------------------------------
-- Semantic enrichment.
-----------------------------------------------------------------
local function enrich_unit_section( names )
  local new = {}
  for _, unit in ipairs( names.UNIT ) do
    local unit_name = assert( unit[1] )
    local new_unit = {
      icon=assert( tonumber( unit[2] ) ), --
      movement=assert( tonumber( unit[3] ) ), --
      attack=assert( tonumber( unit[4] ) ), --
      combat=assert( tonumber( unit[5] ) ), --
      cargo=assert( tonumber( unit[6] ) ), --
      size=assert( tonumber( unit[7] ) ), --
      cost=assert( tonumber( unit[8] ) ), --
      tools=assert( tonumber( unit[9] ) ), --
      guns=assert( tonumber( unit[10] ) ), --
      hull=assert( tonumber( unit[11] ) ), --
      role=assert( unit[12] ), --
    }
    new[unit_name] = new_unit
  end
  names.UNIT = new
end

-----------------------------------------------------------------
-- Main parser entrypoint.
-----------------------------------------------------------------
function M.parse( path )
  info( 'parsing NAMES.TXT at %s...', path )
  local parsed = parse_raw( path )

  enrich_unit_section( parsed )
  -- TODO: add more here as needed.

  return parsed
end

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return M
