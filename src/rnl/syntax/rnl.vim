" Vim syntax file
" Language: RNL (Revolution|Now Language)
" Latest Revision: 08 November 2020

if exists( "b:current_syntax" )
  finish
endif

" ===============================================================
" Comments / Stmt
" ===============================================================
"set commentstring=#\ %s
syn match    rnlLineComment '#.*$'
syn match    rnlSemi ';'

hi def link  rnlLineComment Comment
"
" ===============================================================
" Headers
" ===============================================================
syn keyword  rnlImportKeyword import nextgroup=rnlModuleName skipwhite
syn match    rnlModuleName '[a-z][-a-zA-Z0-9_]*'

syn keyword  rnlIncludeKeyword include nextgroup=rnlIncludeName skipwhite
syn match    rnlIncludeName '"[-a-zA-Z0-9_/.]\+"'

syn keyword  rnlNamespaceKeyword namespace nextgroup=rnlNamespaceName skipwhite
syn match    rnlNamespaceName /[a-z][a-zA-Z0-9_.]*/

hi def link  rnlImportKeyword Keyword
hi def link  rnlModuleName Identifier
hi def link  rnlIncludeKeyword Keyword
hi def link  rnlNamespaceKeyword Keyword
hi def link  rnlIncludeName String
"
" ===============================================================
" Sum type
" ===============================================================
syn keyword  rnlSumtypeKeyword sumtype nextgroup=rnlSumtypeName skipwhite
syn match    rnlSumtypeName '[a-zA-Z][a-zA-Z0-9_]*' contained nextgroup=rnlSumtypeDescBlock
syn region   rnlSumtypeDescBlock start="{" end="};" fold transparent contains=rnlSumtypeFeatures,rnlSumtypeTemplate,rnlSumtypeAlternative,rnlLineComment

syn match    rnlSumtypeTemplate '\.template:' contained nextgroup=rnlSumtypeTemplateParams skipwhite skipempty
syn match    rnlSumtypeTemplateParams '[a-zA-Z][a-zA-Z0-9_]*,\?' contained nextgroup=rnlSumtypeTemplateParams nextgroup=rnlSemi skipwhite

syn match    rnlSumtypeFeatures '\.features:' contained nextgroup=rnlSumtypeFeaturesValues skipwhite skipempty
syn match    rnlSumtypeFeaturesValues /\(serializable\|formattable\)[,]\?/ contained nextgroup=rnlSumtypeFeaturesValues,rnlSemi skipwhite skipempty

syn match    rnlSumtypeAlternative '[a-zA-Z][a-zA-Z0-9_]*:' contained nextgroup=rnlCppDeclType,rnlSumtypeAlternative skipwhite skipempty
syn match    rnlCppDeclType '[a-zA-Z0-9:<>*]\+\s\+' contained nextgroup=rnlCppDeclVar skipwhite
syn match    rnlCppDeclVar '[a-zA-Z][a-zA-Z0-9_]*;' contained nextgroup=rnlCppDeclType,rnlSumtypeAlternative skipwhite skipempty

hi def link  rnlSumtypeKeyword Keyword
hi def link  rnlSumtypeName Identifier
"hi def link rnlSumtypeDescBlock Structure
hi def link  rnlSumtypeTemplate Keyword
hi def link  rnlSumtypeTemplateParams Type
hi def link  rnlSumtypeFeatures Keyword
hi def link  rnlSumtypeFeaturesValues Constant
"hi def link  rnlSumtypeAlternative Identifier
hi def link  rnlCppDeclType Type
"hi def link rnlCppDeclVar Statement