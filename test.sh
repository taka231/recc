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

assert 21 "int main() { return 5+20-4; }"
assert 41 "int main() { return 12 + 34 - 5; }"
assert 47 'int main() { return 5+6*7; }'
assert 15 'int main() { return 5*(9-6); }'
assert 4 'int main() { return (3+5)/2; }'
assert 10 'int main() { return -10+20; }'
assert 15 'int main() { return -3*5+30; }'
assert 15 'int main() { return -3*-5; }'
assert 2 'int main() { return 1+1; }'
assert 1 'int main() { return 1==1; }'
assert 1 'int main() { return 1-2 == -1; }'
assert 1 'int main() { return 1!=2; }'
assert 1 'int main() { return 1<=2; }'
assert 0 'int main() { return 1>1; }'
assert 1 'int main() { return 1+1==2; }'
assert 0 'int main() { return 1+1>2; }'
assert 1 'int main() { return 1+1>=2; }'
assert 1 'int main() { return 1+1<=2; }'
assert 0 'int main() { return 1+1!=2; }'
assert 10 'int main() { int a; return a=10; }'
assert 10 'int main() { int a; a=10; return a; }'
assert 30 'int main() { int a; int b; a=10; b=20; return a+b; }'
assert 10 'int main() { int a; int b; int c; int d; a=1; b=2; c=3; d=4; return a+b+c+d; }'
assert 10 'int main() { int foo; int bar; foo = 3; bar = 7; return foo + bar; }'
assert 10 'int main() { int return_10; return_10 = 10; return return_10; }'
assert 10 'int main() { return 10; return 20; }'
assert 10 '
int main() {
  if (1)
    return 10;
}
'
assert 10 '
int main() {
  if (1)
    return 10;
  else
    return 20;
}
'
assert 20 '
int main() {
  if (0)
    return 10;
  else
    return 20;
}
'
assert 10 '
int main() {
  if (2==2)
    return 10;
  else
    return 20;
}
'

assert 30 '
int main() {
  if (0)
    return 10;
  else if (0)
    return 20;
  else
    return 30;
}
'

assert 10 '
int main() {
  int i;
  i = 0;
  while (i < 10)
    i = i + 1;
  return i;
}
'

assert 55 '
int main() {
  int i;
  int sum;
  sum = 0;
  for (i = 1; i <= 10; i = i + 1)
    sum = sum + i;
  return sum;
}
'

assert 10 '
int main() {
  int i;
  i = 0;
  for (;;i = i + 1)
    if (i == 10)
      return i;
}
'

assert 55 '
int main() {
  int i;
  int sum;
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
int add(int a, int b) {
  return a + b;
}

int main() {
  return add(2, 3);
}
'

assert 9 '
int add(int a, int b) {
  return a + b;
}

int main() {
  return add(2, add(3, 4));
}
'

assert 6 '
int add(int a, int b) {
  return a + b;
}

int main() {
  return add(add(0, 1), add(2, 3));
}
'

assert 120 '
int fact(int n) {
  if (n == 0)
    return 1;
  return n * fact(n - 1);
}
int main() {
  return fact(5);
}
'

assert 55 '
int fib(int n) {
  if (n <= 1)
    return n;
  return fib(n - 1) + fib(n - 2);
}
int main() {
  return fib(10);
}
'

assert 3 '
int main() {
  int x;
  int *y;
  x = 3;
  y = &x;
  return *y;
}'

assert 3 '
int main() {
  int x;
  int *y;
  int **z;
  x = 3;
  y = &x;
  z = &y;
  return **z;
}'

assert 3 '
int main() {
  int x;
  int *y;
  y = &x;
  *y = 3;
  return x;
}
'

assert 4 '
int main() {
  int x;
  return sizeof(x);
}'

assert 8 '
int main() {
  int *x;
  return sizeof(x);
}'

assert 4 '
int main() {
  int *x;
  return sizeof(*x);
}'

assert 4 '
int main() {
  return sizeof(1);
}'

assert 40 '
int main() {
  int x[10];
  return sizeof(x);
}'

assert 200 '
int main() {
  int x[5][10];
  return sizeof(x);
}'

assert 3 '
int main() {
  int a[2];
  *a = 1;
  *(a + 1) = 2;
  int *p;
  p = a;
  return *p + *(p + 1);
}'

assert 40 '
int main() {
  int x[5][10];
  return sizeof(x[0]);
}'

assert 3 '
int main() {
  int x[2];
  x[0] = 1;
  x[1] = 2;
  return x[0] + x[1];
}'

assert 1 '
int main() {
  int x[2];
  0[x] = 1;
  return x[0];
}'

assert 2 '
int x;
int main() {
  x = 2;
  return x;
}'

assert 2 '
int x;
int foo() {
  x = 2;
}
int main() {
  foo();
  return x;
}
'

assert 3 '
int x;
int foo() {
  x = 2;
}
int main() {
  foo();
  int x;
  x = 3;
  return x;
}'

assert 10 '
int foo[4];
int bar() {
  int i;
  for(i = 0; i < 4 ; i = i + 1)
    foo[i] = i + 1;
}

int main() {
  bar();
  int i;
  int sum;
  sum = 0;
  for(i = 0; i < 4 ; i = i + 1)
    sum = sum + foo[i];
  return sum;
}'

assert 3 '
int main() {
  char x[3];
  x[0] = -1;
  x[1] = 2;
  int y;
  y = 4;
  return x[0] + y;
}'

assert 97 '
int main() {
  char *p;
  p = "a";
  return p[0];
}
'

echo OK
