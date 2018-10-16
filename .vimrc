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
  endfor
  let winnr = tabpagewinnr( a:n )
  return bufname( buflist[winnr - 1] )
endfunction

set tabline=%!MyTabLine()

" Automatically format the C++ source files just before saving.
autocmd BufWritePre *.hpp,*.cpp call ClangFormatAll()
