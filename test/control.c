#include "test.h"

/*
 * This is a block comment.
 */

int func1() {
  int i;
  i = 0;
  for (;;i = i + 1)
    if (i == 10)
      return i;
}

int main() {
  ASSERT(3, ({ int x; if (0) x=2; else x=3; x; }));
  ASSERT(3, ({ int x; if (1-1) x=2; else x=3; x; }));
  ASSERT(2, ({ int x; if (1) x=2; else x=3; x; }));
  ASSERT(2, ({ int x; if (2-1) x=2; else x=3; x; }));

  ASSERT(55, ({ int i=0; int j=0; for (i=0; i<=10; i=i+1) j=i+j; j; }));

  ASSERT(55, ({ int j=0; for (int i=0; i<=10; i=i+1) j=i+j; j; }));

  ASSERT(10, ({ int i=0; while(i<10) i=i+1; i; }));

  ASSERT(3, ({ 1; {2;} 3; }));
  ASSERT(5, ({ ;;; 5; }));

  ASSERT(10, ({ int i=0; while(i<10) i=i+1; i; }));
  ASSERT(55, ({ int i=0; int j=0; while(i<=10) {j=i+j; i=i+1;} j; }));

  ASSERT(10, func1());
  ASSERT(10, ({ int i; for (i=0;;i=i+1) if (i == 10) break; i; }));
  ASSERT(10, ({ int i = 0; while (1) {if (i == 10) break; i=i+1;} i;}));

  ASSERT(15, ({ int sum = 0; for (int i=0; i<10; i=i+1) {for (int j=0; j<i; j=j+1) sum = sum + 1;
         if (i == 5) break;} sum; }));

  printf("OK\n");
  return 0;
}
