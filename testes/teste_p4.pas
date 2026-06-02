program teste_pessoa4;
var
  a: integer;
  b: integer;
begin
  a := 10;
  b := 3;

  { Testa operador: = }
  if a = b then
    a := 0;

  { Testa operador: <> }
  if a <> b then
    a := 1;

  { Testa operador: < }
  if a < b then
    b := 0;

  { Testa operador: <= }
  if a <= b then
    b := 1;

  { Testa operador: > }
  if a > b then
    a := a - 1;

  { Testa operador: >= }
  if a >= b then
    b := b + 1;

  { Testa sinal unario negativo }
  a := -5 + 3;

  { Testa expressao com parenteses e relacional }
  if (a + b) <> 0 then
    a := a * 2;

  { Testa while com relacional }
  while a > 0 do
    a := a - 1
end.
