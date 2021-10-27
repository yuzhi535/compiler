%{
    /* definitoins of manifest constants
    LT, LE, EQ, NE, GT, GE,
    IF, THENm ELSE, IDm NUMBER, RELOP*/    
    
%}

/* regular definitions */
delim   [ \t\n]
ws  {delim}+
letter  [a-zA-Z]
digit   [0-9]
id    {letter}({letter}|{digit})*
number  {digit}+(\.{digit}+)?(E[+--]?{digit}+)?

%%

{ws}    {/* no action and no return */}
if      {return {IF}}
then    {return(THEN);}
else    {return(ELSE);}
{id}    {yylval = (int) installID(); return(ID);]}
{number} {yylval = (int) installNum(); return(NUMBER);}
"<"     {yylval = LT; return(RELOP);}
"<="    {yylval = LE; return(RELOP);}
">="    {yylval = GE; return(RELOP);}
">"    {yylval = GT; return(RELOP);}
"<>"    {yylval = NE; return(RELOP);}
"="    {yylval = EQ; return(RELOP);}

%%

int installID() {

}

int installNum() {

}