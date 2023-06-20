-----------------------------------------------------------------
-- Lua snippets for revolution-now.
-----------------------------------------------------------------
local ls = require( 'luasnip' )
local s = ls.snippet
local i = ls.insert_node
local f = ls.function_node
local fmt = require( 'luasnip.extras.fmt' ).fmt

local up = 'unpack';
local unpack = _G[up] -- fool the linter.

local S = {}

local function add_s( tbl ) table.insert( S, s( unpack( tbl ) ) ) end

-----------------------------------------------------------------
-- Test case using testing::World.
-----------------------------------------------------------------
local function unit_test_module()
  -- If we are in ~/dev/revolution-now/test/x/y/z/a.cpp then
  -- test_dir_stem = ~/dev/revolution-now/test/x/y/z/a and we
  -- then extract just x/y/z/a.
  local test_dir_stem = assert( vim.fn.expand( '%:p' ) )
  return string.match( test_dir_stem,
                       'revolution.now/test/(.*)-test.cpp' )
end

add_s{
  '=wtest', fmt( [[
      TEST_CASE( "[{}] {}" ) {{
        World W;
        {}
      }}

    ]], {
    f( unit_test_module, {} ), i( 1, 'tested_func' ),
    i( 0, '// TODO' ),
  } ),
}

-----------------------------------------------------------------
--                         Finished
-----------------------------------------------------------------
return {}, S
