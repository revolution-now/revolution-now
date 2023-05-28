-----------------------------------------------------------------
-- C++ snippets for revolution-now.
-----------------------------------------------------------------
local assembler = require( 'dsicilia.snippets.assemble' )
local ls = require( 'luasnip' )
local S = {}

-----------------------------------------------------------------
-- Snippets.
-----------------------------------------------------------------
-- Test case using the testing::World class. TODO: make this more
-- sophisticated by putting in the correct path name of the file
-- relative to the src folder.
S.wtest =
[[TEST_CASE( "[$TM_FILENAME_BASE] $1" ) {
  World W;
  $0// TODO
}

]]

-----------------------------------------------------------------
-- Parse, and add snippets.
-----------------------------------------------------------------
local cpp_parsed = assembler.parse_snippets( S )

-- Map from filetype to snippets.
local parsed = {
  cpp = cpp_parsed,
}

ls.add_snippets( nil, parsed, { type='autosnippets' } )
