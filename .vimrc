" Revolution-Now-specific vim configuration.
set sw=2
set shiftwidth=2
set tabstop=2

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

let g:focused_tab = 1

function! s:TabProposedPrevious()
  let g:focused_tab = g:focused_tab - 1
  if g:focused_tab < 1
    let g:focused_tab = tabpagenr( '$' )
    return
  endif
  set tabline=%!MyTabLine()
endfunction

function! s:TabProposedNext()
  let g:focused_tab = g:focused_tab + 1
  if g:focused_tab > tabpagenr( '$' )
    let g:focused_tab = 1
    return
  endif
  set tabline=%!MyTabLine()
endfunction

function! s:TabProposedSelect()
  exec ':tabn ' . g:focused_tab
  set tabline=%!MyTabLine()
endfunction

function! s:CRWrapper()
  if g:focused_tab != tabpagenr()
    call s:TabProposedSelect()
    return
  endif
  call feedkeys( ":noh\<CR>" )
endfunction

function! MyTabLine()
  let s = ''
  for i in range(tabpagenr('$'))
    " select the highlighting
    if i + 1 == tabpagenr()
      let s .= '%#TabLineSel#'
    elseif i + 1 == g:focused_tab
      let s .= '%#Keyword#'
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
  endfor
  let winnr = tabpagewinnr( a:n )
  return bufname( buflist[winnr - 1] )
endfunction

set tabline=%!MyTabLine()

" Automatically format the C++ source files just before saving.
autocmd BufWritePre *.hpp,*.cpp call ClangFormatAll()

" Get the folder in which this file resides.
let s:path = expand( '<sfile>:p:h' )

" We set this ycm global variable to point YCM to the conf script.  The
" reason we don't just put a .ycm_extra_conf.py in the root folder
" (which YCM would then find on its own without our help) is that we
" want to keep the folder structure organized with all scripts in the
" scripts folder.
let g:ycm_global_ycm_extra_conf = s:path . '/scripts/ycm_extra_conf.py'

" Tell the vim-templates function where to find the templates.
let g:tmpl_search_paths = [s:path . '/scripts/templates']

command! TabProposedPrevious call s:TabProposedPrevious()
command! TabProposedNext     call s:TabProposedNext()
command! TabProposedSelect   call s:TabProposedSelect()

unmap [
unmap ]

nnoremap [ :TabProposedPrevious<CR>
nnoremap ] :TabProposedNext<CR>

command! CRWrapper call s:CRWrapper()

unmap <CR>
nnoremap <silent> <CR> :CRWrapper<CR>
