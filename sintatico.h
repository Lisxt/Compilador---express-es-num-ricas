#ifndef SINTATICO_H
#define SINTATICO_H

#include "lexico.h"

/* Enumeração de nós para a árvore sintática (AST) */
typedef enum {
    /* Nós herdados da calculadora original */
    AST_NUM, AST_ADD, AST_SUB, AST_MUL, AST_DIV,
    
    /* Novos nós para o MicroPascal*/
    AST_PROG,       /* Nó de programa */
    AST_BLOCK,      /* Nó de bloco */
    AST_DECL,       /* Nó de declaração de variáveis */
    AST_VAR,        /* Nó de variável / identificador */
    
    /* Nós reservados para uso das Pessoas 3 e 4 */
    AST_ASSIGN,     /* Atribuição (:=) */
    AST_IF,         /* Estrutura condicional (if-then-else) */
    AST_WHILE,      /* Estrutura de repetição (while-do) */
    AST_REL         /* Operadores relacionais */
} ASTKind;

/* Estrutura do nó da árvore sintática abstrata (AST) */
typedef struct AST {
    ASTKind kind;
    double num;          /* Válido se kind == AST_NUM */
    char id_name[64];    /* Guarda o nome de identificadores se kind == AST_VAR */
    struct AST *left;    /* Filho esquerdo */
    struct AST *right;   /* Filho direito */
    struct AST *next;    /* Próximo nó (para listas de comandos/declarações) */
} AST;

/* Estrutura do Parser modificada para usar o Lexer sob demanda*/
typedef struct {
    Lexer *lexer;          /* Analisador léxico */
    Token token_corrente;  /* Janela do token sob análise atual */
} Parser;

/* API do Analisador Sintático */
AST* parse_ast(Lexer *lex);
void ast_print(const AST *t, int depth);
void ast_free(AST *t);

/* Não-terminais da Gramática - (Estrutura e Declarações) */
AST* programa(Parser *p);
AST* bloco(Parser *p);
AST* parteDeclaracoesVariaveis(Parser *p);
AST* declaracaoVariaveis(Parser *p);
AST* listaIdentificadores(Parser *p);
AST* tipo(Parser *p);

/* Não-terminais da Gramática - PESSOA 3 (Comandos - Stubs para integração) */
AST* comandoComposto(Parser *p);
AST* comando(Parser *p);
AST* atribuicao(Parser *p);
AST* comandoCondicional(Parser *p);
AST* comandoRepetitivo(Parser *p);

/* Não-terminais da Gramática - PESSOA 4 (Expressões - Mantidos da calculadora original) */
AST* expressao(Parser *p);
AST* relacao(Parser *p);
AST* expressaoSimples(Parser *p);
AST* termo(Parser *p);
AST* fator(Parser *p);
AST* variavel(Parser *p);

#endif /* SINTATICO_H */