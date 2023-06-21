" Get the folder in which this file resides.
let s:this_folder = fnamemodify( resolve( expand( '<sfile>:p' ) ), ':h' )

" This lua file contains the actual code.
exec ':source ' . s:this_folder . '/nvimrc.lua'
