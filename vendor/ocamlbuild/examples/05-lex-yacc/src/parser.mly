
%token <int> INT
%token PLUS MINUS TIMES DIV
%token LPAREN RPAREN
%left PLUS MINUS        /* lowest precedence */
%left TIMES DIV         /* medium precedence */
%nonassoc UMINUS        /* highest precedence */
%start main             /* the entry point */
%token EOF
%type <int> main

%%

(* Menhir let us give names to symbol values,
   instead of having to use $1, $2, $3 as in ocamlyacc *)
main
  : e = expr EOF                { e }
  ;

expr
  : n = INT                 { n }
  | LPAREN e = expr RPAREN  { e }
  | e1=expr PLUS  e2=expr   { e1 + e2 }
  | e1=expr MINUS e2=expr   { e1 - e2 }
  | e1=expr TIMES e2=expr   { e1 * e2 }
  | e1=expr DIV   e2=expr   { e1 / e2 }
  | MINUS e = expr %prec UMINUS { - e }
  ;
