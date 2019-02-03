" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
" Revolution|Now code editing startup script for vim.
" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

let s:first = 'main.cpp'

let s:stems = [
  \ 'src/globals',
  \ 'src/sdl-util',
  \ 'src/plane',
  \ 'src/menu',
  \ 'src/tiles',
  \ 'src/console',
  \ 'src/window',
  \ 'src/geo-types',
  \ 'src/nation',
  \ 'src/unit',
  \ 'src/ownership',
  \ 'src/orders',
  \ 'src/analysis',
  \ 'src/travel',
  \ 'src/job',
  \ 'src/combat',
  \ 'src/fight',
  \ 'src/dispatch',
  \ 'src/turn',
  \ 'src/render',
  \ 'src/loops',
\]

"let s:pairs = [
"  \ ['x/y/z.ext', 'a/c/d.xyz']
"\]

let s:quads = [
  \ ['doc/priorities.txt', 'scripts/session.vim',
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
" Move cursor from NERDTree into main pane.
call feedkeys( "\<C-L>" )
" Open a terminal in a vsplit.
"call feedkeys( ":vert term\<CR>\<C-W>h" )

" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
" Some user functions for later
" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
function s:OpenSourcePair( name )
  call OpenCppPair( 'src/' . a:name )
endfunction

command! -nargs=1 PairOpen call s:OpenSourcePair( <f-args> )
