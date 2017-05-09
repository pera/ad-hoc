" Vim syntax file
" Language: Ad-hoc
" Maintainer: Brian Gomes Bascoy
" Last Change: 2016-05-17

if exists("b:current_syntax")
  finish
endif

syn case ignore

syn region adhocCommentLine start="--" end="$" contains=adhocTodo
syn region adhocCommentBlock start="---" end="---" contains=adhocTodo
syn keyword adhocTodo TODO FIXME XXX

syn region adhocDelimiter matchgroup=Delimiter start="\[" matchgroup=Delimiter end="\]" transparent
syn region adhocString start=+"+ skip=+\\\\\|\\"+ end=+"+ contains=adhocEscape
syn region adhocString start=+'+ skip=+\\\\\|\\'+ end=+'+ contains=adhocEscape

syn match adhocDelimiter "|"
syn match adhocOperator "<<<*"
syn match adhocOperator ">>>*"
syn keyword adhocInclude import
syn keyword adhocConditional if else
syn keyword adhocFunction map fold filter

hi def link adhocCommentLine Comment
hi def link adhocCommentBlock Comment
hi def link adhocDelimiter Delimiter
hi def link adhocOperator Operator
hi def link adhocString String
hi def link adhocInclude Include
hi def link adhocFunction Function
hi def link adhocConditional Conditional

syn sync minlines=200
syn sync maxlines=500

let b:current_syntax = "adhoc"
