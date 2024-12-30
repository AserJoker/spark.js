program -> [
    interpreter_directive,
    directive*,
    statement*,
]
interpreter-directive -> [
    invisible*,
    reg(#!.*),
    LTC*
]
LTC -> reg(\u0xa|\u0xd|\u0x2028|\u0x29)
WSC -> reg(\u0x9|\u0xb|\u0xc|\u0x20|\u0xa0|\u0xfeff)
comment -> [
    (LTC|WCS)*,
    reg(\/\/.*),
]
multiline-comment -> [
    (LTC|WCS)*,
    reg(\/\*.*\*\/,multiline)
]
invisible -> (LTC|WCS|comment|multiline-comment)
string -> reg('(^')*')|reg("(^")*")
directive -> string
number -> reg(0?.[0-9]+((e|E)((0?.[0-9]+)|([0-9]+))))|reg([1-9][0-9]*)|reg(0(x|X)([0-9]|[a-z]|[A-Z])+)
bool -> reg(true)|reg(false)
regex -> reg(\/.+\/g?m?i?u?)
keyword -> reg(break|case|catch|class|const|continue|debugger|default|delete|do|else|export|extends|false|finally|for|function|if|import|in|instanceof|new|null|return|super|switch|this|throw|true|try|typeof|var|void|while|with|enum|implements|interface|let|package|private|protected|public|static|yield)
word -> reg(([a-z]|[A-Z]|_|$)([a-z]|[A-Z]|[0-9])*)
statement -> empty-statement|label-statement|block-statement|import-declaration|export-declaration|debugger-statement|while-statement|do-while-statement|return-statement|break-statement|continue-statement|throw-statement|if-statement|switch-statement|try-statement|for-of-statement|for-in-statement|for-statement|function-declaration|class-declaration|[variable-declaration|expression-statement,(reg(;)|eof)]
empty-statement -> [invisible*,reg(;)]
label-statement -> invisible*,[word,reg(:),statement]
block-statement -> [invisible*,reg({),statement*,invisible*,reg(})]
debugger-statement -> [invisible*,reg(debugger)]
while-statement -> [invisible*,reg(while),invisible*,reg(\(),expression,invisible*,reg(\)),block-statement|expression]
do-while-statement -> [invisible*,reg(do),block-statement|expression,invisible*,reg(while),invisible*,reg(\(),expression,reg(\))]

