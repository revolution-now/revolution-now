" =========================== Config ============================
let s:stems = [
 \ 'ss/colony',
 \ 'config/colony',
 \ 'colony-mgr',
 \ 'colony-evolve',
 \ 'colony-buildings',
 \ 'colview-buildings',
 \ 'teaching',
\]

let s:luas = [
  \ 'startup',
  \ 'meta',
  \ 'util',
\]

let s:quads = [
  \ ['doc/todo.txt', 'scripts/session.vim',
  \  'doc/design.txt',     'doc/ideas.txt'],
\]

" ========================= Functions ===========================

function s:Open3( stem )
  echo '  - ' . a:stem
  let l:rds_impl_opened = 0
  if filereadable( 'src/' . a:stem . '.rds' )
    exe 'silent tabnew src/' . a:stem . '.rds'
    exe 'silent vsplit src/' . a:stem . '.hpp'
  elseif filereadable( 'src/' . a:stem . '-impl.rds' )
    let l:rds_impl_opened = 1
    exe 'silent tabnew src/' . a:stem . '-impl.rds'
    exe 'silent vsplit src/' . a:stem . '.hpp'
  else
    exe 'silent tabnew src/' . a:stem . '.hpp'
  endif
  exe 'silent vsplit src/' . a:stem . '.cpp'
  if a:stem =~ '^config/'
    exe 'silent vsplit config/rcl/' . fnamemodify( a:stem, ':t:r' ) . '.rcl'
  elseif filereadable( 'test/' . a:stem . '.cpp' )
    exe 'silent vsplit test/' . a:stem . '.cpp'
  else
    " Turn off auto template initialization, create the file,
    " then initailize it with the unit test template. If we don't
    " do it this way then the file will be created and auto
    " initialized with the regular cpp template.
    let l:tmp = g:tmpl_auto_initialize
    let g:tmpl_auto_initialize = 0
    exe 'silent vnew test/' . a:stem . '.cpp'
    let g:tmpl_auto_initialize = l:tmp
    :TemplateInit cpptest
    set nomodified
  endif
  4wincmd h
  if l:rds_impl_opened == 0
    if filereadable( 'src/' . a:stem . '-impl.rds' )
      exe 'silent split src/' . a:stem . '-impl.rds'
      wincmd k
    endif
  endif
endfunction

function s:OpenShaderPair( stem )
  echo '  - ' . a:stem
  exe 'silent tabnew src/' . a:stem . '.vert'
  exe 'silent vsplit src/' . a:stem . '.frag'
  wincmd h
endfunction

function! s:OpenModule( name )
  if filereadable( 'src/' . a:name . '.vert' )
    call s:OpenShaderPair( a:name )
    return
  endif
  call s:Open3( a:name )
endfunction

function! OpenModuleWithInput()
  let curline = getline('.')
  call inputsave()
  let name = input('Enter name: ')
  call inputrestore()
  call s:OpenModule( name )
endfunction
nnoremap <C-p> :call OpenModuleWithInput()<CR>

function s:OpenQuad( upper_left, upper_right, lower_right, lower_left )
  exe 'silent tabnew ' . a:upper_left
  exe 'silent vsplit ' . a:upper_right
  exe 'silent split '  . a:lower_right
  wincmd h
  exe 'silent split '  . a:lower_left
  wincmd k
endfunction

function s:OpenVertList( list )
  if !empty( a:list )
    exe 'silent tabnew ' . a:list[0]
    for i in range( 1, len( a:list )-1 )
      exe 'silent vsplit ' . a:list[i]
    endfor
    for i in range( 1, len( a:list )-1 )
      wincmd h
    endfor
  endif
endfunction

function s:OpenLuas( luas )
  let l:luas2 = []
  for lua in a:luas
    call add( l:luas2, 'src/lua/' . lua . '.lua' )
  endfor
  call s:OpenVertList( l:luas2 )
endfunction

" ============================ Main =============================
" This needs to be the number of message output (echo) otherwise
" you will get 'Press ENTER to continue...'.
let s:lines = 4 + len( s:stems )
exe 'set cmdheight=' . s:lines

" For convenience.
command! -nargs=1 Open3 call s:Open3( <f-args> )
command! -nargs=1 PairOpen call s:Open3( <f-args> )

echo 'opening main...'
silent edit exe/main.cpp

echo 'opening luas...'
" call s:OpenLuas( s:luas )

echo 'opening docs...'
for q in s:quads
  call s:OpenQuad( q[0], q[1], q[3], q[2] )
endfor

echo 'opening cpps...'
for s in s:stems
  call s:OpenModule( s )
endfor

tabdo set cmdheight=1
tabdo wincmd =

" This goes to the first tab and closes it.  If you want the
" `main` tab to appear, then comment out the :q line.
tabn 1
" :q
tabprev

if has( 'nvim' )
  tabdo set cmdheight=0
end