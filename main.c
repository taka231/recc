#include "recc.h"
#include <stdio.h>

char *user_input;
Token *token;

int main(int argc, char **argv) {
  if (argc != 2) {
    error("引数の個数が正しくありません");
    return 1;
  }

  // トークナイズしてパースする
  user_input = argv[1];
  token = tokenize(user_input);
  program();

  // アセンブリの前半部分を出力
  printf(".intel_syntax noprefix\n");

  // 先頭の定義から順にコード生成
  for (int i = 0; code[i]; i++) {
    gen_definition(code[i]);
  }

  return 0;
}
