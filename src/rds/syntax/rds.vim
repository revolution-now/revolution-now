" Vim syntax file
" Language: Rds (special case of Lua)

if exists( "b:current_syntax" )
  finish
endif

" ===============================================================
" Comments / Stmt
" ===============================================================
syn keyword luaTodo contained TODO FIXME

set commentstring=#\ %s
syn match    rdsLineComment '#.*$' contains=luaTodo,@Spell

hi def link  rdsLineComment Comment
hi def link  luaTodo        Todo

" ===============================================================
" Include
" ===============================================================
syn keyword  rdsIncludeKeyword include nextgroup=rdsIncludeQuoted skipwhite skipempty
syn region   rdsIncludeQuoted start='"' end='"' contains=rdsIncludeQuotedContents skipwhite skipempty
syn match    rdsIncludeQuotedContents "[<a-zA-Z_][a-zA-Z0-9_\.>/-]*" contained skipwhite skipempty

hi def link  rdsIncludeKeyword Keyword
hi def link  rdsIncludeQuoted Comment
hi def link  rdsIncludeQuotedContents String

" ===============================================================
" Namespace
" ===============================================================
syn keyword  rdsNamespaceKeyword namespace nextgroup=rdsNamespaceQuoted skipwhite skipempty
syn region   rdsNamespaceQuoted start="'" end="'" contained contains=rdsNamespaceQuotedContents skipwhite skipempty
syn match    rdsNamespaceQuotedContents "[a-zA-Z_][a-zA-Z0-9_\.]*" contained skipwhite skipempty

hi def link  rdsNamespaceKeyword Keyword
hi def link  rdsNamespaceQuoted Comment
hi def link  rdsNamespaceQuotedContents String

" ===============================================================
" Enum
" ===============================================================
syn keyword  rdsEnumKeyword enum nextgroup=rdsEnumDot,rdsEnumName skipwhite skipempty
syn match    rdsEnumDot '\.' contained nextgroup=rdsEnumName
syn match    rdsEnumName '[a-zA-Z_][a-zA-Z0-9_]*' contained nextgroup=rdsEnumListBlock,rdsEnumListBlockErr skipwhite skipempty
syn region   rdsEnumListBlock start='{' end='}' contained fold contains=rdsEnumItem,rdsLineComment skipwhite skipempty
syn match    rdsEnumItem '[a-zA-Z_][a-zA-Z0-9_]*' contained nextgroup=rdsEnumItemComma skipwhite skipempty
syn match    rdsEnumItemComma ',' contained nextgroup=rdsEnumItem skipwhite skipempty

hi def link  rdsEnumKeyword      Keyword
hi def link  rdsEnumName         None
hi def link  rdsEnumItem         Identifier
hi def link  rdsEnumItemComma    Comment
hi def link  rdsEnumListBlockErr Error

" ===============================================================
" Sumtype
" ===============================================================
syn keyword  rdsSumtypeKeyword sumtype nextgroup=rdsSumtypeDot skipwhite skipempty

syn match    rdsSumtypeDot '\.' nextgroup=rdsSumtypeName
syn match    rdsSumtypeName '[a-zA-Z_][a-zA-Z0-9_]\+' nextgroup=rdsSumtypeTableBlock skipwhite skipempty
syn region   rdsSumtypeTableBlock start='{' end='}' contained fold contains=rdsSumtypeAlternative,rdsSumtypeFeatures,rdsSumtypeTemplate,rdsLineComment

syn match    rdsSumtypeAlternative '[a-zA-Z][a-zA-Z0-9_]*' contained nextgroup=rdsSumtypeAlternativeBlock skipwhite skipempty
syn region   rdsSumtypeAlternativeBlock start='{' end='}' contained fold contains=rdsSumtypeAlternativeVar,rdsLineComment nextgroup=rdsSumtypeAlternativeBlockComma skipwhite skipempty
syn match    rdsSumtypeAlternativeBlockComma ',' contained nextgroup=rdsSumtypeAlternative,rdsSumtypeTemplate,rdsSumtypeFeatures skipwhite skipempty
syn match    rdsSumtypeAlternativeVar '[a-zA-Z_][a-zA-Z0-9_]*' contained nextgroup=rdsSumtypeAlternativeVarTypeQuoted skipwhite skipempty
syn region   rdsSumtypeAlternativeVarTypeQuoted start="'" end="'" contained contains=rdsSumtypeAlternativeVarTypeQuotedContents,rdsSumtypeAlternativeVarTypeError nextgroup=rdsSumtypeAlternativeVarComma skipwhite skipempty
syn match    rdsSumtypeAlternativeVarTypeError "[^']\+" contained
syn match    rdsSumtypeAlternativeVarTypeQuotedContents "[a-zA-Z_][a-zA-Z0-9_:\*, <>]*" contained skipwhite skipempty
syn match    rdsSumtypeAlternativeVarComma ',' nextgroup=rdsSumtypeAlternativeVar skipwhite skipempty

syn keyword  rdsSumtypeTemplate _template contained nextgroup=rdsSumtypeTemplateListBlock skipwhite skipempty
syn region   rdsSumtypeTemplateListBlock start='{' end='}' contained fold contains=rdsSumtypeTemplateListItem,rdsSumtypeTemplateListItemErr nextgroup=rdsSumtypeTemplateListBlockComma skipwhite skipempty
syn match    rdsSumtypeTemplateListBlockComma ',' contained nextgroup=rdsSumtypeAlternative,rdsSumtypeFeatures skipwhite skipempty
syn match    rdsSumtypeTemplateListItemErr '[^{} ,]\+' contained
syn match    rdsSumtypeTemplateListItem '[a-zA-Z0-9]\+' contained nextgroup=rdsSumtypeTemplateListItemComma skipwhite skipempty
syn match    rdsSumtypeTemplateListItemComma ',' contained nextgroup=rdsSumtypeTemplateListItem skipwhite skipempty

syn keyword  rdsSumtypeFeatures _features contained nextgroup=rdsSumtypeFeaturesListBlock skipwhite skipempty
syn region   rdsSumtypeFeaturesListBlock start='{' end='}' contained fold contains=rdsSumtypeFeaturesListItem,rdsSumtypeFeaturesListItemErr nextgroup=rdsSumtypeFeaturesListBlockComma skipwhite skipempty
syn match    rdsSumtypeFeaturesListBlockComma ',' contained nextgroup=rdsSumtypeAlternative,rdsSumtypeTemplate skipwhite skipempty
syn match    rdsSumtypeFeaturesListItemErr '[^{} ,]\+' contained
syn match    rdsSumtypeFeaturesListItem '\(equality\|validation\|offsets\)' contained

hi def link  rdsSumtypeDot                    Comment
hi def link  rdsSumtypeAlternativeVarComma    Comment
hi def link  rdsSumtypeAlternativeBlockComma  Comment
hi def link  rdsSumtypeTemplateListItemComma  Comment
hi def link  rdsSumtypeTemplateListBlockComma Comment
hi def link  rdsSumtypeFeaturesListBlockComma Comment

hi def link  rdsSumtypeKeyword               Keyword
hi def link  rdsSumtypeName                  None

hi def link  rdsSumtypeAlternative                      Identifier
hi def link  rdsSumtypeAlternativeVar                   None
hi def link  rdsSumtypeAlternativeVarTypeQuoted         Comment
hi def link  rdsSumtypeAlternativeVarTypeQuotedContents String
hi def link  rdsSumtypeAlternativeVarTypeError          Error

hi def link  rdsSumtypeFeatures            Keyword
hi def link  rdsSumtypeFeaturesListItemErr Error
hi def link  rdsSumtypeFeaturesListItem    Tag

hi def link  rdsSumtypeTemplate            Keyword
hi def link  rdsSumtypeTemplateListItemErr Error
hi def link  rdsSumtypeTemplateListItem    Tag

" ===============================================================
" Struct
" ===============================================================
syn keyword  rdsStructKeyword struct nextgroup=rdsStructDot skipwhite skipempty

syn match    rdsStructDot '\.' nextgroup=rdsStructName
syn match    rdsStructName '[a-zA-Z_][a-zA-Z0-9_]\+' nextgroup=rdsStructTableBlock skipwhite skipempty
syn region   rdsStructTableBlock start='{' end='}' contained fold contains=rdsStructVar,rdsStructFeatures,rdsStructTemplate,rdsLineComment

syn match    rdsStructVar '[a-zA-Z_][a-zA-Z0-9_]*' contained nextgroup=rdsStructVarTypeQuoted skipwhite skipempty
syn region   rdsStructVarTypeQuoted start="'" end="'" contained contains=rdsStructVarTypeQuotedContents,rdsStructVarTypeError nextgroup=rdsStructVarComma skipwhite skipempty
syn match    rdsStructVarTypeError "[^']\+" contained
syn match    rdsStructVarTypeQuotedContents "[a-zA-Z_][a-zA-Z0-9_:\*, <>]*" contained skipwhite skipempty
syn match    rdsStructVarComma ',' nextgroup=rdsStructVar,rdsStructFeatures skipwhite skipempty

syn keyword  rdsStructTemplate _template contained nextgroup=rdsStructTemplateListBlock skipwhite skipempty
syn region   rdsStructTemplateListBlock start='{' end='}' contained fold contains=rdsStructTemplateListItem,rdsStructTemplateListItemErr nextgroup=rdsStructTemplateListBlockComma skipwhite skipempty
syn match    rdsStructTemplateListBlockComma ',' contained nextgroup=rdsStructVar,rdsStructFeatures skipwhite skipempty
syn match    rdsStructTemplateListItemErr '[^{} ,]\+' contained
syn match    rdsStructTemplateListItem '[a-zA-Z0-9]\+' contained nextgroup=rdsStructTemplateListItemComma skipwhite skipempty
syn match    rdsStructTemplateListItemComma ',' contained nextgroup=rdsStructTemplateListItem skipwhite skipempty

syn keyword  rdsStructFeatures _features contained nextgroup=rdsStructFeaturesListBlock skipwhite skipempty
syn region   rdsStructFeaturesListBlock start='{' end='}' contained fold contains=rdsStructFeaturesListItem,rdsStructFeaturesListItemErr nextgroup=rdsStructFeaturesListBlockComma skipwhite skipempty
syn match    rdsStructFeaturesListBlockComma ',' contained nextgroup=rdsStructVar,rdsStructTemplate skipwhite skipempty
syn match    rdsStructFeaturesListItemErr '[^{} ,]\+' contained
syn match    rdsStructFeaturesListItem '\(equality\|nodiscard\|validation\|offsets\)' contained

hi def link  rdsStructVarComma               Comment
hi def link  rdsStructTemplateListItemComma  Comment
hi def link  rdsStructTemplateListBlockComma Comment
hi def link  rdsStructFeaturesListBlockComma Comment

hi def link  rdsStructKeyword                Keyword
hi def link  rdsStructDot                    Comment
hi def link  rdsStructName                   None

hi def link  rdsStructVar                   None
hi def link  rdsStructVarTypeQuoted         Comment
hi def link  rdsStructVarTypeQuotedContents String
hi def link  rdsStructVarTypeError          Error

hi def link  rdsStructFeatures            Keyword
hi def link  rdsStructFeaturesListItemErr Error
hi def link  rdsStructFeaturesListItem    Tag

hi def link  rdsStructTemplate            Keyword
hi def link  rdsStructTemplateListItemErr Error
hi def link  rdsStructTemplateListItem    Tag

" ===============================================================
" Config
" ===============================================================
syn keyword  rdsConfigKeyword config nextgroup=rdsConfigDot skipwhite skipempty

syn match    rdsConfigDot '\.' nextgroup=rdsConfigName
syn match    rdsConfigName '[a-zA-Z_][a-zA-Z0-9_]\+' nextgroup=rdsConfigTableBlock skipwhite skipempty
syn region   rdsConfigTableBlock start='{' end='}' contained fold

hi def link  rdsConfigKeyword                Keyword
hi def link  rdsConfigDot                    Comment
hi def link  rdsConfigName                   None
