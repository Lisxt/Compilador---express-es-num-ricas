#ifndef SEMANTICO_H
#define SEMANTICO_H

#include "sintatico.h" /* ASTKind/AST */
#include <stddef.h>

// Tipos da linguagem (TY_NUM). Deixamos em enum caso precisemos colocar
// string/bool depois.
typedef enum { TY_NUM = 1, TY_ERROR = -1 } TypeKind;

/* Resultado consolidado de checagem semântica por nó. */
typedef struct {
  TypeKind type; /* tipo inferido para a expressão */
  int is_const;  /* 1 se o valor é constante em tempo de compilação */
  double cval;   /* valor constante (válido se is_const = 1) */
  int has_error; /* 1 se houve erro em algum ponto da subárvore */
} SemInfo;

/* Executa verificação semântica da AST.
   - Preenche errbuf (se não nulo) com a primeira mensagem de erro encontrada.
   - Retorna 0 se houve erro; 1 se está semanticamente válida. */
int sem_check(const AST *t, char *errbuf, size_t errbufsz);

/* Aplica constant folding:
   - Substitui subárvores puramente constantes por nós AST_NUM.
   - Retorna a (possivelmente) nova raiz (pode reutilizar nós existentes). */
AST *sem_fold(AST *t);

/* Avalia expressão se (e somente se) toda a AST for constante.
   - Em caso de sucesso, escreve em *out e retorna 1; caso contrário, 0. */
int sem_eval_const(const AST *t, double *out);

#endif