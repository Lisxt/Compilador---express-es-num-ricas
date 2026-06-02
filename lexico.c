#include "lexico.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void tv_init(TokenVec *v) {
  v->data = NULL;
  v->size = 0;
  v->cap = 0;
}

static void tv_reserve(TokenVec *v, size_t n) {
  if (n <= (size_t)v->cap)
    return;

  size_t cap = v->cap ? v->cap : 16;

  while (cap < n) {
    cap *= 2;
  }

  Token *p = realloc(v->data, cap * sizeof(Token));

  if (!p) {
    perror("realloc");
    exit(1);
  }

  v->data = p;
  v->cap = cap;
}

static void tv_push(TokenVec *v, Token t) {
  if (v->size + 1 > v->cap) {
    tv_reserve(v, v->size + 1);
  }

  v->data[v->size++] = t;
}

void tv_free(TokenVec *v) {
  free(v->data);

  v->data = NULL;
  v->size = 0;
  v->cap = 0;
}

static void skip_ws(Lexer *lex) {
  for (;;) {
    char c = lex->src[lex->pos];

    if (c == ' ' || c == '\t' || c == '\r') {
      lex->pos++;

    } else if (c == '\n') {
      lex->pos++;
      lex->line++;

    } else if (c == '{') {
      lex->pos++;
      while (lex->src[lex->pos] != '\0' && lex->src[lex->pos] != '}') {
        if (lex->src[lex->pos] == '\n')
          lex->line++;
        lex->pos++;
      }
      if (lex->src[lex->pos] == '}')
        lex->pos++;

    } else {
      break;
    }
  }
}

// Dicionário de palavras reservadas.
// A gente busca aqui pra saber se um identificador é uma variável ou uma
// palavra do MicroPascal (ex: "while").

typedef struct {
  const char *word;
  int type;
} Keyword;

static const Keyword keywords[] = {
    {"program", TOKEN_PROGRAM}, {"var", TOKEN_VAR},
    {"integer", TOKEN_INTEGER}, {"real", TOKEN_REAL},
    {"begin", TOKEN_BEGIN},     {"end", TOKEN_END},
    {"if", TOKEN_IF},           {"then", TOKEN_THEN},
    {"else", TOKEN_ELSE},       {"while", TOKEN_WHILE},
    {"do", TOKEN_DO},           {NULL, 0}};

// Ignora o que não importa: espaços, tabs, \n e comentários.
// Se esbarrar num \n (mesmo dentro de comentário), atualiza a contagem da linha
// para o erro sintático não ficar perdido.

// Motor do léxico. Lê os caracteres um a um e devolve o próximo token.
// Identifica se é palavra reservada, variável, número ou símbolo matemático.

static Token getToken(Lexer *lex) {
  Token tok;
  memset(&tok, 0, sizeof(tok));

  skip_ws(lex);

  tok.line = lex->line;

  char c = lex->src[lex->pos];

  /* fim de arquivo */
  if (c == '\0') {
    tok.type = TOKEN_EOF;
    strcpy(tok.lexeme, "EOF");
    return tok;
  }

  if (isalpha((unsigned char)c) || c == '_') {
    int i = 0;
    while (isalnum((unsigned char)lex->src[lex->pos]) ||
           lex->src[lex->pos] == '_') {
      if (i < 63)
        tok.lexeme[i++] = lex->src[lex->pos];
      lex->pos++;
    }
    tok.lexeme[i] = '\0';

    tok.type = TOKEN_IDENT;
    for (int k = 0; keywords[k].word; k++) {
      if (strcmp(tok.lexeme, keywords[k].word) == 0) {
        tok.type = keywords[k].type;
        break;
      }
    }
    return tok;
  }

  if (isdigit((unsigned char)c)) {
    unsigned long long acc = 0;
    int i = 0;

    while (isdigit((unsigned char)lex->src[lex->pos])) {
      acc = acc * 10u + (lex->src[lex->pos] - '0');
      if (i < 63)
        tok.lexeme[i++] = lex->src[lex->pos];
      lex->pos++;
    }

    if (lex->src[lex->pos] == '.') {
      if (i < 63)
        tok.lexeme[i++] = lex->src[lex->pos++];
      while (isdigit((unsigned char)lex->src[lex->pos])) {
        if (i < 63)
          tok.lexeme[i++] = lex->src[lex->pos];
        lex->pos++;
      }
    }

    tok.lexeme[i] = '\0';
    tok.type = NUM;
    tok.value = (double)acc;
    return tok;
  }

  tok.lexeme[0] = c;
  tok.lexeme[1] = '\0';
  lex->pos++;

  switch (c) {
  case '+':
    tok.type = PLUS;
    break;
  case '-':
    tok.type = MINUS;
    break;
  case '*':
    tok.type = MULT;
    break;
  case '/':
    tok.type = DIV;
    break;
  case '(':
    tok.type = LPAREN;
    break;
  case ')':
    tok.type = RPAREN;
    break;

  case ':':
    if (lex->src[lex->pos] == '=') {
      tok.lexeme[1] = lex->src[lex->pos++];
      tok.lexeme[2] = '\0';
      tok.type = TOKEN_ASSIGN;
    } else {
      tok.type = ':';
    }
    break;

  case '<':
    if (lex->src[lex->pos] == '>') {
      tok.lexeme[1] = lex->src[lex->pos++];
      tok.lexeme[2] = '\0';
      tok.type = TOKEN_NEQ;
    } else if (lex->src[lex->pos] == '=') {
      tok.lexeme[1] = lex->src[lex->pos++];
      tok.lexeme[2] = '\0';
      tok.type = TOKEN_LEQ;
    } else {
      tok.type = '<';
    }
    break;

  case '>':
    if (lex->src[lex->pos] == '=') {
      tok.lexeme[1] = lex->src[lex->pos++];
      tok.lexeme[2] = '\0';
      tok.type = TOKEN_GEQ;
    } else {
      tok.type = '>';
    }
    break;

  case '=':
    tok.type = '=';
    break;
  case ',':
    tok.type = ',';
    break;
  case ';':
    tok.type = ';';
    break;
  case '.':
    tok.type = '.';
    break;

  default:
    fprintf(stderr, "Erro lexico: caractere inesperado '%c'\n", c);
    exit(1);
  }

  tok.value = 0;
  return tok;
}

TokenVec tokenize_to_vector(const char *src) {
  Lexer lex;
  lexer_init(&lex, src);

  TokenVec v;
  tv_init(&v);

  for (;;) {
    Token t = getToken(&lex);
    tv_push(&v, t);

    if (t.type == END || t.type == TOKEN_EOF) {
      break;
    }
  }

  return v;
}

// token_name — retorna o nome textual de um tipo de token
// Usada para montar mensagens de erro e saídas de depuração.

const char *token_name(int t) {
  switch (t) {
  case NUM:
    return "NUM";
  case PLUS:
    return "PLUS";
  case MINUS:
    return "MINUS";
  case MULT:
    return "MULT";
  case DIV:
    return "DIV";
  case LPAREN:
    return "LPAREN";
  case RPAREN:
    return "RPAREN";
  case END:
    return "END";
  case TOKEN_IDENT:
    return "IDENT";
  case TOKEN_PROGRAM:
    return "program";
  case TOKEN_VAR:
    return "var";
  case TOKEN_INTEGER:
    return "integer";
  case TOKEN_REAL:
    return "real";
  case TOKEN_BEGIN:
    return "begin";
  case TOKEN_END:
    return "end";
  case TOKEN_IF:
    return "if";
  case TOKEN_THEN:
    return "then";
  case TOKEN_ELSE:
    return "else";
  case TOKEN_WHILE:
    return "while";
  case TOKEN_DO:
    return "do";
  case TOKEN_ASSIGN:
    return ":=";
  case TOKEN_NEQ:
    return "<>";
  case TOKEN_LEQ:
    return "<=";
  case TOKEN_GEQ:
    return ">=";
  case TOKEN_EOF:
    return "EOF";
  case ':':
    return ":";
  case ';':
    return ";";
  case ',':
    return ",";
  case '.':
    return ".";
  case '=':
    return "=";
  case '<':
    return "<";
  case '>':
    return ">";

  default:
    return "UNKNOWN";
  }
}

void lexer_init(Lexer *lex, const char *src) {
  lex->src = src;
  lex->pos = 0;
  lex->line = 1;
}

Token proxToken(Lexer *lex) { return getToken(lex); }