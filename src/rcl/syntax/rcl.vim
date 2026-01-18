" ---------------------------------------------------------------
" Vim syntax file for the rcl config language.
" ---------------------------------------------------------------
if exists("b:current_syntax")
  finish
endif

syn case match

" ---------------------------------------------------------------
" Comments.
" ---------------------------------------------------------------
" Single-line: #
syn match  rclCommentLine  "#.*$" contains=@Spell

" ---------------------------------------------------------------
" Strings.
" ---------------------------------------------------------------
" Double-quoted strings with backslash escapes
syn region rclString start=+"+ skip=+\\\\\|\\"+ end=+"+

" ---------------------------------------------------------------
" Literals.
" ---------------------------------------------------------------
" Booleans: true/false
syn keyword rclBoolean true false

" Null
syn keyword rclNull null

" Numbers:
" - Hex: 0xff :contentReference[oaicite:2]{index=2}
syn match rclNumber /\v<0x\x+>/

" - Decimal / float
syn match rclNumber /\v<\d+(\.\d+)?>/

" ---------------------------------------------------------------
" Keys.
" ---------------------------------------------------------------
" Highlight a "key" token when it precedes =, :, or a block {.
" - bare key:   key = value;
" - section:    section { ... }
" - dotted:     a.b.c { ... }

" Make the key as a region so we can color dots inside it.
syn region rclKey
      \ matchgroup=rclKey
      \ start=/\v^\s*\zs[[:alpha:]_.]/
      \ end=/\v\ze\s*([=:]|\{)/
      \ contains=rclKeyDot

" Dots inside keys.
syn match rclKeyDot /\./ contained

" ---------------------------------------------------------------
" Delimiters.
" ---------------------------------------------------------------
syn match rclDelimiter /[{}\[\],:=]/

" ---------------------------------------------------------------
" Link to standard highlight groups.
" ---------------------------------------------------------------
hi def link rclCommentLine   Comment

hi def link rclString        String

hi def link rclBoolean       Boolean
hi def link rclNull          Constant
hi def link rclNumber        Number

hi def link rclKey           Keyword
hi def link rclKeyDot        Delimiter
hi def link rclDelimiter     Delimiter

" ---------------------------------------------------------------
" Finished.
" ---------------------------------------------------------------
let b:current_syntax = "rcl"