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