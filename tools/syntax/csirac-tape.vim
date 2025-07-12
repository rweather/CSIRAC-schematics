" Syntax highlighting for CSIRAC 12-hole punch tapes with the CVT extension.

" Quit when a syntax file was already loaded.
if exists("b:current_syntax")
  finish
endif

" Match syntactic elements.
syn match   cvtComment          /\*.*$/
syn match   cvtHighWord         /^[ 0-3][0-9] [ 0-3][0-9][^XY]/
syn match   cvtLowWord          /^[ 0-3][0-9] [ 0-3][0-9]X/
syn match   cvtYPunch           /^[ 0-3][0-9] [ 0-3][0-9][X ]Y/

" Set the highlighting categories.
hi def link cvtComment          Comment
hi def link cvtHighWord         Statement
hi def link cvtLowWord          Type
hi def link cvtYPunch           Define

" Activate the language.
let b:current_syntax = "csirac-tape"
