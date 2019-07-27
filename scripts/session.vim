" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
" Revolution|Now code editing startup script for vim.
" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

let s:first = 'main.cpp'

let s:stems = [
  \ 'src/sdl-util',
  \ 'src/init',
  \ 'src/config-files',
  \ 'src/coord',
  \ 'src/screen',
  \ 'src/compositor',
  \ 'src/font',
  \ 'src/ttf',
  \ 'src/text',
  \ 'src/frame',
  \ 'src/plane',
  \ 'src/image',
  \ 'src/tiles',
  \ 'src/commodity',
  \ 'src/cargo',
  \ 'src/utype',
  \ 'src/unit',
  \ 'src/ownership',
  \ 'src/europort',
  \ 'src/europort-view',
  \ 'src/turn',
  \ 'src/render',
\]

  "\ 'src/adt',
  "\ 'src/analysis',
  "\ 'src/auto-pad',
  "\ 'src/cache',
  "\ 'src/cargo',
  "\ 'src/color',
  "\ 'src/combat',
  "\ 'src/conductor',
  "\ 'src/console',
  "\ 'src/dispatch',
  "\ 'src/enum',
  "\ 'src/errors',
  "\ 'src/fight',
  "\ 'src/fonts',
  "\ 'src/hash',
  "\ 'src/id',
  "\ 'src/input',
  "\ 'src/job',
  "\ 'src/logging',
  "\ 'src/main',
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
  "\ 'src/rand',
  "\ 'src/render',
  "\ 'src/sound',
  "\ 'src/terrain',
  "\ 'src/text',
  "\ 'src/time',
  "\ 'src/travel',
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

let s:quads = [
  \ ['doc/todo.txt', 'scripts/session.vim',
  \  'doc/design.txt',     'doc/ideas.txt'],
\]

" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
" Functions
" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function OpenSidePanels()
    :NERDTreeToggle src
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
    call OpenInTab( a:cpp )
    call OpenVSplit( a:hpp )
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

call OpenFirst( 'src/' . s:first )
for s in s:stems | call OpenCppPair( s )                   | endfor
"for p in s:pairs | call OpenPair( p[0], p[1] )             | endfor
for q in s:quads | call OpenQuad( q[0], q[1], q[2], q[3] ) | endfor

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
