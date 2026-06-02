#include "semantico.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- utilidades internas ---------- */

// Helper pra salvar a mensagem de erro no buffer com segurança
static void set_err(char *buf, size_t n, const char *msg) {
  if (!buf || n == 0)
    return;

  /*
   * Copia a mensagem para o buffer de erro.
   * O snprintf evita escrita além do tamanho disponível.
   */
  snprintf(buf, n, "%s", msg);
}

// Resolve as regras de operações binárias (+, -, *, /).
// Checa tipos incompatíveis, divisão por zero, e
// tenta otimizar constantes que já sabemos o valor.
static SemInfo sem_join_bin(const SemInfo *L, const SemInfo *R, ASTKind op,
                            char *err, size_t n) {
  SemInfo out = {TY_NUM, 0, 0.0, 0};

  /*
   * Se algum dos operandos já contém erro semântico,
   * a operação também passa a conter erro.
   */
  if (L->type == TY_ERROR || R->type == TY_ERROR) {
    out.type = TY_ERROR;
    out.has_error = 1;
    return out;
  }

  // Por enquanto o MicroPascal só aceita números nas contas.
  // A checagem já tá estruturada caso a gente coloque booleanos depois.
  if (L->type != TY_NUM || R->type != TY_NUM) {
    out.type = TY_ERROR;
    out.has_error = 1;

    set_err(err, n,
            "erro semantico: operador aritmetico requer operandos numericos");

    return out;
  }

  out.type = TY_NUM;

  /*
   * Constant folding:
   * se os dois lados da operação são constantes,
   * o resultado pode ser calculado ainda na análise semântica.
   */
  if (L->is_const && R->is_const) {
    out.is_const = 1;

    switch (op) {
    case AST_ADD:
      out.cval = L->cval + R->cval;
      break;

    case AST_SUB:
      out.cval = L->cval - R->cval;
      break;

    case AST_MUL:
      out.cval = L->cval * R->cval;
      break;

    case AST_DIV:
      /*
       * Se o divisor é zero e isso já é conhecido
       * em tempo de compilação, há erro semântico.
       */
      if (R->cval == 0.0) {
        out.type = TY_ERROR;
        out.has_error = 1;
        out.is_const = 0;

        set_err(err, n,
                "erro semantico: divisao por zero conhecida em tempo de "
                "compilacao");
      } else {
        out.cval = L->cval / R->cval;
      }
      break;

    default:
      break;
    }

  } else {
    /*
     * Mesmo quando nem toda a expressão é constante,
     * ainda é possível detectar divisão por zero
     * se o lado direito for estaticamente conhecido como zero.
     */
    if (op == AST_DIV && R->is_const && R->cval == 0.0) {
      out.type = TY_ERROR;
      out.has_error = 1;

      set_err(err, n, "erro semantico: divisao por zero");
    }
  }

  /*
   * Propaga eventuais erros encontrados nos operandos.
   */
  out.has_error = out.has_error || L->has_error || R->has_error;

  return out;
}

// Desce pela AST validando se o código faz sentido.
// Se achar algo bizarro, anota o erro e propaga pra cima.
static SemInfo sem_walk(const AST *t, char *err, size_t n) {
  SemInfo s = {TY_NUM, 0, 0.0, 0};

  /*
   * Uma árvore nula é considerada erro semântico.
   */
  if (!t) {
    s.type = TY_ERROR;
    s.has_error = 1;

    set_err(err, n, "nó AST nulo");

    return s;
  }

  switch (t->kind) {
  case AST_NUM:
    /*
     * Um nó numérico sempre é do tipo numérico
     * e também é uma constante.
     */
    s.type = TY_NUM;
    s.is_const = 1;
    s.cval = t->num;

    return s;

  case AST_ADD:
  case AST_SUB:
  case AST_MUL:
  case AST_DIV: {
    /*
     * Para operadores binários, a função analisa primeiro
     * a subárvore esquerda e depois a subárvore direita.
     */
    SemInfo L = sem_walk(t->left, err, n);
    SemInfo R = sem_walk(t->right, err, n);

    /*
     * Depois combina as informações dos dois operandos
     * conforme o operador encontrado.
     */
    return sem_join_bin(&L, &R, t->kind, err, n);
  }

  case AST_VAR:
    /*
     * Variável → tipo numérico, valor desconhecido em tempo de compilação.
     * Sem tabela de símbolos: afirmamos apenas que a variável é TY_NUM.
     */
    s.type = TY_NUM;
    s.is_const = 0;
    return s;

  case AST_ASSIGN: {
    /*
     * Atribuição: verifica semanticamente a expressão do lado direito.
     * O lado esquerdo (AST_VAR) é estrutural nesta versão sem tabela.
     */
    if (!t->right) {
      s.type = TY_ERROR;
      s.has_error = 1;
      set_err(err, n, "erro semantico: atribuicao sem expressao");
      return s;
    }
    SemInfo R = sem_walk(t->right, err, n);
    s.type = R.type;
    s.has_error = R.has_error;
    return s;
  }

  case AST_REL: {
    /*
     * Operador relacional: verifica que ambos os operandos são TY_NUM.
     * Resultado é TY_NUM (convenção Pascal: 0 = falso, ≠0 = verdadeiro).
     */
    SemInfo L = sem_walk(t->left, err, n);
    SemInfo R = sem_walk(t->right, err, n);
    if (L.type == TY_ERROR || R.type == TY_ERROR) {
      s.type = TY_ERROR;
      s.has_error = 1;
      return s;
    }
    if (L.type != TY_NUM || R.type != TY_NUM) {
      s.type = TY_ERROR;
      s.has_error = 1;
      set_err(err, n,
              "erro semantico: operador relacional requer operandos numericos");
      return s;
    }
    s.type = TY_NUM;
    s.has_error = L.has_error || R.has_error;
    return s;
  }

  case AST_IF: {
    /*
     * Condicional: verifica condição (left) e os ramos then/else
     * (right = AST_BLOCK com left=then, right=else).
     */
    SemInfo cond = sem_walk(t->left, err, n);
    int erro = cond.has_error;
    if (t->right) {
      SemInfo ramos = sem_walk(t->right, err, n);
      erro |= ramos.has_error;
    }
    s.has_error = erro;
    if (erro)
      s.type = TY_ERROR;
    return s;
  }

  case AST_WHILE: {
    /*
     * Repetição: verifica condição (left) e corpo (right).
     * Corpo pode ser NULL para comando vazio (while x do ;).
     */
    SemInfo cond = sem_walk(t->left, err, n);
    int erro = cond.has_error;
    if (t->right) {
      SemInfo corpo = sem_walk(t->right, err, n);
      erro |= corpo.has_error;
    }
    s.has_error = erro;
    if (erro)
      s.type = TY_ERROR;
    return s;
  }

  case AST_PROG: {
    /*
     * Raiz do programa: propaga verificação para o bloco principal (left).
     */
    if (t->left) {
      SemInfo blk = sem_walk(t->left, err, n);
      s.has_error = blk.has_error;
      if (s.has_error)
        s.type = TY_ERROR;
    }
    return s;
  }

  case AST_BLOCK: {
    /*
     * Bloco / comando composto.
     * right: outro bloco (bloco do programa: left=decls, right=cmd_composto)
     *        ou ramo else de um if.
     * left:  primeiro elemento de uma lista encadeada via next
     *        (comandos ou declarações).
     */
    int erro = 0;
    if (t->right) {
      SemInfo c = sem_walk(t->right, err, n);
      erro |= c.has_error;
    }
    /* Percorre lista encadeada via next (comandos ou declarações) */
    const AST *cur = t->left;
    while (cur) {
      SemInfo c = sem_walk(cur, err, n);
      erro |= c.has_error;
      cur = cur->next;
    }
    s.has_error = erro;
    if (erro)
      s.type = TY_ERROR;
    return s;
  }

  case AST_DECL:
    /*
     * Declaração de variável: nó estrutural.
     * Sem tabela de símbolos, não há verificação semântica adicional.
     */
    s.type = TY_NUM;
    s.has_error = 0;
    return s;

  default:
    /*
     * Qualquer tipo de nó desconhecido é tratado
     * como erro semântico.
     */
    s.type = TY_ERROR;
    s.has_error = 1;

    set_err(err, n, "erro semantico: operador/desconhecido");

    return s;
  }
}

/* ---------- API ---------- */

/*
 * Verifica se uma árvore sintática abstrata está semanticamente correta.
 *
 * Retorna:
 * - 1, se não houver erro semântico;
 * - 0, se houver erro.
 *
 * Em caso de erro, grava a mensagem no buffer errbuf.
 */
int sem_check(const AST *t, char *errbuf, size_t errbufsz) {
  char tmpbuf[256];

  tmpbuf[0] = '\0';

  SemInfo s = sem_walk(t, tmpbuf, sizeof(tmpbuf));

  if (s.has_error) {
    set_err(errbuf, errbufsz, tmpbuf[0] ? tmpbuf : "erro semantico");

    return 0;
  }

  return 1;
}

/*
 * Avalia uma expressão constante representada pela AST.
 *
 * A função só retorna sucesso se:
 * - não houver erro semântico;
 * - a expressão inteira for constante.
 *
 * Retorna:
 * - 1, se conseguiu avaliar;
 * - 0, caso contrário.
 */
int sem_eval_const(const AST *t, double *out) {
  SemInfo s = sem_walk(t, NULL, 0);

  if (!s.has_error && s.is_const) {
    if (out) {
      *out = s.cval;
    }

    return 1;
  }

  return 0;
}

/* ---------- Constant folding destrutivo ---------- */

/*
 * Verifica se um tipo de nó da AST representa operação binária.
 *
 * São operações binárias:
 * - soma;
 * - subtração;
 * - multiplicação;
 * - divisão.
 */
static int is_bin(ASTKind k) {
  return (k == AST_ADD || k == AST_SUB || k == AST_MUL || k == AST_DIV);
}

// Faz o Constant Folding navegando pela árvore de baixo pra cima.
// Se achar uma conta só com constantes (ex: 2 + 3),
// calcula o resultado e reaproveita o nó da operação
// transformando ele num número fixo.
AST *sem_fold(AST *t) {
  if (!t)
    return t;

  /*
   * A otimização só se aplica a nós binários.
   */
  if (is_bin(t->kind)) {
    /*
     * Primeiro aplica constant folding nas subárvores.
     */
    t->left = sem_fold(t->left);
    t->right = sem_fold(t->right);

    double lv = 0.0;
    double rv = 0.0;

    /*
     * Verifica se os lados esquerdo e direito são constantes.
     */
    int lc = sem_eval_const(t->left, &lv);
    int rc = sem_eval_const(t->right, &rv);

    if (lc && rc) {
      /*
       * Em caso de divisão por zero, não simplifica o nó.
       * Assim, a análise semântica posterior ainda poderá
       * reportar o erro adequadamente.
       */
      if (t->kind == AST_DIV && rv == 0.0) {
        return t;
      }

      double res = 0.0;

      /*
       * Calcula o resultado da operação constante.
       */
      switch (t->kind) {
      case AST_ADD:
        res = lv + rv;
        break;

      case AST_SUB:
        res = lv - rv;
        break;

      case AST_MUL:
        res = lv * rv;
        break;

      case AST_DIV:
        res = lv / rv;
        break;

      default:
        break;
      }

      /*
       * Libera as subárvores antigas, pois elas serão substituídas
       * por um único nó numérico.
       */
      ast_free(t->left);
      ast_free(t->right);

      /*
       * Reaproveita o nó atual como um nó numérico.
       */
      t->kind = AST_NUM;
      t->num = res;
      t->left = NULL;
      t->right = NULL;
    }
  } else {
    // Tem que descer a recursão pra achar as contas escondidas
    // dentro de atribuições, ifs e whiles, além de percorrer a lista encadeada
    // (next).
    t->left = sem_fold(t->left);
    t->right = sem_fold(t->right);

    AST *prox = t->next;
    while (prox) {
      sem_fold(prox);
      prox = prox->next;
    }
  }

  return t;
}