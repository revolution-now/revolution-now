" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
" Revolution|Now code editing startup script for vim.
" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

let s:first = 'main.cpp'

let s:stems = [
  \ 'sdl-util',
  \ 'viewport',
  \ 'world',
  \ 'tiles',
  \ 'globals',
  \ 'base-util'
\]

let s:pairs = [
  \ ['macros.hpp', 'global-constants.hpp']
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
    execute ':tabnew src/' . a:name
    call OpenSidePanels()
endfunction

function OpenVSplit( name )
    execute ':vsplit src/' . a:name
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
