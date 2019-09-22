%token LPAREN RPAREN
%token EOL ASSN SQUOTE EOF REQUIRE
%token <string> MODULE_NAME
%token <string> MODULE_PATH 
%start main             /* the entry point */
%type <string * string> main
%%
main:
    MODULE_NAME ASSN REQUIRE LPAREN SQUOTE MODULE_PATH SQUOTE RPAREN EOF                { ($1, $6) }
;
