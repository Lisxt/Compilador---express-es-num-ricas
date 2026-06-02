#include "lexico.h"
#include "semantico.h"
#include "sintatico.h"
#include <stdio.h>
#include <stdlib.h>

/* FUNÇÃO PRINCIPAL: COMPILADOR MICROPASCAL*/
int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Erro de uso. Formato esperado: %s <arquivo.pas>\n",
            argv[0]);
    return 1;
  }

  FILE *arquivo = fopen(argv[1], "r");
  if (!arquivo) {
    fprintf(stderr, "Erro: Nao foi possivel abrir o arquivo '%s'.\n", argv[1]);
    return 1;
  }

  /* Descobre o tamanho do arquivo para alocar a memória correta */
  fseek(arquivo, 0, SEEK_END);
  long tamanho = ftell(arquivo);
  fseek(arquivo, 0, SEEK_SET);

  /* Aloca um buffer de string para armazenar todo o código-fonte */
  char *buffer = (char *)malloc(tamanho + 1);
  if (!buffer) {
    fprintf(stderr, "Erro critico: Falha de alocacao de memoria.\n");
    fclose(arquivo);
    return 1;
  }

  /* Lê o arquivo inteiro para o buffer e fecha o arquivo */
  size_t bytes_lidos = fread(buffer, 1, tamanho, arquivo);
  buffer[bytes_lidos] = '\0';
  fclose(arquivo);

  /* Inicialização da infraestrutura do analisador léxico usando a função
   * correta */
  Lexer lexer;
  lexer_init(&lexer, buffer);

  printf("--- Iniciando Analise Sintatica de '%s' ---\n", argv[1]);

  /* Disparando o parser pelo axioma principal */
  AST *arvore_resultante = parse_ast(&lexer);

  printf("\n>>> Analise Sintatica concluida com SUCESSO! <<<\n");

  /* --- Constant Folding: simplifica subárvores constantes na AST --- */
  printf("\n--- Iniciando Constant Folding ---\n");
  arvore_resultante = sem_fold(arvore_resultante);
  printf(">>> Constant Folding concluido! <<<\n");

  /* --- Analise Semantica: verificacao de tipos e erros em tempo de compilacao
   * --- */
  printf("\n--- Iniciando Analise Semantica de '%s' ---\n", argv[1]);
  char errbuf[256];
  errbuf[0] = '\0';
  if (!sem_check(arvore_resultante, errbuf, sizeof(errbuf))) {
    fprintf(stderr, "\n%s\n", errbuf);
    ast_free(arvore_resultante);
    free(buffer);
    return 1;
  }
  printf(">>> Analise Semantica concluida com SUCESSO! <<<\n");

  /* Liberacao de memoria da AST e do buffer do codigo-fonte */
  ast_free(arvore_resultante);
  free(buffer);

  return 0;
}