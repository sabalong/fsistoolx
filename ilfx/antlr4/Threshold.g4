grammar Threshold;

ruleFile
    : ruleDecl EOF
    ;

ruleDecl
    : rating ':' expr
    ;

rating
    : INT
    ;

expr
    : orExpr
    ;

orExpr
    : andExpr (OR andExpr)*
    ;

andExpr
    : comparison (AND comparison)*
    ;

comparison
    : value op value (op value)*
    | '(' expr ')'
    ;

value
    : INT
    | FLOAT
    | IDENT
    ;

op
    : '<'
    | '<='
    | '>'
    | '>='
    | '=='
    ;

AND    : [aA][nN][dD] | '&&' ;
OR     : [oO][rR] | '||' ;
FLOAT  : [0-9]+ '.' [0-9]+ ;
INT    : [0-9]+ ;
IDENT  : [a-zA-Z_][a-zA-Z_0-9]* ;
WS     : [ \t\r\n]+ -> skip ;
