" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
" Revolution|Now code editing startup script for vim.
" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

let s:first = 'exe/main.cpp'

let s:stems = [
  \ 'src/tx',
  \ 'src/gfx',
  \ 'src/lua',
  \ 'src/console',
  \ 'src/line-editor',
  \ 'src/terminal',
  \ 'src/views',
  \ 'src/input',
  \ 'src/nation',
  \ 'src/player',
  \ 'src/screen',
  \ 'src/compositor',
  \ 'src/frame',
  \ 'src/plane',
  \ 'src/europort',
  \ 'src/europort-view',
\]

  "\ 'src/adt',
  "\ 'src/analysis',
  "\ 'src/auto-pad',
  "\ 'src/cache',
  "\ 'src/cargo',
  "\ 'src/color',
  "\ 'src/combat',
  "\ 'src/commodity',
  "\ 'src/compositor',
  "\ 'src/conductor',
  "\ 'src/config-files',
  "\ 'src/console',
  "\ 'src/coord',
  "\ 'src/dispatch',
  "\ 'src/enum',
  "\ 'src/errors',
  "\ 'src/europort',
  "\ 'src/europort-view',
  "\ 'src/fight',
  "\ 'src/font',
  "\ 'src/frame',
  "\ 'src/hash',
  "\ 'src/id',
  "\ 'src/image',
  "\ 'src/init',
  "\ 'src/input',
  "\ 'src/job',
  "\ 'src/logging',
  "\ 'src/math',
  "\ 'src/menu',
  "\ 'src/midiplayer',
  "\ 'src/midiseq',
  "\ 'src/mplayer',
  "\ 'src/mv-points',
  "\ 'src/nation',
  "\ 'src/oggplayer',
  "\ 'src/orders',
  "\ 'src/ownership',
  "\ 'src/physics',
  "\ 'src/plane',
  "\ 'src/rand',
  "\ 'src/render',
  "\ 'src/screen',
  "\ 'src/sdl-util',
  "\ 'src/sound',
  "\ 'src/terrain',
  "\ 'src/text',
  "\ 'src/tiles',
  "\ 'src/time',
  "\ 'src/travel',
  "\ 'src/ttf',
  "\ 'src/tune',
  "\ 'src/turn',
  "\ 'src/ui',
  "\ 'src/unit',
  "\ 'src/util',
  "\ 'src/utype',
  "\ 'src/viewport',
  "\ 'src/views',
  "\ 'src/window',

"let s:pairs = [
"  \ ['x/y/z.ext', 'a/c/d.xyz']
"\]

let s:luas = [
  \ 'startup',
  \ 'meta',
  \ 'util',
\]

let s:quads = [
  \ ['doc/todo.txt', 'scripts/session.vim',
  \  'doc/design.txt',     'doc/ideas.txt'],
\]

" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
" Functions
" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function OpenSidePanels()
    :NERDTreeToggle src
    execute ':wincmd h'
    :TagbarOpen
endfunction

function OpenFirst( name )
    execute ':e ' . a:name
    call OpenSidePanels()
endfunction

function OpenInTab( name )
    execute ':tabnew ' . a:name
    call OpenSidePanels()
endfunction

function OpenVSplit( name )
    execute ':vsplit ' . a:name
endfunction

function OpenPair( hpp, cpp )
    call OpenInTab( a:hpp )
    call OpenVSplit( a:cpp )
    execute ':wincmd h'
endfunction

function OpenLua( lua )
    call OpenInTab( 'src/lua/' . a:lua . '.lua' )
    "call OpenVSplit( 'src/lua/' . a:lua . '.lua' )
endfunction

function OpenQuad( f1, f2, f3, f4 )
    call OpenPair( a:f1, a:f2 )
    execute ':split ' . a:f3
    :wincmd l
    execute ':split ' . a:f4
    :wincmd k
    :wincmd h
endfunction

function OpenCppPair( stem )
    call OpenPair( a:stem . '.hpp', a:stem . '.cpp' )
endfunction

" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
" Do it
" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

call OpenFirst( s:first )
"for p in s:pairs  | call OpenPair( p[0], p[1] )             | endfor
for l in s:luas   | call OpenLua( l )                       | endfor
for s in s:stems  | call OpenCppPair( s )                   | endfor
for q in s:quads  | call OpenQuad( q[0], q[1], q[2], q[3] ) | endfor

" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
" Finish up.
" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

" Move back to first tab
:tabn 1
" Move cursor from main pane to TagBar, refresh it once (by tog-
" gling on/off the help screen) then move back to main pane. This
" is to work around a strange issue where the tagbar is empty in
" the first tab.
call feedkeys( "\<C-L>??\<C-H>" )
" Open a terminal in a vsplit.
"call feedkeys( ":vert term\<CR>\<C-W>h" )

" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
" Some user functions for later
" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
function s:OpenSourcePair( name )
  call OpenCppPair( 'src/' . a:name )
  " Move the red tab selector to align with this new tab which
  " will now be selected.
  :TabProposedNext
endfunction

command! -nargs=1 PairOpen call s:OpenSourcePair( <f-args> )
command! -nargs=1 LuaOpen  call OpenLua( <f-args> )
