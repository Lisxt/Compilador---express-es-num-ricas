#include "sintatico.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Função auxiliar para criação de nós genéricos da AST */
static AST *criar_no(ASTKind kind) {
  AST *t = (AST *)calloc(1, sizeof(AST));
  if (!t) {
    fprintf(stderr, "Erro crítico: Falha na alocação de memória para AST.\n");
    exit(1);
  }
  t->kind = kind;
  return t;
}

// Tenta casar o token atual. Se der errado, joga o erro no terminal e mata a
// execução.
void casaToken(Parser *p, int esperado) {
  if (p->token_corrente.type == esperado) {
    p->token_corrente = proxToken(p->lexer);
  } else {
    if (p->token_corrente.type == TOKEN_EOF) {
      fprintf(stderr, "%d:fim de arquivo nao esperado.\n",
              p->token_corrente.line);
    } else {
      fprintf(stderr, "%d:token nao esperado [%s].\n", p->token_corrente.line,
              p->token_corrente.lexeme);
    }
    exit(1);
  }
}

/* FUNÇÕES DOS NÃO-TERMINAIS*/
/* <programa>::= program <identificador>; <bloco>. */
AST *programa(Parser *p) {
  printf("<programa> ::= program <identificador>; <bloco>.\n");

  AST *no_prog = criar_no(AST_PROG);

  casaToken(p, TOKEN_PROGRAM);

  if (p->token_corrente.type == TOKEN_IDENT) {
    strncpy(no_prog->id_name, p->token_corrente.lexeme, 63);
  }
  casaToken(p, TOKEN_IDENT);

  casaToken(p, ';');

  no_prog->left = bloco(p);

  casaToken(p, '.');

  return no_prog;
}

/* <bloco> ::= <parte de declarações de variáveis> <comando composto> */
AST *bloco(Parser *p) {
  printf(
      "<bloco> ::= <parte de declarações de variáveis> <comando composto>\n");

  AST *no_bloco = criar_no(AST_BLOCK);

  no_bloco->left = parteDeclaracoesVariaveis(p);
  no_bloco->right = comandoComposto(p);

  return no_bloco;
}

/* <parte de declarações de variáveis> ::= { var <declaração de variáveis> {;
 * <declaração de variáveis>}; } */
AST *parteDeclaracoesVariaveis(Parser *p) {
  if (p->token_corrente.type == TOKEN_VAR) {
    printf("<parte de declarações de variáveis> ::= var <declaração de "
           "variáveis> {; <declaração de variáveis>};\n");
    casaToken(p, TOKEN_VAR);

    AST *raiz_decl = declaracaoVariaveis(p);
    AST *atual = raiz_decl;

    casaToken(p, ';');

    while (p->token_corrente.type == TOKEN_IDENT) {
      AST *proxima_decl = declaracaoVariaveis(p);
      atual->next = proxima_decl;
      atual = proxima_decl;
      casaToken(p, ';');
    }

    return raiz_decl;
  }

  printf("<parte de declarações de variáveis> ::= vazio\n");
  return NULL;
}

/* <declaração de variáveis> ::=<lista de identificadores>: <tipo> */
AST *declaracaoVariaveis(Parser *p) {
  printf("<declaração de variáveis> ::= <lista de identificadores>: <tipo>\n");

  AST *no_decl = criar_no(AST_DECL);

  no_decl->left = listaIdentificadores(p);
  casaToken(p, ':');
  no_decl->right = tipo(p);

  return no_decl;
}

/* <lista de identificadores> ::= <identificador>{,<identificador> } */
AST *listaIdentificadores(Parser *p) {
  printf(
      "<lista de identificadores> ::= <identificador> {, <identificador>}\n");

  AST *raiz_var = criar_no(AST_VAR);
  strncpy(raiz_var->id_name, p->token_corrente.lexeme, 63);
  casaToken(p, TOKEN_IDENT);

  AST *atual = raiz_var;

  while (p->token_corrente.type == ',') {
    casaToken(p, ',');

    AST *prox_var = criar_no(AST_VAR);
    strncpy(prox_var->id_name, p->token_corrente.lexeme, 63);
    casaToken(p, TOKEN_IDENT);

    atual->next = prox_var;
    atual = prox_var;
  }

  return raiz_var;
}

/* <tipo> ::= integer | real */
AST *tipo(Parser *p) {
  AST *no_tipo = NULL;
  if (p->token_corrente.type == TOKEN_INTEGER) {
    printf("<tipo> ::= integer\n");
    no_tipo = criar_no(AST_NUM);
    casaToken(p, TOKEN_INTEGER);
  } else if (p->token_corrente.type == TOKEN_REAL) {
    printf("<tipo> ::= real\n");
    no_tipo = criar_no(AST_NUM);
    casaToken(p, TOKEN_REAL);
  } else {
    casaToken(p, TOKEN_INTEGER);
  }
  return no_tipo;
}


/* <comando> ::= <atribuição> | <comando composto> | <comando condicional> |
 * <comando repetitivo> | vazio */
AST *comando(Parser *p) {
  int tipo = p->token_corrente.type;

  if (tipo == TOKEN_IDENT) {
    return atribuicao(p);
  } else if (tipo == TOKEN_BEGIN) {
    return comandoComposto(p);
  } else if (tipo == TOKEN_IF) {
    return comandoCondicional(p);
  } else if (tipo == TOKEN_WHILE) {
    return comandoRepetitivo(p);
  }

  return NULL; /* Comando vazio */
}

/* <comando composto> ::= begin <comando> { ; <comando> } end */
AST *comandoComposto(Parser *p) {
  printf("<comando composto> ::= begin <comando> { ; <comando> } end\n");

  casaToken(p, TOKEN_BEGIN);

  AST *no_bloco = criar_no(AST_BLOCK);
  AST *cmd_inicial = comando(p);
  no_bloco->left = cmd_inicial;

  AST *atual = cmd_inicial;

  while (p->token_corrente.type == ';') {
    casaToken(p, ';');
    AST *prox_cmd = comando(p);

    if (atual) {
      atual->next = prox_cmd;
      atual = prox_cmd;
    } else {
      no_bloco->left = prox_cmd;
      atual = prox_cmd;
    }
  }

  casaToken(p, TOKEN_END);
  return no_bloco;
}

/* <atribuição> ::= <variável> := <expressão> */
AST *atribuicao(Parser *p) {
  printf("<atribuicao> ::= <variavel> := <expressao>\n");

  AST *no_assign = criar_no(AST_ASSIGN);

  no_assign->left = variavel(p);

  casaToken(p, TOKEN_ASSIGN);
  no_assign->right = expressao(p);

  return no_assign;
}

/* <comando condicional> ::= if <expressão> then <comando> [ else <comando> ] */
AST *comandoCondicional(Parser *p) {
  printf("<comando condicional> ::= if <expressao> then <comando> [ else "
         "<comando> ]\n");

  AST *no_if = criar_no(AST_IF);

  casaToken(p, TOKEN_IF);
  no_if->left = expressao(p);

  casaToken(p, TOKEN_THEN);

  AST *no_branches = criar_no(AST_BLOCK);
  no_branches->left = comando(p);

  if (p->token_corrente.type == TOKEN_ELSE) {
    casaToken(p, TOKEN_ELSE);
    no_branches->right = comando(p);
  }

  no_if->right = no_branches;

  return no_if;
}

/* <comando repetitivo> ::= while <expressão> do <comando> */
AST *comandoRepetitivo(Parser *p) {
  printf("<comando repetitivo> ::= while <expressao> do <comando>\n");

  AST *no_while = criar_no(AST_WHILE);

  casaToken(p, TOKEN_WHILE);
  no_while->left = expressao(p);

  casaToken(p, TOKEN_DO);
  no_while->right = comando(p);

  return no_while;
}

/* Protótipos adicionados para evitar erros de compilação */
AST *expressao(Parser *p);
AST *relacao(Parser *p);
AST *expressaoSimples(Parser *p);
AST *termo(Parser *p);
AST *fator(Parser *p);
AST *variavel(Parser *p);

/* <expressão> ::= <expressão simples> [ <relação> <expressão simples> ] */
AST *expressao(Parser *p) {
  printf(
      "<expressao> ::= <expressaoSimples> [ <relacao> <expressaoSimples> ]\n");

  AST *no = expressaoSimples(p);

  /* Verifica se o próximo token é um operador relacional */
  int t = p->token_corrente.type;
  if (t == '=' || t == TOKEN_NEQ || t == '<' || t == TOKEN_LEQ || t == '>' ||
      t == TOKEN_GEQ) {
    AST *no_rel = relacao(p); /* consome o operador e cria nó AST_REL */
    no_rel->left = no;        /* operando esquerdo já analisado        */
    no_rel->right = expressaoSimples(p); /* operando direito             */
    return no_rel;
  }

  return no;
}

// Lê o operador lógico e cria um AST_REL.
// O chamador que vai pendurar as expressões nos filhos left e right.
AST *relacao(Parser *p) {
  printf("<relacao> ::= = | <> | < | <= | > | >=\n");

  int op = p->token_corrente.type;
  AST *no_rel = criar_no(AST_REL);
  no_rel->num = (double)op; /* armazena o código do operador relacional */

  switch (op) {
  case '=':
    strcpy(no_rel->id_name, "=");
    casaToken(p, '=');
    break;
  case TOKEN_NEQ:
    strcpy(no_rel->id_name, "<>");
    casaToken(p, TOKEN_NEQ);
    break;
  case '<':
    strcpy(no_rel->id_name, "<");
    casaToken(p, '<');
    break;
  case TOKEN_LEQ:
    strcpy(no_rel->id_name, "<=");
    casaToken(p, TOKEN_LEQ);
    break;
  case '>':
    strcpy(no_rel->id_name, ">");
    casaToken(p, '>');
    break;
  case TOKEN_GEQ:
    strcpy(no_rel->id_name, ">=");
    casaToken(p, TOKEN_GEQ);
    break;
  default:
    fprintf(stderr, "%d:token nao esperado [%s].\n", p->token_corrente.line,
            p->token_corrente.lexeme);
    exit(1);
  }

  return no_rel;
}

/* <expressão simples> ::= [ + | - ] <termo> { ( + | - ) <termo> }
 * <operador aditivo>  ::= + | -
 *
 * O sinal unário opcional é representado como (0 - x) na AST,
 * o que permite avaliação e constant-folding sem nó especial.    */
AST *expressaoSimples(Parser *p) {
  /* Sinal unário opcional */
  int sinal = 0;
  if (p->token_corrente.type == '+') {
    sinal = '+';
    casaToken(p, '+');
  } else if (p->token_corrente.type == '-') {
    sinal = '-';
    casaToken(p, '-');
  }

  AST *no = termo(p);

  /* Aplica negação unária representando -x como (0 - x) */
  if (sinal == '-') {
    AST *no_neg = criar_no(AST_SUB);
    AST *zero = criar_no(AST_NUM);
    zero->num = 0.0;
    no_neg->left = zero;
    no_neg->right = no;
    no = no_neg;
  }

  /* Operadores aditivos binários */
  while (p->token_corrente.type == '+' || p->token_corrente.type == '-') {
    int op = p->token_corrente.type;
    casaToken(p, op);
    AST *novo = criar_no(op == '+' ? AST_ADD : AST_SUB);
    novo->left = no;
    novo->right = termo(p);
    no = novo;
  }
  return no;
}

/* <termo> ::= <fator> { ( * | / ) <fator> }
 * <operador multiplicativo> ::= * | /            */
AST *termo(Parser *p) {
  AST *no = fator(p);
  while (p->token_corrente.type == '*' || p->token_corrente.type == '/') {
    int op = p->token_corrente.type;
    casaToken(p, op);
    AST *novo = criar_no(op == '*' ? AST_MUL : AST_DIV);
    novo->left = no;
    novo->right = fator(p);
    no = novo;
  }
  return no;
}

/* <fator> ::= <variável> | <número> | ( <expressão> ) */
AST *fator(Parser *p) {
  /* Agora checa corretamente pelo token de número literal (NUM) */
  if (p->token_corrente.type == NUM) {
    AST *no = criar_no(AST_NUM);
    no->num = atof(p->token_corrente.lexeme);
    casaToken(p, NUM);
    return no;
  } else if (p->token_corrente.type == TOKEN_IDENT) {
    return variavel(p);
  } else if (p->token_corrente.type == '(') {
    casaToken(p, '(');
    AST *no = expressao(p);
    casaToken(p, ')');
    return no;
  }

  /* Se chegar aqui, força o erro pedindo o que era esperado */
  casaToken(p, NUM);
  return NULL;
}

AST *variavel(Parser *p) {
  AST *no = criar_no(AST_VAR);
  strncpy(no->id_name, p->token_corrente.lexeme, 63);
  casaToken(p, TOKEN_IDENT);
  return no;
}

/* FUNÇÕES PRINCIPAIS DE CONTROLE DA AST */
AST *parse_ast(Lexer *lex) {
  Parser p;
  p.lexer = lex;
  p.token_corrente = proxToken(lex); /* Corrigido para proxToken */

  AST *root = programa(&p);

  casaToken(&p, TOKEN_EOF);
  return root;
}

void ast_free(AST *t) {
  if (!t)
    return;
  ast_free(t->left);
  ast_free(t->right);
  ast_free(t->next);
  free(t);
}

void ast_print(const AST *t, int d) {
  if (!t)
    return;
  for (int i = 0; i < d; i++)
    fputs("  ", stdout);
  printf("No tipo: %d\n", t->kind);
  ast_print(t->left, d + 1);
  ast_print(t->right, d + 1);
  ast_print(t->next, d);
}