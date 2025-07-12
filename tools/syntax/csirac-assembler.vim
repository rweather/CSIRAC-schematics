" Syntax highlighting for the CSIRAC New Assembler; CNA extension.

" Quit when a syntax file was already loaded.
if exists("b:current_syntax")
  finish
endif

" Match syntactic elements.
syn match   cnaComment          /;.*$/
syn match   cnaLabel            /[A-Za-z_][A-Za-z0-9_]*:/
syn match   cnaVariable         /[A-Za-z_][A-Za-z0-9_]*/
syn match   cnaDirective        /\.[A-Za-z_][A-Za-z0-9_]*/
syn match   cnaConstant         /[0-9][0-9]*/

syn match   cnaSource           /([^)]*M)/
syn match   cnaSource           "(I)"
syn match   cnaSource           "(N1)"
syn match   cnaSource           "(N2)"
syn match   cnaSource           "(A)"
syn match   cnaSource           "s(A)"
syn match   cnaSource           "r(A)"
syn match   cnaSource           "2(A)"
syn match   cnaSource           "p1(A)"
syn match   cnaSource           "c(A)"
syn match   cnaSource           "z(A)"
syn match   cnaSource           "(B)"
syn match   cnaSource           "(R)"
syn match   cnaSource           "r(B)"
syn match   cnaSource           "(C)"
syn match   cnaSource           "s(C)"
syn match   cnaSource           "r(C)"
syn match   cnaSource           /(D[0-9][0-9]*)/
syn match   cnaSource           /s(D[0-9][0-9]*)/
syn match   cnaSource           /r(D[0-9][0-9]*)/
syn match   cnaSource           "(Z)"
syn match   cnaSource           "(Hl)"
syn match   cnaSource           "(Hu)"
syn match   cnaSource           "(S)"
syn match   cnaSource           "\<p11\>"
syn match   cnaSource           "([^)]*K)"
syn match   cnaSource           "([^)]*[abcd])"
syn match   cnaSource           "\<p20\>"

syn match   cnaDestination      "\<M\>"
syn match   cnaDestination      "\<I\>"
syn match   cnaDestination      "\<Ot\>"
syn match   cnaDestination      "\<Op\>"
syn match   cnaDestination      "\<A\>"
syn match   cnaDestination      "+A"
syn match   cnaDestination      "-A"
syn match   cnaDestination      /\.A/
syn match   cnaDestination      "\<vA\>"
syn match   cnaDestination      /\~A/
syn match   cnaDestination      "\<P\>"
syn match   cnaDestination      "\<B\>"
syn match   cnaDestination      "\<xB\>"
syn match   cnaDestination      "\<Lx\>"
syn match   cnaDestination      "\<C\>"
syn match   cnaDestination      "+C"
syn match   cnaDestination      "-C"
syn match   cnaDestination      /D[0-9][0-9]*/
syn match   cnaDestination      /[+]D[0-9][0-9]*/
syn match   cnaDestination      /-D[0-9][0-9]*/
syn match   cnaDestination      "\<Z\>"
syn match   cnaDestination      "\<Hl\>"
syn match   cnaDestination      "\<Hu\>"
syn match   cnaDestination      "\<S\>"
syn match   cnaDestination      "+S"
syn match   cnaDestination      "\<cS\>"
syn match   cnaDestination      "+K"
syn match   cnaDestination      "\<a\>"
syn match   cnaDestination      "\<b\>"
syn match   cnaDestination      "\<c\>"
syn match   cnaDestination      "\<d\>"
syn match   cnaDestination      "\<T\>"

syn match   cnaMove             "->"

syn match   cnaString           /'[^']*'/
syn match   cnaString           /"[^"]*"/

" Set the highlighting categories.
hi def link cnaComment          Comment
hi def link cnaLabel            Define
hi def link cnaDirective        Type
hi def link cnaSource           Statement
hi def link cnaDestination      Statement
hi def link cnaMove             Type
hi def link cnaConstant         Number
hi def link cnaString           String
hi def link cnaVariable         Number

" Activate the language.
let b:current_syntax = "csirac-assembler"
