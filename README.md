# Compilador MicroPascal

Projeto desenvolvido para a disciplina de **Linguagens Formais, Autômatos e Compiladores**.

O objetivo deste trabalho é implementar um compilador para uma linguagem baseada em **MicroPascal**, contemplando as etapas de análise léxica, análise sintática e análise semântica utilizando **Parser Descendente Recursivo (Recursive Descent Parser)**.

---

## Funcionalidades

### Análise Léxica

Reconhecimento de:

- Identificadores
- Números inteiros
- Números reais
- Palavras reservadas
- Operadores aritméticos
- Operadores relacionais
- Delimitadores
- Comentários

### Palavras Reservadas

```text
program
var
integer
real
begin
end
if
then
else
while
do
```

### Operadores Aritméticos

```text
+
-
*
/
```

### Operadores Relacionais

```text
=
<>
<
<=
>
>=
```

---

## Análise Sintática

O parser foi implementado manualmente utilizando a técnica de **Recursive Descent Parsing**.

Os seguintes não-terminais da gramática foram implementados:

```text
programa
bloco
parteDeclaracoesVariaveis
declaracaoVariaveis
listaIdentificadores
tipo
comando
comandoComposto
atribuicao
comandoCondicional
comandoRepetitivo
expressao
expressaoSimples
termo
fator
relacao
variavel
```

Durante a análise sintática o compilador exibe as produções utilizadas e constrói uma Árvore Sintática Abstrata (AST).

---

## Árvore Sintática Abstrata (AST)

Os seguintes tipos de nós são utilizados:

```text
AST_PROG
AST_BLOCK
AST_DECL
AST_VAR
AST_ASSIGN
AST_IF
AST_WHILE
AST_REL
AST_ADD
AST_SUB
AST_MUL
AST_DIV
AST_NUM
```

---

## Análise Semântica

O projeto possui uma etapa de análise semântica responsável por:

- Verificação de expressões numéricas;
- Detecção de divisão por zero em tempo de compilação;
- Constant Folding;
- Percurso completo da AST.

### Exemplo de Constant Folding

Entrada:

```pascal
x := 2 + 3 * 4;
```

Após otimização:

```pascal
x := 14;
```

---

## Estrutura do Projeto

```text
.
├── main.c
├── lexico.c
├── lexico.h
├── sintatico.c
├── sintatico.h
├── semantico.c
├── semantico.h
├── README.md
├── relatorio_tecnico.md
├── resultados_testes.txt
└── testes
    ├── teste_p3.pas
    ├── teste_p4.pas
    ├── teste_correto_completo.pas
    ├── erro_token_inesperado.pas
    ├── erro_fim_arquivo.pas
    ├── erro_falta_dois_pontos.pas
    └── teste_div_zero.pas
```

---

## Requisitos

- GCC compatível com C11
- Linux, macOS ou Windows

---

## Compilação

### Linux / macOS

```bash
gcc -Wall -Wextra -std=c11 -o compilador main.c lexico.c sintatico.c semantico.c -lm
```

### Windows (MinGW)

```bash
gcc -Wall -Wextra -std=c11 -o compilador.exe main.c lexico.c sintatico.c semantico.c -lm
```

---

## Execução

### Linux / macOS

```bash
./compilador arquivo.pas
```

Exemplo:

```bash
./compilador testes/teste_correto_completo.pas
```

### Windows

```cmd
compilador.exe arquivo.pas
```

Exemplo:

```cmd
compilador.exe testes\teste_correto_completo.pas
```

---

## Casos de Teste

### Programas Válidos

- teste_p3.pas
- teste_p4.pas
- teste_correto_completo.pas

### Programas com Erro Sintático

- erro_token_inesperado.pas
- erro_fim_arquivo.pas
- erro_falta_dois_pontos.pas

### Teste Semântico

- teste_div_zero.pas

---

## Exemplos de Saída

### Token inesperado

```text
5:token nao esperado [*].
```

### Fim de arquivo inesperado

```text
6:fim de arquivo nao esperado.
```

### Divisão por zero

```text
erro semantico: divisao por zero conhecida em tempo de compilacao
```

---

## Fluxo de Execução

```text
Código Fonte (.pas)
          │
          ▼
 Análise Léxica
          │
          ▼
 Análise Sintática
          │
          ▼
 Construção da AST
          │
          ▼
 Constant Folding
          │
          ▼
 Análise Semântica
          │
          ▼
 Resultado Final
```

---

## Tecnologias Utilizadas

- Linguagem C
- GCC
- Parser Descendente Recursivo
- Árvore Sintática Abstrata (AST)

---

## Restrições do Projeto

Este trabalho foi implementado manualmente, sem utilização de:

- Lex
- Flex
- Yacc
- Bison
- ANTLR
- Geradores automáticos de analisadores léxicos ou sintáticos

---

## Disciplina

**Linguagens Formais, Autômatos e Compiladores**

Universidade Católica de Brasília (UCB)

---

## Autores

Projeto desenvolvido em grupo para fins acadêmicos.

- Alice Xavier
- Gustavo Xavier
- Júlia Clovandi 
- Luís Felipe
- Yuri Clovandi 
