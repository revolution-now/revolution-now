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
