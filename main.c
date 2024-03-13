#include "recc.h"
#include <stdio.h>

char *user_input;
Token *token;
char *filename;

int main(int argc, char **argv) {
  if (argc != 2) {
    error("引数の個数が正しくありません");
    return 1;
  }

  // トークナイズしてパースする
  filename = argv[1];
  user_input = read_file(filename);
  token = tokenize(user_input);
  program();

  // アセンブリの前半部分を出力
  printf(".intel_syntax noprefix\n");

  // 文字列リテラルを生成
  for (int i=0; i < string_literals_len; i++) {
    printf(".section .rodata\n");
    printf(".Lstr%d:\n", i);
    printf("  .string \"%s\"\n", string_literals[i]);
  }

  // 先頭の定義から順にコード生成
  for (int i = 0; code[i]; i++) {
    gen_definition(code[i]);
  }

  return 0;
}
