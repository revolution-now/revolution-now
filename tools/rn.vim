" Revolution-Now-specific vim configuration.

" Sourcing other vim scripts in current folder.
" 1: Get the absolute path of this script.
" 2: Resolve all symbolic links.
" 3: Get the folder of the resolved absolute file.
let s:this_path = fnamemodify( resolve( expand( '<sfile>:p' ) ), ':h' )
" Get the folder in which this file resides.
let s:this_folder = fnamemodify( resolve( expand( '<sfile>:p' ) ), ':h' )

"exec 'source ' . s:this_path . '/another.vim'

" This is so that the Rcl/Rds syntax files get picked up.
let &runtimepath .= ',' . s:this_path . '/../src/rcl'
let &runtimepath .= ',' . s:this_path . '/../src/rds'

" This makes the design doc look nicer because it typically con-
" tains snippets of code.
au BufNewFile,BufRead *design.txt set syntax=cpp11
" jsav files are not yaml, but it seems to work nicely.
au BufNewFile,BufRead *.jsav set syntax=yaml
au BufNewFile,BufRead *.rds set filetype=rds
au BufNewFile,BufRead *.rcl set syntax=yaml

" Our version of Lua has a `continue` keyword which normal Lua
" does not.
syntax keyword luaStatement continue

" Tell the vim-templates function where to find the templates.
let g:tmpl_search_paths = [s:this_folder . '/templates']

exec ':source ' . s:this_folder . '/nvimrc.lua'
