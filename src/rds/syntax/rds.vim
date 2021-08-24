" Vim syntax file
" Language: Rds (special case of Rcl)

if exists( "b:current_syntax" )
  finish
endif

" ===============================================================
" Comments / Stmt
" ===============================================================
set commentstring=#\ %s
syn match    rdsLineComment '#.*$'

hi def link  rdsLineComment Comment

" ===============================================================
" Include
" ===============================================================
syn keyword  rdsIncludeKeyword include nextgroup=rdsIncludeBlock skipwhite
syn region   rdsIncludeBlock start='\[' end='\]' contained fold contains=rdsIncludeFile,rdsIncludeFileErr,rdsLineComment
syn match    rdsIncludeFile '[a-zA-Z0-9.<>/_-]\+' contained skipwhite skipempty
syn match    rdsIncludeFileErr '["\']' contained skipwhite skipempty

hi def link  rdsIncludeKeyword Keyword
hi def link  rdsIncludeFile String
hi def link  rdsIncludeFileErr Error

" ===============================================================
" Namespace
" ===============================================================
syn keyword  rdsNamespaceKeyword namespace nextgroup=rdsNamespaceDot
syn match    rdsNamespaceDot '\.' contained nextgroup=rdsNamespaceName
syn match    rdsNamespaceName '[a-zA-Z_][a-zA-Z0-9_]\+' contained nextgroup=rdsNamespaceBlock,rdsNamespaceKeywordDot skipwhite skipempty
syn region   rdsNamespaceBlock start='{' end='}' contained fold contains=rdsSumtypeKeyword,rdsEnumKeyword,rdsNamespaceKeyword,rdsLineComment skipwhite skipempty
syn match    rdsNamespaceKeywordDot '\.' contained nextgroup=rdsSumtypeKeyword,rdsEnumKeyword,rdsNamespaceKeyword

hi def link  rdsNamespaceKeyword Keyword
hi def link  rdsNamespaceName Identifier

" ===============================================================
" Enum
" ===============================================================
syn keyword  rdsEnumKeyword enum contained nextgroup=rdsEnumDot,rdsEnumTableBlock skipwhite skipempty
syn match    rdsEnumDot '\.' contained nextgroup=rdsEnumName
syn match    rdsEnumName '[a-zA-Z_][a-zA-Z0-9_]*' contained nextgroup=rdsEnumListBlock,rdsEnumListBlockErr skipwhite skipempty
syn region   rdsEnumListBlockErr start='{' end='}' contained fold skipwhite skipempty
syn region   rdsEnumListBlock start='\[' end='\]' contained fold contains=rdsEnumItem,rdsLineComment skipwhite skipempty
syn match    rdsEnumItem '[a-zA-Z_][a-zA-Z0-9_]*' contained skipwhite skipempty
syn region   rdsEnumTableBlock start='{' end='}' contained fold contains=rdsEnumName,rdsLineComment skipwhite skipempty

hi def link  rdsEnumKeyword Keyword
" hi def link  rdsEnumName Type
hi def link  rdsEnumItem Identifier
hi def link  rdsEnumListBlockErr Error

" ===============================================================
" Sumtype
" ===============================================================
syn keyword  rdsSumtypeKeyword sumtype contained nextgroup=rdsSumtypeDot,rdsSumtypeGroupBlock skipwhite

syn match    rdsSumtypeDot '\.' contained nextgroup=rdsSumtypeName
syn match    rdsSumtypeName '[a-zA-Z_][a-zA-Z0-9_]\+' contained nextgroup=rdsSumtypeTableBlock,rdsSumtypeNameDot skipwhite
syn region   rdsSumtypeTableBlock start='{' end='}' contained fold contains=rdsSumtypeAlternative,rdsSumtypeFeatures,rdsSumtypeTemplate,rdsLineComment
syn match    rdsSumtypeAlternative '[a-zA-Z][a-zA-Z0-9_]*' contained nextgroup=rdsSumtypeAlternativeBlock,rdsSumtypeAlternativeColon,rdsSumtypeAlternativeEquals  skipwhite skipempty
syn match    rdsSumtypeAlternativeColon ':' contained nextgroup=rdsSumtypeAlternativeBlock skipwhite skipempty
syn match    rdsSumtypeAlternativeEquals '=' contained nextgroup=rdsSumtypeAlternativeBlock skipwhite skipempty
syn region   rdsSumtypeAlternativeBlock start='{' end='}' contained fold contains=rdsSumtypeAlternativeVar,rdsLineComment skipwhite skipempty
syn match    rdsSumtypeAlternativeVar '[a-zA-Z_][a-zA-Z0-9_]*' contained nextgroup=rdsSumtypeAlternativeVarColon
syn match    rdsSumtypeAlternativeVarColon ':' contained nextgroup=rdsSumtypeAlternativeVarTypeQuoted,rdsSumtypeAlternativeVarTypeUnquoted  skipwhite skipempty
syn region   rdsSumtypeAlternativeVarTypeQuoted start="'" end="'" contained contains=rdsSumtypeAlternativeVarTypeQuotedContents,rdsSumtypeAlternativeVarTypeError  skipwhite skipempty
syn match    rdsSumtypeAlternativeVarTypeError "[^']\+" contained
syn match    rdsSumtypeAlternativeVarTypeQuotedContents "[a-zA-Z_][a-zA-Z0-9_:\*, <>]*" contained
syn match    rdsSumtypeAlternativeVarTypeUnquoted "[a-zA-Z_][a-zA-Z0-9_]*" contained

syn region   rdsSumtypeGroupBlock start='{' end='}' contained fold contains=rdsSumtypeName skipwhite

syn keyword  rdsSumtypeTemplate _template contained nextgroup=rdsSumtypeTemplateColon,rdsSumtypeTemplateListBlock skipwhite skipempty
syn match    rdsSumtypeTemplateColon ':' contained nextgroup=rdsSumtypeTemplateListBlock skipwhite skipempty
syn region   rdsSumtypeTemplateListBlock start='\[' end='\]' contained fold contains=rdsSumtypeTemplateListItem,rdsSumtypeTemplateListItemErr skipwhite skipempty
syn match    rdsSumtypeTemplateListItemErr '[^\[\] ,]\+' contained
syn match    rdsSumtypeTemplateListItem '[a-zA-Z0-9]\+' contained

syn keyword  rdsSumtypeFeatures _features contained nextgroup=rdsSumtypeFeaturesColon,rdsSumtypeFeaturesListBlock skipwhite skipempty
syn match    rdsSumtypeFeaturesColon ':' contained nextgroup=rdsSumtypeFeaturesListBlock skipwhite skipempty
syn region   rdsSumtypeFeaturesListBlock start='\[' end='\]' contained fold contains=rdsSumtypeFeaturesListItem,rdsSumtypeFeaturesListItemErr skipwhite skipempty
syn match    rdsSumtypeFeaturesListItemErr '[^\[\] ,]\+' contained
syn match    rdsSumtypeFeaturesListItem '\(equality\|formattable\|serializable\)' contained

syn match    rdsSumtypeNameDot '\.' contained nextgroup=rdsSumtypeFeatures,rdsSumtypeTemplate

hi def link  rdsSumtypeKeyword Keyword
" hi def link  rdsSumtypeName Type
hi def link  rdsSumtypeAlternative Structure
hi def link  rdsSumtypeAlternativeVarTypeQuoted Comment
hi def link  rdsSumtypeAlternativeVarTypeQuotedContents String
hi def link  rdsSumtypeAlternativeVarTypeUnquoted String
hi def link  rdsSumtypeAlternativeVarTypeError Error

hi def link  rdsSumtypeFeatures Keyword
hi def link  rdsSumtypeFeaturesListItemErr Error
hi def link  rdsSumtypeFeaturesListItem Tag

hi def link  rdsSumtypeTemplate Keyword
hi def link  rdsSumtypeTemplateListItemErr Error
hi def link  rdsSumtypeTemplateListItem Tag