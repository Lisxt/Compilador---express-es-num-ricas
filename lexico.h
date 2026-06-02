#ifndef LEXICO_H
#define LEXICO_H

#define NUM    256
#define PLUS   '+'
#define MINUS  '-'
#define MULT   '*'
#define DIV    '/'
#define LPAREN '('
#define RPAREN ')'
#define END    0

#define TOKEN_IDENT    257   /* identificador definido pelo programador */
#define TOKEN_PROGRAM  258   /* palavra reservada: program              */
#define TOKEN_VAR      259   /* palavra reservada: var                  */
#define TOKEN_INTEGER  260   /* palavra reservada: integer              */
#define TOKEN_REAL     261   /* palavra reservada: real                 */
#define TOKEN_BEGIN    262   /* palavra reservada: begin                */
#define TOKEN_END      263   /* palavra reservada: end                  */
#define TOKEN_IF       264   /* palavra reservada: if                   */
#define TOKEN_THEN     265   /* palavra reservada: then                 */
#define TOKEN_ELSE     266   /* palavra reservada: else                 */
#define TOKEN_WHILE    267   /* palavra reservada: while                */
#define TOKEN_DO       268   /* palavra reservada: do                   */
#define TOKEN_ASSIGN   269   /* operador de atribuição :=               */
#define TOKEN_NEQ      270   /* operador relacional <>                  */
#define TOKEN_LEQ      271   /* operador relacional <=                  */
#define TOKEN_GEQ      272   /* operador relacional >=                  */
#define TOKEN_EOF      273   /* fim do arquivo fonte                    */
typedef struct {
    int    type;
    double value;  
    char   lexeme[64]; 
    int   line;
} Token;

/*
 * Vetor dinâmico de tokens.
 * Mantido do projeto base para compatibilidade com tokenize_to_vector().
 */
typedef struct {
    Token *data;
    int size;
    int cap;
} TokenVec;
typedef struct {
    const char *src; 
    int         pos;  
    int         line;  
} Lexer;

TokenVec    tokenize_to_vector(const char *src);
void        tv_free(TokenVec *v);
const char *token_name(int t);

// Funções que o parser usa pra consumir os tokens sob demanda

void  lexer_init(Lexer *lex, const char *src);
Token proxToken(Lexer *lex);

#endif