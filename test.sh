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

assert 21 "main() { return 5+20-4; }"
assert 41 "main() { return 12 + 34 - 5; }"
assert 47 'main() { return 5+6*7; }'
assert 15 'main() { return 5*(9-6); }'
assert 4 'main() { return (3+5)/2; }'
assert 10 'main() { return -10+20; }'
assert 15 'main() { return -3*5+30; }'
assert 15 'main() { return -3*-5; }'
assert 2 'main() { return 1+1; }'
assert 1 'main() { return 1==1; }'
assert 1 'main() { return 1!=2; }'
assert 1 'main() { return 1<=2; }'
assert 0 'main() { return 1>1; }'
assert 1 'main() { return 1+1==2; }'
assert 0 'main() { return 1+1>2; }'
assert 1 'main() { return 1+1>=2; }'
assert 1 'main() { return 1+1<=2; }'
assert 0 'main() { return 1+1!=2; }'
assert 10 'main() { return a=10; }'
assert 10 'main() { a=10; return a; }'
assert 30 'main() { a=10; b=20; return a+b; }'
assert 10 'main() { a=1; b=2; c=3; d=4; return a+b+c+d; }'
assert 10 'main() { foo = 3; bar = 7; return foo + bar; }'
assert 10 'main() { return_10 = 10; return return_10; }'
assert 10 'main() { return 10; return 20; }'
assert 10 '
main() {
  if (1)
    return 10;
}
'
assert 10 '
main() {
  if (1)
    return 10;
  else
    return 20;
}
'
assert 20 '
main() {
  if (0)
    return 10;
  else
    return 20;
}
'
assert 10 '
main() {
  if (2==2)
    return 10;
  else
    return 20;
}
'

assert 30 '
main() {
  if (0)
    return 10;
  else if (0)
    return 20;
  else
    return 30;
}
'

assert 10 '
main() {
  i = 0;
  while (i < 10)
    i = i + 1;
  return i;
}
'

assert 55 '
main() {
  sum = 0;
  for (i = 1; i <= 10; i = i + 1)
    sum = sum + i;
  return sum;
}
'

assert 10 '
main() {
  i = 0;
  for (;;i = i + 1)
    if (i == 10)
      return i;
}
'

assert 55 '
main() {
  sum = 0;
  i = 0;
  while (i <= 10) {
    sum = sum + i;
    i = i + 1;
  }
  return sum;
}
'

assert 5 '
add(a, b) {
  return a + b;
}

main() {
  return add(2, 3);
}
'

assert 9 '
add(a, b) {
  return a + b;
}

main() {
  return add(2, add(3, 4));
}
'

assert 6 '
add(a, b) {
  return a + b;
}

main() {
  return add(add(0, 1), add(2, 3));
}
'

assert 120 '
fact(n) {
  if (n == 0)
    return 1;
  return n * fact(n - 1);
}
main() {
  return fact(5);
}
'

assert 55 '
fib(n) {
  if (n <= 1)
    return n;
  return fib(n - 1) + fib(n - 2);
}
main() {
  return fib(10);
}
'


echo OK
