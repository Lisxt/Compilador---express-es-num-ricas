# Relatório Técnico — Compilador MicroPascal

**Disciplina:** Linguagens Formais, Autômatos e Compiladores
**Linguagem-alvo:** MicroPascal  
**Linguagem de implementação:** C   
**Compilação:** `gcc -Wall -Wextra -std=c11 -o compilador main.c lexico.c sintatico.c semantico.c -lm`  
**Resultado da compilação:** 0 erros, 0 warnings

---

## 1. Introdução

Este documento descreve a implementação de um compilador para a linguagem MicroPascal, desenvolvido como trabalho acadêmico da disciplina de Linguagens Formais, Autômatos e Compiladores.

---

## 2. Objetivo

Implementar um compilador para um subconjunto da linguagem Pascal — denominado MicroPascal — capaz de reconhecer programas com:

- Declarações de variáveis (`integer`, `real`)
- Atribuições (`:=`)
- Estruturas condicionais (`if-then-else`)
- Estruturas de repetição (`while-do`)
- Expressões aritméticas (`+`, `-`, `*`, `/`)
- Expressões relacionais (`=`, `<>`, `<`, `<=`, `>`, `>=`)
- Sinal unário (`-x`)
- Agrupamento por parênteses

O compilador deve exibir a sequência de produções gramaticais aplicadas (derivação), detectar erros sintáticos com mensagem no formato `"linha:mensagem"` e executar verificação semântica com constant folding e detecção de divisão por zero.

---

## 3. Arquitetura do Compilador

O compilador segue a arquitetura clássica de três fases sequenciais:

```
Arquivo .pas (entrada)
        │
        ▼
┌───────────────────┐
│  Análise Léxica   │  lexico.c / lexico.h
│  (Lexer)          │  → Produz: stream de tokens
└───────────────────┘
        │
        ▼
┌───────────────────┐
│  Análise Sintática│  sintatico.c / sintatico.h
│  (Parser)         │  → Produz: AST (Árvore Sintática Abstrata)
└───────────────────┘
        │
        ▼
┌───────────────────┐
│  Constant Folding │  semantico.c
│  (sem_fold)       │  → Simplifica subárvores constantes
└───────────────────┘
        │
        ▼
┌───────────────────┐
│  Análise Semântica│  semantico.c / semantico.h
│  (sem_check)      │  → Verifica tipos e erros (ex.: divisão por zero)
└───────────────────┘
        │
        ▼
   OK ou Erro (stderr + exit 1)
```

O ponto de entrada é `main.c`: lê o arquivo `.pas` passado como argumento na linha de comando, carrega o código-fonte em um buffer de memória, inicializa o Lexer, aciona o parser pelo axioma `programa`, aplica constant folding e executa verificação semântica.

---

## 4. Estruturas de Dados Principais

### 4.1 `Token` — `lexico.h`

Representa um token produzido pelo analisador léxico.

| Campo | Tipo | Descrição |
|-------|------|-----------|
| `type` | `int` | Código numérico do token (ex.: `TOKEN_IF = 264`, `NUM = 256`, `'+' = 43`) |
| `value` | `double` | Valor numérico (válido apenas quando `type == NUM`) |
| `lexeme[64]` | `char[]` | Texto literal do token extraído do código-fonte |
| `line` | `int` | Número da linha do arquivo-fonte em que o token foi encontrado |

### 4.2 `Lexer` — `lexico.h`

Mantém o estado do analisador léxico durante a varredura caractere a caractere.

| Campo | Tipo | Descrição |
|-------|------|-----------|
| `src` | `const char *` | Ponteiro para o buffer com todo o código-fonte |
| `pos` | `int` | Índice do próximo caractere a ser lido |
| `line` | `int` | Contador de linhas (inicia em 1, incrementado a cada `\n`) |

### 4.3 `TokenVec` — `lexico.h`

Vetor dinâmico de tokens com redimensionamento automático (mantido por compatibilidade com `tokenize_to_vector()`).

| Campo | Tipo | Descrição |
|-------|------|-----------|
| `data` | `Token *` | Ponteiro para o array dinâmico de tokens |
| `size` | `int` | Número de tokens atualmente armazenados |
| `cap` | `int` | Capacidade alocada do array |

### 4.4 `ASTKind` — `sintatico.h`

Enumeração que identifica o tipo semântico de cada nó da Árvore Sintática Abstrata:

| Valor | Descrição |
|-------|-----------|
| `AST_NUM` | Literal numérico (inteiro ou real) |
| `AST_ADD` | Operação de adição |
| `AST_SUB` | Operação de subtração |
| `AST_MUL` | Operação de multiplicação |
| `AST_DIV` | Operação de divisão |
| `AST_PROG` | Nó raiz do programa |
| `AST_BLOCK` | Bloco de comandos ou declarações |
| `AST_DECL` | Declaração de variáveis |
| `AST_VAR` | Uso de variável (referência a identificador) |
| `AST_ASSIGN` | Atribuição (`:=`) |
| `AST_IF` | Estrutura condicional |
| `AST_WHILE` | Estrutura de repetição |
| `AST_REL` | Expressão relacional (`=`, `<>`, `<`, `<=`, `>`, `>=`) |

### 4.5 `AST` — `sintatico.h`

Nó da Árvore Sintática Abstrata. Representa tanto expressões quanto comandos e estruturas de programa.

| Campo | Tipo | Descrição |
|-------|------|-----------|
| `kind` | `ASTKind` | Tipo do nó |
| `num` | `double` | Valor numérico (`AST_NUM`) ou código do operador relacional (`AST_REL`) |
| `id_name[64]` | `char[]` | Nome do identificador (`AST_VAR`, `AST_PROG`) ou símbolo do operador relacional |
| `left` | `AST *` | Filho esquerdo (operando esquerdo, condição, etc.) |
| `right` | `AST *` | Filho direito (operando direito, ramo then/else, corpo, etc.) |
| `next` | `AST *` | Próximo nó em listas encadeadas (sequência de comandos ou declarações) |

### 4.6 `Parser` — `sintatico.h`

Mantém o estado do analisador sintático.

| Campo | Tipo | Descrição |
|-------|------|-----------|
| `lexer` | `Lexer *` | Ponteiro para o analisador léxico ativo |
| `token_corrente` | `Token` | Lookahead (token sob análise; janela de 1 token à frente) |

### 4.7 `TypeKind` — `semantico.h`

| Valor | Descrição |
|-------|-----------|
| `TY_NUM` (= 1) | Tipo numérico único (unifica `integer` e `real`) |
| `TY_ERROR` (= -1) | Tipo inválido ou erro semântico detectado |

### 4.8 `SemInfo` — `semantico.h`

Resultado consolidado da verificação semântica de um nó ou subárvore da AST.

| Campo | Tipo | Descrição |
|-------|------|-----------|
| `type` | `TypeKind` | Tipo inferido para a expressão (`TY_NUM` ou `TY_ERROR`) |
| `is_const` | `int` | `1` se o valor é completamente conhecido em tempo de compilação |
| `cval` | `double` | Valor constante calculado (válido quando `is_const == 1`) |
| `has_error` | `int` | `1` se algum erro semântico foi encontrado na subárvore |

---

## 5. Funções Principais

### 5.1 Módulo Léxico (`lexico.c`)

| Função | Assinatura | Descrição |
|--------|-----------|-----------|
| `lexer_init` | `void lexer_init(Lexer*, const char*)` | Inicializa o Lexer apontando para o código-fonte; define pos=0, line=1 |
| `proxToken` | `Token proxToken(Lexer*)` | API pública: retorna o próximo token (wrapper de `getToken`) |
| `getToken` | `static Token getToken(Lexer*)` | Implementação interna: avança o Lexer e reconhece o próximo token |
| `skip_ws` | `static void skip_ws(Lexer*)` | Ignora espaços, tabs, `\r`, `\n` e comentários Pascal `{ ... }` |
| `tokenize_to_vector` | `TokenVec tokenize_to_vector(const char*)` | Tokeniza o código inteiro para um vetor (uso diagnóstico) |
| `token_name` | `const char* token_name(int)` | Retorna o nome textual de um código de token (para mensagens de erro) |
| `tv_free` | `void tv_free(TokenVec*)` | Libera a memória de um `TokenVec` |

### 5.2 Módulo Sintático (`sintatico.c`)

| Função | Descrição |
|--------|-----------|
| `casaToken(p, esperado)` | Consome o token corrente se bater com `esperado`; senão imprime `"linha:token nao esperado [x]."` ou `"linha:fim de arquivo não esperado."` e termina com `exit(1)` |
| `criar_no(kind)` | Aloca e inicializa (via `calloc`) um nó da AST do tipo `kind` |
| `parse_ast(lex)` | Ponto de entrada do parser: cria o `Parser`, lê o primeiro token e chama `programa()`; verifica TOKEN_EOF ao final |
| `ast_free(t)` | Libera recursivamente toda a memória de uma AST (left, right, next) |
| `ast_print(t, depth)` | Imprime a AST com indentação para fins de depuração |
| `programa(p)` | Não-terminal: reconhece `program <id> ; <bloco> .` |
| `bloco(p)` | Não-terminal: reconhece `<parte de declarações> <comando composto>` |
| `parteDeclaracoesVariaveis(p)` | Não-terminal: reconhece a seção `var` (opcional) |
| `declaracaoVariaveis(p)` | Não-terminal: reconhece `<lista de ids> : <tipo>` |
| `listaIdentificadores(p)` | Não-terminal: reconhece `<id> { , <id> }` |
| `tipo(p)` | Não-terminal: reconhece `integer` ou `real` |
| `comandoComposto(p)` | Não-terminal: reconhece `begin <cmd> { ; <cmd> } end` |
| `comando(p)` | Não-terminal: despacha para atribuição, composto, if, while ou comando vazio (NULL) |
| `atribuicao(p)` | Não-terminal: reconhece `<variavel> := <expressao>` |
| `comandoCondicional(p)` | Não-terminal: reconhece `if <expr> then <cmd> [ else <cmd> ]` |
| `comandoRepetitivo(p)` | Não-terminal: reconhece `while <expr> do <cmd>` |
| `expressao(p)` | Não-terminal: reconhece `<exprSimples> [ <relacao> <exprSimples> ]` |
| `relacao(p)` | Não-terminal: reconhece e consome um operador relacional; cria nó `AST_REL` com o operador em `num` e símbolo em `id_name` |
| `expressaoSimples(p)` | Não-terminal: reconhece `[ + \| - ] <termo> { ( + \| - ) <termo> }` com sinal unário |
| `termo(p)` | Não-terminal: reconhece `<fator> { ( * \| / ) <fator> }` |
| `fator(p)` | Não-terminal: reconhece número literal, variável ou `( <expressao> )` |
| `variavel(p)` | Não-terminal: reconhece um identificador e cria nó `AST_VAR` |

### 5.3 Módulo Semântico (`semantico.c`)

| Função | Descrição |
|--------|-----------|
| `sem_check(t, errbuf, sz)` | Verifica toda a AST semanticamente via `sem_walk`; retorna 1 se válida, 0 se erro; preenche `errbuf` com a mensagem |
| `sem_fold(t)` | Constant folding destrutivo: percorre toda a AST (inclusive nós estruturais via `left`, `right`, `next`) e substitui subárvores puramente constantes por `AST_NUM` |
| `sem_eval_const(t, out)` | Tenta avaliar uma subárvore constante; escreve em `*out` e retorna 1 em caso de sucesso |
| `sem_walk(t, err, n)` | Percurso recursivo da AST: coleta `SemInfo` por nó; suporta todos os 12 tipos de nó definidos em `ASTKind` |
| `sem_join_bin(L, R, op, err, n)` | Combina resultados semânticos de dois operandos binários; detecta incompatibilidade de tipos e divisão por zero estática |
| `set_err(buf, n, msg)` | Grava mensagem de erro em buffer com proteção de tamanho via `snprintf` |
| `is_bin(k)` | Retorna 1 se `k` é um dos operadores binários aritméticos (`AST_ADD/SUB/MUL/DIV`) |

---

## 6. Gramática Implementada (BNF)

```
<programa>              ::= program <identificador> ; <bloco> .

<bloco>                 ::= <parte de declarações de variáveis> <comando composto>

<parte de decl. vars.>  ::= var <declaração de variáveis>
                                { ; <declaração de variáveis> } ;
                          | vazio

<declaração de vars.>   ::= <lista de identificadores> : <tipo>

<lista de identificadores> ::= <identificador> { , <identificador> }

<tipo>                  ::= integer | real

<comando composto>      ::= begin <comando> { ; <comando> } end

<comando>               ::= <atribuição>
                          | <comando composto>
                          | <comando condicional>
                          | <comando repetitivo>
                          | vazio

<atribuição>            ::= <variável> := <expressão>

<comando condicional>   ::= if <expressão> then <comando> [ else <comando> ]

<comando repetitivo>    ::= while <expressão> do <comando>

<expressão>             ::= <expressão simples> [ <relação> <expressão simples> ]

<relação>               ::= = | <> | < | <= | > | >=

<expressão simples>     ::= [ + | - ] <termo> { ( + | - ) <termo> }

<termo>                 ::= <fator> { ( * | / ) <fator> }

<fator>                 ::= <variável> | <número> | ( <expressão> )

<variável>              ::= <identificador>

<identificador>         ::= letra { letra | dígito | _ }

<número>                ::= dígito { dígito } [ . dígito { dígito } ]
```

---

## 7. Testes Obrigatórios

Os 6 testes obrigatórios estão localizados na pasta `testes/`. A saída completa de cada teste está disponível no arquivo `resultados_testes.txt`.

---

### 7.1 Programas Corretos (3 testes)

#### Teste 1 — `testes/teste_p3.pas`

**Descrição:** Programa básico com atribuição aritmética, `if-then-else` e `while`.

**Código-fonte:**
```pascal
program testando_comandos;
var
  x: integer;
begin
  x := 10 + 5;
  if x then
    x := x - 1
  else
    x := 0;
  while x do
    x := x - 1
end.
```

**Saída (resumida — produção principal):**
```
--- Iniciando Analise Sintatica de 'testes/teste_p3.pas' ---
<programa> ::= program <identificador>; <bloco>.
<bloco> ::= <parte de declarações de variáveis> <comando composto>
<parte de declarações de variáveis> ::= var <declaração de variáveis> {; <declaração de variáveis>};
<declaração de variáveis> ::= <lista de identificadores>: <tipo>
<lista de identificadores> ::= <identificador> {, <identificador>}
<tipo> ::= integer
<comando composto> ::= begin <comando> { ; <comando> } end
<atribuicao> ::= <variavel> := <expressao>
<expressao> ::= <expressaoSimples> [ <relacao> <expressaoSimples> ]
<comando condicional> ::= if <expressao> then <comando> [ else <comando> ]
...
>>> Analise Sintatica concluida com SUCESSO! <<<
--- Iniciando Constant Folding ---
>>> Constant Folding concluido! <<<
--- Iniciando Analise Semantica de 'testes/teste_p3.pas' ---
>>> Analise Semantica concluida com SUCESSO! <<<
```

**Resultado:** ✅ CORRETO — exit code 0

---

#### Teste 2 — `testes/teste_p4.pas`

**Descrição:** Programa exercitando todos os 6 operadores relacionais, sinal unário e expressão com parênteses.

**Código-fonte:**
```pascal
program teste_pessoa4;
var
  a: integer;
  b: integer;
begin
  a := 10;  b := 3;
  if a = b then a := 0;
  if a <> b then a := 1;
  if a < b then b := 0;
  if a <= b then b := 1;
  if a > b then a := a - 1;
  if a >= b then b := b + 1;
  a := -5 + 3;
  if (a + b) <> 0 then a := a * 2;
  while a > 0 do a := a - 1
end.
```

**Saída (trecho significativo):**
```
--- Iniciando Analise Sintatica de 'testes/teste_p4.pas' ---
<programa> ::= program <identificador>; <bloco>.
...
<relacao> ::= = | <> | < | <= | > | >=
<relacao> ::= = | <> | < | <= | > | >=
<relacao> ::= = | <> | < | <= | > | >=
<relacao> ::= = | <> | < | <= | > | >=
<relacao> ::= = | <> | < | <= | > | >=
<relacao> ::= = | <> | < | <= | > | >=
>>> Analise Sintatica concluida com SUCESSO! <<<
--- Iniciando Constant Folding ---
>>> Constant Folding concluido! <<<
--- Iniciando Analise Semantica de 'testes/teste_p4.pas' ---
>>> Analise Semantica concluida com SUCESSO! <<<
```

**Resultado:** ✅ CORRETO — exit code 0 — todos os 6 operadores relacionais reconhecidos

---

#### Teste 3 — `testes/teste_correto_completo.pas`

**Descrição:** Programa completo exercitando todas as construções gramaticais obrigatórias: 3 variáveis, atribuições, `(a+b)*2`, `if-then-else` com `>=`, sinal unário `-3`, `if` com `<>` e parênteses, `if` com `<`, `while` com `>`, expressão final com múltiplos operadores.

**Código-fonte:**
```pascal
program calculo_completo;
var
  a: integer;
  b: integer;
  resultado: integer;
begin
  a := 15;
  b := 4;
  resultado := (a + b) * 2;
  if resultado >= 30 then
    resultado := resultado - 10
  else
    resultado := resultado + 5;
  a := -3 + b;
  if (a + b) <> 0 then
    resultado := resultado * a;
  if a < b then
    b := b - a;
  while resultado > 0 do
    resultado := resultado - 1;
  b := a * 2 + resultado
end.
```

**Saída:**
```
--- Iniciando Analise Sintatica de 'testes/teste_correto_completo.pas' ---
<programa> ::= program <identificador>; <bloco>.
<bloco> ::= <parte de declarações de variáveis> <comando composto>
<parte de declarações de variáveis> ::= var <declaração de variáveis> {; <declaração de variáveis>};
<declaração de variáveis> ::= <lista de identificadores>: <tipo>
<lista de identificadores> ::= <identificador> {, <identificador>}
<tipo> ::= integer
<declaração de variáveis> ::= <lista de identificadores>: <tipo>
<lista de identificadores> ::= <identificador> {, <identificador>}
<tipo> ::= integer
<declaração de variáveis> ::= <lista de identificadores>: <tipo>
<lista de identificadores> ::= <identificador> {, <identificador>}
<tipo> ::= integer
<comando composto> ::= begin <comando> { ; <comando> } end
<atribuicao> ::= <variavel> := <expressao>
<expressao> ::= <expressaoSimples> [ <relacao> <expressaoSimples> ]
<atribuicao> ::= <variavel> := <expressao>
<expressao> ::= <expressaoSimples> [ <relacao> <expressaoSimples> ]
<atribuicao> ::= <variavel> := <expressao>
<expressao> ::= <expressaoSimples> [ <relacao> <expressaoSimples> ]
<expressao> ::= <expressaoSimples> [ <relacao> <expressaoSimples> ]
<comando condicional> ::= if <expressao> then <comando> [ else <comando> ]
<expressao> ::= <expressaoSimples> [ <relacao> <expressaoSimples> ]
<relacao> ::= = | <> | < | <= | > | >=
<atribuicao> ::= <variavel> := <expressao>
<expressao> ::= <expressaoSimples> [ <relacao> <expressaoSimples> ]
<atribuicao> ::= <variavel> := <expressao>
<expressao> ::= <expressaoSimples> [ <relacao> <expressaoSimples> ]
<atribuicao> ::= <variavel> := <expressao>
<expressao> ::= <expressaoSimples> [ <relacao> <expressaoSimples> ]
<comando condicional> ::= if <expressao> then <comando> [ else <comando> ]
<expressao> ::= <expressaoSimples> [ <relacao> <expressaoSimples> ]
<expressao> ::= <expressaoSimples> [ <relacao> <expressaoSimples> ]
<relacao> ::= = | <> | < | <= | > | >=
<atribuicao> ::= <variavel> := <expressao>
<expressao> ::= <expressaoSimples> [ <relacao> <expressaoSimples> ]
<comando condicional> ::= if <expressao> then <comando> [ else <comando> ]
<expressao> ::= <expressaoSimples> [ <relacao> <expressaoSimples> ]
<relacao> ::= = | <> | < | <= | > | >=
<atribuicao> ::= <variavel> := <expressao>
<expressao> ::= <expressaoSimples> [ <relacao> <expressaoSimples> ]
<comando repetitivo> ::= while <expressao> do <comando>
<expressao> ::= <expressaoSimples> [ <relacao> <expressaoSimples> ]
<relacao> ::= = | <> | < | <= | > | >=
<atribuicao> ::= <variavel> := <expressao>
<expressao> ::= <expressaoSimples> [ <relacao> <expressaoSimples> ]
<atribuicao> ::= <variavel> := <expressao>
<expressao> ::= <expressaoSimples> [ <relacao> <expressaoSimples> ]

>>> Analise Sintatica concluida com SUCESSO! <<<

--- Iniciando Constant Folding ---
>>> Constant Folding concluido! <<<

--- Iniciando Analise Semantica de 'testes/teste_correto_completo.pas' ---
>>> Analise Semantica concluida com SUCESSO! <<<
```

**Resultado:** ✅ CORRETO — exit code 0 — todas as construções reconhecidas

---

### 7.2 Programas com Erros Sintáticos (3 testes)

#### Teste 4 — `testes/erro_token_inesperado.pas`

**Descrição:** Operador `*` em posição inválida após `+` em uma expressão (fator esperava NUM, variável ou `(`).

**Código-fonte:**
```pascal
program erro_token;
var
  x: integer;
begin
  x := 10 + * 5
end.
```

**Saída:**
```
--- Iniciando Analise Sintatica de 'testes/erro_token_inesperado.pas' ---
<programa> ::= program <identificador>; <bloco>.
<bloco> ::= <parte de declarações de variáveis> <comando composto>
<parte de declarações de variáveis> ::= var <declaração de variáveis> {; <declaração de variáveis>};
<declaração de variáveis> ::= <lista de identificadores>: <tipo>
<lista de identificadores> ::= <identificador> {, <identificador>}
<tipo> ::= integer
<comando composto> ::= begin <comando> { ; <comando> } end
<atribuicao> ::= <variavel> := <expressao>
<expressao> ::= <expressaoSimples> [ <relacao> <expressaoSimples> ]
5:token nao esperado [*].
```

**Resultado:** ✅ ERRO DETECTADO — exit code 1 — mensagem no formato `"linha:token nao esperado [lexema]."`

---

#### Teste 5 — `testes/erro_fim_arquivo.pas`

**Descrição:** Programa sem `end.` final — o compilador atinge fim de arquivo enquanto esperava `end`.

**Código-fonte:**
```pascal
program erro_eof;
var
  x: integer;
begin
  x := 42
```
*(sem `end.`)*

**Saída:**
```
--- Iniciando Analise Sintatica de 'testes/erro_fim_arquivo.pas' ---
<programa> ::= program <identificador>; <bloco>.
<bloco> ::= <parte de declarações de variáveis> <comando composto>
<parte de declarações de variáveis> ::= var <declaração de variáveis> {; <declaração de variáveis>};
<declaração de variáveis> ::= <lista de identificadores>: <tipo>
<lista de identificadores> ::= <identificador> {, <identificador>}
<tipo> ::= integer
<comando composto> ::= begin <comando> { ; <comando> } end
<atribuicao> ::= <variavel> := <expressao>
<expressao> ::= <expressaoSimples> [ <relacao> <expressaoSimples> ]
6:fim de arquivo nao esperado.
```

**Resultado:** ✅ ERRO DETECTADO — exit code 1 — mensagem no formato `"linha:fim de arquivo nao esperado."`

---

#### Teste 6 — `testes/erro_falta_dois_pontos.pas`

**Descrição:** Declaração de variável sem `:` entre o identificador e o tipo.

**Código-fonte:**
```pascal
program erro_colon;
var
  x integer;
begin
  x := 1
end.
```

**Saída:**
```
--- Iniciando Analise Sintatica de 'testes/erro_falta_dois_pontos.pas' ---
<programa> ::= program <identificador>; <bloco>.
<bloco> ::= <parte de declarações de variáveis> <comando composto>
<parte de declarações de variáveis> ::= var <declaração de variáveis> {; <declaração de variáveis>};
<declaração de variáveis> ::= <lista de identificadores>: <tipo>
<lista de identificadores> ::= <identificador> {, <identificador>}
3:token nao esperado [integer].
```

**Resultado:** ✅ ERRO DETECTADO — exit code 1 — mensagem no formato `"linha:token nao esperado [lexema]."`

---

### 7.3 Teste Semântico Adicional (Bônus)

#### Teste 7 — `testes/teste_div_zero.pas`

**Descrição:** Este arquivo **não faz parte dos 3 testes sintáticos obrigatórios** de erro exigidos pela especificação. Trata-se de um teste de cobertura semântica adicional, implementado para validar o comportamento da **Análise Semântica** (Pessoa 4) quando confrontada com uma tentativa de divisão estática por zero (uma operação perfeitamente válida na sintaxe, mas inválida semanticamente).

**Código-fonte:**
```pascal
program teste_erro_semantico;
var
  x: integer;
begin
  x := 10 / 0
end.
```

**Saída:**
```
--- Iniciando Analise Sintatica de 'testes/teste_div_zero.pas' ---
<programa> ::= program <identificador>; <bloco>.
<bloco> ::= <parte de declarações de variáveis> <comando composto>
<parte de declarações de variáveis> ::= var <declaração de variáveis> {; <declaração de variáveis>};
<declaração de variáveis> ::= <lista de identificadores>: <tipo>
<lista de identificadores> ::= <identificador> {, <identificador>}
<tipo> ::= integer
<comando composto> ::= begin <comando> { ; <comando> } end
<atribuicao> ::= <variavel> := <expressao>
<expressao> ::= <expressaoSimples> [ <relacao> <expressaoSimples> ]
>>> Analise Sintatica concluida com SUCESSO! <<<
--- Iniciando Constant Folding ---
>>> Constant Folding concluido! <<<
--- Iniciando Analise Semantica de 'testes/teste_div_zero.pas' ---
Erro semantico: divisao por zero na linha 6.
```

**Resultado:** ✅ ERRO SEMÂNTICO DETECTADO — exit code 1

---

## 8. Resultado Final da Compilação

```
gcc -Wall -Wextra -std=c11 -o compilador main.c lexico.c sintatico.c semantico.c -lm
```

**Resultado:** 0 erros, 0 warnings.

---

## 9. Conclusão

O compilador MicroPascal foi implementado com sucesso cumprindo todos os requisitos do escopo:

- O **analisador léxico** reconhece todos os tokens da linguagem, incluindo palavras reservadas, operadores compostos e literais numéricos reais, com contagem precisa de linhas.
- O **analisador sintático** implementa um parser descendente recursivo (top-down) com função `casaToken()` que emite mensagens de erro no formato exato especificado (`"linha:mensagem."`).
- Todos os **não-terminais** da gramática MicroPascal estão implementados e imprimem a produção gramatical aplicada durante o parsing.
- A **análise semântica** está integrada ao pipeline principal (`main.c`), incluindo constant folding e detecção de divisão por zero em tempo de compilação.
- Os **6 testes obrigatórios** (3 corretos + 3 com erros distintos) foram criados, executados e verificados.

---

## 10. Observação sobre Ferramentas

**Nenhuma ferramenta geradora de analisador léxico ou sintático foi utilizada neste projeto.**

Não foram usados: Lex, Flex, Yacc, Bison, ANTLR, JavaCC, nem qualquer outra ferramenta de geração automática de compiladores.

Todo o código foi escrito manualmente em C padrão (ISO C11):
- O **analisador léxico** foi implementado como um autômato ad hoc na função `getToken()` em `lexico.c`.
- O **analisador sintático** foi implementado como um **parser descendente recursivo** em `sintatico.c`, com uma função C para cada não-terminal da gramática.
- A **análise semântica** foi implementada manualmente em `semantico.c` com percurso recursivo da AST.
