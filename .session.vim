" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
" Revolution|Now code editing startup script for vim.
" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

let s:first = 'main.cpp'

let s:stems = [
  \ 'src/globals',
  \ 'src/base-util',
  \ 'src/sdl-util',
  \ 'src/fonts',
  \ 'src/world',
  \ 'src/viewport',
  \ 'src/unit',
  \ 'src/movement',
  \ 'src/cargo',
  \ 'src/ownership',
  \ 'src/turn',
  \ 'src/render',
  \ 'src/loops',
\]

let s:pairs = [
  \ ['src/global-constants.hpp', 'src/macros.hpp'],
\]

let s:quads = [
  \ ['notes/design.txt', 'notes/priorities.txt',
  \  '.session.vim',     'notes/ideas.txt']
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
    call feedkeys( ":split " . a:f3 . "\<CR>" )
    call feedkeys( "\<C-W>l" )
    call feedkeys( ":split " . a:f4 . "\<CR>" )
    call feedkeys( "\<C-W>k\<C-W>h" )
endfunction

function OpenCppPair( stem )
    call OpenPair( a:stem . '.hpp', a:stem . '.cpp' )
endfunction

" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
" Do it
" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

call OpenFirst( 'src/' . s:first )
for s in s:stems | call OpenCppPair( s )                   | endfor
for p in s:pairs | call OpenPair( p[0], p[1] )             | endfor
for q in s:quads | call OpenQuad( q[0], q[1], q[2], q[3] ) | endfor

" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
" Finish up.
" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

" Move back to first tab
call feedkeys( ']' )
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
