-----------------------------------------------------------------
-- Default template tag handlers.
-----------------------------------------------------------------
-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local fnamemodify = vim.fn.fnamemodify

-----------------------------------------------------------------
-- Handlers.
-----------------------------------------------------------------
local handlers = {
  FILE_NAME=function( _, ctx )
    return fnamemodify( ctx.path, ':t' )
  end,
  FILE_STEM=function( _, ctx )
    return fnamemodify( ctx.path, ':t:r' )
  end,
  AUTHOR=function( _, ctx )
    if ctx.author == 'dsicilia' then return 'David P. Sicilia' end
    return ctx.author
  end,
  YEAR=function( _, _ ) return os.date( '%Y' ) end,
  MONTH=function( _, _ ) return os.date( '%m' ) end,
  DAY=function( _, _ ) return os.date( '%d' ) end,
  MODULE=function( _, ctx )
    local no_ext = fnamemodify( ctx.path, ':r' )
    if no_ext:match( '-test' ) then
      no_ext = no_ext:gsub( '-test', '' )
    end
    local no_src = no_ext:match( '^.*/src/(.*)' )
    local no_tst = no_ext:match( '^.*/test/(.*)' )
    if not no_src and not no_tst then return 'unknown' end
    return no_src or no_tst
  end,
  NAMESPACE=function( self, ctx )
    local module = self:MODULE( ctx )
    local first_folder = module:match( '^([^/]+)/' )
    local namespaces = {
      base='base',
      luapp='lua',
      render='rr',
      gl='gl',
      gfx='gfx',
      mock='mock',
      stb='stb',
      rcl='rcl',
      refl='refl',
      rds='rds',
      cdr='cdr',
      sav='sav',
    }
    return namespaces[first_folder] or 'rn'
  end,
}

-----------------------------------------------------------------
-- Finished.
-----------------------------------------------------------------
return function( tag, ctx )
  return handlers[tag] and handlers[tag]( handlers, ctx )
end
