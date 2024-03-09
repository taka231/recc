#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./recc "$input" > tmp.s
  cc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 "0;"
assert 42 "42;"
assert 21 "5+20-4;"
assert 41 " 12 + 34 - 5 ;"
assert 47 '5+6*7;'
assert 15 '5*(9-6);'
assert 4 '(3+5)/2;'
assert 10 '-10+20;'
assert 30 '+10+20;'
assert 15 '-3*+5+30;'
assert 2 '-(3+5)+10;'
assert 1 '1==1;'
assert 1 '1<=1;'
assert 1 '1<=2;'
assert 0 '1>=2;'
assert 1 '1<2;'
assert 0 '1>1;'
assert 1 '1+1==2;'
assert 1 '1+1>=2;'
assert 0 '1+1>2;'
assert 10 'a=10;'
assert 10 'a=10; a;'
assert 30 'a=10; b=20; c = a + b; c;'
assert 10 'a=1; b=2; c=3; d=4; a+b+c+d;'
assert 10 'foo = 3; bar = 7; foo + bar;'
assert 10 'foo = 3; bar = 7; return foo + bar;'
assert 10 'return_10 = 10; return return_10;'
assert 10 'return 10; return 20;'
assert 10 '
if (1)
  return 10;
'
assert 10 '
if (1)
  return 10;
else
  return 20;
'
assert 20 '
if (0)
  return 10;
else
  return 20;
'
assert 10 '
if (2==2)
  return 10;
else
  return 20;
'

assert 30 '
if (0)
  return 10;
else if (0)
  return 20;
else
  return 30;
'

assert 11 '
i = 0;
while (i <= 10)
  i = i + 1;
return i;
'

assert 55 '
sum = 0;
for (i = 1; i <= 10; i = i + 1)
  sum = sum + i;
return sum;
'

assert 10 '
i = 0;
for (;;i = i + 1)
  if (i == 10)
    return i;
'

echo OK
