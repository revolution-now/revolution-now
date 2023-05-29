-----------------------------------------------------------------
-- Snippets for revolution-now.
-----------------------------------------------------------------
local SNIPPETS_FOLDER = '~/dev/revolution-now/tools/ide/snippets'

-- Lua snippets.
require( "luasnip.loaders.from_lua" ).lazy_load {
  paths = { SNIPPETS_FOLDER .. '/lua' }
}

-- Snippets from snipmate files.
require( "luasnip.loaders.from_snipmate" ).lazy_load {
  paths = { SNIPPETS_FOLDER .. '/snipmate' }
}
