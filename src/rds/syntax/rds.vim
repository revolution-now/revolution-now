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
syn keyword  rdsNamespaceKeyword namespace nextgroup=rdsNamespaceDot,rdsNamespaceName skipwhite skipempty
syn match    rdsNamespaceDot '\.' contained nextgroup=rdsNamespaceName
syn match    rdsNamespaceName '[a-zA-Z_][a-zA-Z0-9_]\+' contained nextgroup=rdsNamespaceBlock,rdsNamespaceKeywordDot skipwhite skipempty
syn region   rdsNamespaceBlock start='{' end='}' contained fold contains=rdsSumtypeKeyword,rdsStructKeyword,rdsEnumKeyword,rdsNamespaceKeyword,rdsLineComment skipwhite skipempty
syn match    rdsNamespaceKeywordDot '\.' contained nextgroup=rdsSumtypeKeyword,rdsStructKeyword,rdsEnumKeyword,rdsNamespaceKeyword

hi def link  rdsNamespaceKeyword Keyword
hi def link  rdsNamespaceName Identifier

" ===============================================================
" Enum
" ===============================================================
syn keyword  rdsEnumKeyword enum contained nextgroup=rdsEnumDot,rdsEnumName skipwhite skipempty
syn match    rdsEnumDot '\.' contained nextgroup=rdsEnumName
syn match    rdsEnumName '[a-zA-Z_][a-zA-Z0-9_]*' contained nextgroup=rdsEnumListBlock,rdsEnumListBlockErr skipwhite skipempty
syn region   rdsEnumListBlockErr start='{' end='}' contained fold skipwhite skipempty
syn region   rdsEnumListBlock start='\[' end='\]' contained fold contains=rdsEnumItem,rdsLineComment skipwhite skipempty
syn match    rdsEnumItem '[a-zA-Z_][a-zA-Z0-9_]*' contained skipwhite skipempty

hi def link  rdsEnumKeyword Keyword
" hi def link  rdsEnumName Type
hi def link  rdsEnumItem Identifier
hi def link  rdsEnumListBlockErr Error

" ===============================================================
" Sumtype
" ===============================================================
syn keyword  rdsSumtypeKeyword sumtype contained nextgroup=rdsSumtypeDot,rdsSumtypeName skipwhite skipempty

syn match    rdsSumtypeDot '\.' contained nextgroup=rdsSumtypeName
syn match    rdsSumtypeName '[a-zA-Z_][a-zA-Z0-9_]\+' contained nextgroup=rdsSumtypeTableBlock,rdsSumtypeNameDot,rdsSumtypeTemplate,rdsSumtypeFeatures skipwhite
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
hi def link  rdsSumtypeAlternative Identifier
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

" ===============================================================
" Struct
" ===============================================================
syn keyword  rdsStructKeyword struct contained nextgroup=rdsStructDot,rdsStructName skipwhite skipempty

syn match    rdsStructDot '\.' contained nextgroup=rdsStructName
syn match    rdsStructName '[a-zA-Z_][a-zA-Z0-9_]\+' contained nextgroup=rdsStructTableBlock,rdsStructNameDot,rdsStructTemplate,rdsStructFeatures skipwhite
syn region   rdsStructTableBlock start='{' end='}' contained fold contains=rdsStructMember,rdsStructFeatures,rdsStructTemplate,rdsLineComment

syn match    rdsStructMember '[a-zA-Z][a-zA-Z0-9_]*' contained nextgroup=rdsStructMemberColon,rdsStructMemberEquals skipwhite skipempty
syn match    rdsStructMemberColon ':' contained nextgroup=rdsStructMemberTypeQuoted,rdsStructMemberTypeUnquoted skipwhite skipempty
syn match    rdsStructMemberEquals '=' contained nextgroup=rdsStructMemberTypeQuoted,rdsStructMemberTypeUnquoted skipwhite skipempty
syn region   rdsStructMemberTypeQuoted start="'" end="'" contained contains=rdsStructMemberTypeQuotedContents,rdsStructMemberTypeError  skipwhite skipempty
syn match    rdsStructMemberTypeError "[^']\+" contained
syn match    rdsStructMemberTypeQuotedContents "[a-zA-Z_][a-zA-Z0-9_:\*, <>]*" contained
syn match    rdsStructMemberTypeUnquoted "[a-zA-Z_][a-zA-Z0-9_]*" contained

syn keyword  rdsStructTemplate _template contained nextgroup=rdsStructTemplateColon,rdsStructTemplateListBlock skipwhite skipempty
syn match    rdsStructTemplateColon ':' contained nextgroup=rdsStructTemplateListBlock skipwhite skipempty
syn region   rdsStructTemplateListBlock start='\[' end='\]' contained fold contains=rdsStructTemplateListItem,rdsStructTemplateListItemErr skipwhite skipempty
syn match    rdsStructTemplateListItemErr '[^\[\] ,]\+' contained
syn match    rdsStructTemplateListItem '[a-zA-Z0-9]\+' contained

syn keyword  rdsStructFeatures _features contained nextgroup=rdsStructFeaturesColon,rdsStructFeaturesListBlock skipwhite skipempty
syn match    rdsStructFeaturesColon ':' contained nextgroup=rdsStructFeaturesListBlock skipwhite skipempty
syn region   rdsStructFeaturesListBlock start='\[' end='\]' contained fold contains=rdsStructFeaturesListItem,rdsStructFeaturesListItemErr skipwhite skipempty
syn match    rdsStructFeaturesListItemErr '[^\[\] ,]\+' contained
syn match    rdsStructFeaturesListItem '\(equality\|formattable\|serializable\|validation\)' contained

syn match    rdsStructNameDot '\.' contained nextgroup=rdsStructFeatures,rdsStructTemplate

hi def link  rdsStructKeyword Keyword
hi def link  rdsStructMember Identifier
hi def link  rdsStructMemberTypeQuoted Comment
hi def link  rdsStructMemberTypeQuotedContents String
hi def link  rdsStructMemberTypeUnquoted String
hi def link  rdsStructMemberTypeError Error

hi def link  rdsStructFeatures Keyword
hi def link  rdsStructFeaturesListItemErr Error
hi def link  rdsStructFeaturesListItem Tag

hi def link  rdsStructTemplate Keyword
hi def link  rdsStructTemplateListItemErr Error
hi def link  rdsStructTemplateListItem Tag