" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
" Revolution|Now code editing startup script for vim.
" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

let s:first = 'main.cpp'

let s:stems = [
  \ 'src/sdl-util',
  \ 'src/unit',
  \ 'src/viewport',
  \ 'src/world',
  \ 'src/tiles',
  \ 'src/globals',
  \ 'src/base-util'
\]

let s:pairs = [
  \ ['src/macros.hpp', 'src/global-constants.hpp'],
  \ ['src/aliases.hpp', 'src/typed-int.hpp'],
  \ ['notes/priorities.txt', 'notes/ideas.txt']
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

function OpenCppPair( stem )
    call OpenPair( a:stem . '.hpp', a:stem . '.cpp' )
endfunction

" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
" Do it
" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

call OpenFirst( 'src/' . s:first )
for s in s:stems | call OpenCppPair( s )       | endfor
for p in s:pairs | call OpenPair( p[0], p[1] ) | endfor

" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
" Finish up.
" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

" Move back to first tab
call feedkeys( ']' )
" Move cursor from NERDTree into main pane.
call feedkeys( "\<C-L>" )
