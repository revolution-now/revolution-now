" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
" Revolution|Now code editing startup script for vim.
" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

let s:first = 'exe/main.cpp'

let s:stems = [
  \ 'src/commodity',
  \ 'src/cargo',
  \ 'src/unit',
  \ 'src/ustate',
  \ 'src/fb',
  \ 'src/serial',
\]

  "\ 'src/adt.cpp',
  "\ 'src/analysis.cpp',
  "\ 'src/auto-pad.cpp',
  "\ 'src/cache.cpp',
  "\ 'src/cargo.cpp',
  "\ 'src/color.cpp',
  "\ 'src/combat.cpp',
  "\ 'src/commodity.cpp',
  "\ 'src/compositor.cpp',
  "\ 'src/conductor.cpp',
  "\ 'src/config-files.cpp',
  "\ 'src/console.cpp',
  "\ 'src/coord.cpp',
  "\ 'src/dispatch.cpp',
  "\ 'src/dragdrop.cpp',
  "\ 'src/enum.cpp',
  "\ 'src/errors.cpp',
  "\ 'src/europort.cpp',
  "\ 'src/europort-view.cpp',
  "\ 'src/fight.cpp',
  "\ 'src/font.cpp',
  "\ 'src/frame.cpp',
  "\ 'src/fsm.cpp',
  "\ 'src/gfx.cpp',
  "\ 'src/hash.cpp',
  "\ 'src/id.cpp',
  "\ 'src/image.cpp',
  "\ 'src/init.cpp',
  "\ 'src/input.cpp',
  "\ 'src/job.cpp',
  "\ 'src/line-editor.cpp',
  "\ 'src/linking.cpp',
  "\ 'src/logging.cpp',
  "\ 'src/lua.cpp',
  "\ 'src/math.cpp',
  "\ 'src/menu.cpp',
  "\ 'src/midiplayer.cpp',
  "\ 'src/midiseq.cpp',
  "\ 'src/mplayer.cpp',
  "\ 'src/mv-points.cpp',
  "\ 'src/nation.cpp',
  "\ 'src/oggplayer.cpp',
  "\ 'src/orders.cpp',
  "\ 'src/physics.cpp',
  "\ 'src/plane.cpp',
  "\ 'src/player.cpp',
  "\ 'src/power.cpp',
  "\ 'src/rand.cpp',
  "\ 'src/render.cpp',
  "\ 'src/scope-exit.cpp',
  "\ 'src/screen.cpp',
  "\ 'src/sdl-util.cpp',
  "\ 'src/serial.cpp',
  "\ 'src/sound.cpp',
  "\ 'src/terminal.cpp',
  "\ 'src/terrain.cpp',
  "\ 'src/text.cpp',
  "\ 'src/tiles.cpp',
  "\ 'src/time.cpp',
  "\ 'src/travel.cpp',
  "\ 'src/ttf.cpp',
  "\ 'src/tune.cpp',
  "\ 'src/turn.cpp',
  "\ 'src/tx.cpp',
  "\ 'src/ui.cpp',
  "\ 'src/unit.cpp',
  "\ 'src/ustate.cpp',
  "\ 'src/util.cpp',
  "\ 'src/utype.cpp',
  "\ 'src/viewport.cpp',
  "\ 'src/views.cpp',
  "\ 'src/window.cpp',

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
