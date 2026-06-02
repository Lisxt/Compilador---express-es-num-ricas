program calculo_completo;
var
  a: integer;
  b: integer;
  resultado: integer;
begin
  { Atribuicoes basicas }
  a := 15;
  b := 4;

  { Expressao aritmetica com parenteses }
  resultado := (a + b) * 2;

  { Condicional com operador relacional >= e else }
  if resultado >= 30 then
    resultado := resultado - 10
  else
    resultado := resultado + 5;

  { Sinal unario negativo }
  a := -3 + b;

  { Expressao relacional com <> e parenteses }
  if (a + b) <> 0 then
    resultado := resultado * a;

  { Operador relacional < }
  if a < b then
    b := b - a;

  { While com condicao relacional > }
  while resultado > 0 do
    resultado := resultado - 1;

  { Atribuicao com multiplos operadores }
  b := a * 2 + resultado
end.
