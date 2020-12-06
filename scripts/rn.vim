" Revolution-Now-specific vim configuration.
set sw=2
set shiftwidth=2
set tabstop=2

" Sourcing other vim scripts in currnet folder.
" 1: Get the absolute path of this script.
" 2: Resolve all symbolic links.
" 3: Get the folder of the resolved absolute file.
let s:this_path = fnamemodify( resolve( expand( '<sfile>:p' ) ), ':h' )
" Get the folder in which this file resides.
let s:this_folder = fnamemodify( resolve( expand( '<sfile>:p' ) ), ':h' )

"exec 'source ' . s:this_path . '/another.vim'

" This is so that the RNL syntax file gets picked up.
let &runtimepath .= ',' . s:this_path . '/../src/rnl'

" This makes the design doc look nicer because it typically con-
" tains snippets of code.
au BufNewFile,BufRead *design.txt set syntax=cpp11
" jsav files are not yaml, but it seems to work nicely.
au BufNewFile,BufRead *.jsav set syntax=yaml
au BufNewFile,BufRead *.rnl set filetype=rnl

"function! CloseTerminal()
"		let s:term_buf_name = bufname( "*bin/fish*" )
"    if s:term_buf_name != ""
"        let bnr = bufwinnr( s:term_buf_name )
"        if bnr > 0
"            :exe bnr . "wincmd w"
"            call feedkeys( "\<C-D>" )
"        endif
"    endif
"endfunction

"function CloseTerminal()
"  call feedkeys( ":tabn 1\<CR>\<C-W>\<C-T>\<C-W>l\<C-W>l\<C-D>\<C-W>\<C-T>" )
"  call feedkeys( ":qa\<CR>" )
"endfunction

"nnoremap Q :call CloseTerminal()<CR>

function! MyTabLine()
  let s = ''
  for i in range(tabpagenr('$'))
    " select the highlighting
    if i + 1 == tabpagenr()
      let s .= '%#TabLineSel#'
    else
      let s .= '%#TabLine#'
    endif

    " set the tab page number (for mouse clicks)
    let s .= '%' . (i + 1) . 'T'

    " the label is made by MyTabLabel()
    let s .= ' %{MyTabLabel(' . (i + 1) . ')} '
  endfor

  " after the last tab fill with TabLineFill and reset tab page nr
  let s .= '%#TabLineFill#%T'

  " right-align the label to close the current tab page
  if tabpagenr('$') > 1
    let s .= '%=%#TabLine#%999XX'
  endif

  return s
endfunction

function! MyTabLabel( n )
  let buflist = tabpagebuflist( a:n )
  let winnr = tabpagewinnr( a:n, '$' )
  for i in range( winnr )
    let path = bufname( buflist[i] )
    let ext = fnamemodify( path, ':e' )
    if ext == 'cpp' || ext == 'hpp'
      return fnamemodify( path, ':t:r' )
    endif
    if ext == 'ucl'
      return fnamemodify( 'ucl', ':t:r' )
    endif
    if ext == 'txt'
      return fnamemodify( 'docs', ':t:r' )
    endif
    if ext == 'lua'
      let path = fnamemodify( path, ':t:r' )
      return 'lua/' . path
    endif
  endfor
  let winnr = tabpagewinnr( a:n )
  return bufname( buflist[winnr - 1] )
endfunction

set tabline=%!MyTabLine()

" Only call clang format if we are under the src or test folders.
function! MaybeClangFormat()
  " Full path of file trying to be formatted.
  let file_path = resolve( expand( '%:p' ) )
  " Take the full resolved path of the folder containing this
  " vimscript file, then go up one level to get the root folder
  " of the RN source tree.
  let rn_root = fnamemodify( s:this_path, ':h' )
  let allowed_folders = [
    \ 'src',
    \ 'exe',
    \ 'test',
    \ ]
  for folder in allowed_folders
    if file_path =~ '^' . rn_root . '/'. folder
      call ClangFormatAll()
      return
    endif
  endfor
endfunction

" Automatically format the C++ source files just before saving.
autocmd BufWritePre *.hpp,*.cpp call MaybeClangFormat()

" We set this ycm global variable to point YCM to the conf script.  The
" reason we don't just put a .ycm_extra_conf.py in the root folder
" (which YCM would then find on its own without our help) is that we
" want to keep the folder structure organized with all scripts in the
" scripts folder.
let g:ycm_global_ycm_extra_conf = s:this_folder . '/ycm_extra_conf.py'

" Tell the vim-templates function where to find the templates.
let g:tmpl_search_paths = [s:this_folder . '/templates']

" function! BuildO()
"   " :!~/dev/rn/scripts/o.sh %
"   let cmd = ":silent !tmux split-window -f -p 10 -vb "
"   let cmd = cmd . '/home/dsicilia/dev/rn/scripts/vim-build.sh o ' . %
"   exec cmd
"   :silent !tmux select-pane -D
" endfunction
"
function! TmuxBuild( target )
  let module = ''
  if a:target == 'o'
    let module = @%
  endif
  let cmd = ":silent !tmux split-window -f -p 10 -vb "
  let cmd = cmd . '/home/dsicilia/dev/rn/scripts/vim-build.sh ' . a:target
  let cmd = cmd . ' ' . module
  exec cmd
  :silent !tmux select-pane -D
endfunction

" Build command
nnoremap <F5>  :silent call TmuxBuild( 'run' )<CR>
nnoremap <F6>  :silent call TmuxBuild( 'all' )<CR>
nnoremap <F9>  :silent call TmuxBuild( 'test' )<CR>
nnoremap <F10> :silent call TmuxBuild( 'o' )<CR>
