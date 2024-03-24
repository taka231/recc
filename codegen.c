#include "recc.h"
#include <stdio.h>
#include <stdlib.h>

int labelseq = 1;

char *loop_label;

void gen(Node *node);

void gen_lval(Node *node) {
  switch (node->kind) {
  case ND_LVAR:
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", node->offset);
    printf("  push rax\n");
    return;
  case ND_GVAR:
    printf("  lea rax, %.*s[rip]\n", node->len, node->name);
    printf("  push rax\n");
    return;
  case ND_DEREF:
    gen(node->lhs);
    return;
  }
}

void gen(Node *node) {
  switch (node->kind) {
  case ND_NUM:
    printf("  push %d\n", node->val);
    return;
  case ND_STRING_LIT:
    printf("  lea rax, .Lstr%d[rip]\n", node->val);
    printf("  push rax\n");
    return;
  case ND_LVAR:
  case ND_GVAR:
    if (node->type->ty == ARRAY) {
      gen_lval(node);
      return;
    }
    gen_lval(node);
    printf("  pop rax\n");
    if (node->type->ty == CHAR)
      printf("  movsx rax, byte ptr [rax]\n");
    else
      printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  case ND_ASSIGN:
    gen_lval(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");
    if (node->lhs->type->ty == CHAR)
      printf("  mov [rax], dil\n");
    else
      printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
    return;
  case ND_RETURN:
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    return;
  case ND_IF: {
    int label = labelseq++;
    printf("  push 0\n"); // dummy
    gen(node->lhs); // cond
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    if (node->else_stmt) {
      printf("  je .Lelse%d\n", label);
      gen(node->rhs); // then
      printf("  pop rax\n");
      printf("  jmp .Lend%d\n", label);
      printf(".Lelse%d:\n", label);
      gen(node->else_stmt); // else
      printf("  pop rax\n");
    } else {
      printf("  je .Lend%d\n", label);
      gen(node->rhs); // then
      printf("  pop rax\n");
    }
    printf(".Lend%d:\n", label);
    return;
  }
  case ND_WHILE: {
    int label = labelseq++;
    char *prev_label = loop_label;
    loop_label = malloc(20);
    sprintf(loop_label, ".Lend%d", label);
    printf("push 0\n"); // dummy
    printf(".Lbegin%d:\n", label);
    gen(node->lhs); // cond
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je .Lend%d\n", label);
    printf("  pop rax\n");
    gen(node->rhs); // body
    printf("  jmp .Lbegin%d\n", label);
    printf(".Lend%d:\n", label);
    loop_label = prev_label;
    return;
  }
  case ND_FOR: {
    int label = labelseq++;
    char *prev_label = loop_label;
    loop_label = malloc(20);
    sprintf(loop_label, ".Lend%d", label);
    printf("  push 0\n"); // dummy
    if (node->lhs) {
      gen(node->lhs); // init
      printf("  pop rax\n");
    }
    printf(".Lbegin%d:\n", label);
    if (node->rhs) {
      gen(node->rhs); // cond
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je .Lend%d\n", label);
    }
    printf("  pop rax\n");
    gen(node->for_body);
    if (node->for_third) {
      gen(node->for_third); // post
      printf("  pop rax\n");
    }
    printf("  jmp .Lbegin%d\n", label);
    printf(".Lend%d:\n", label);
    loop_label = prev_label;
    return;
  }
  case ND_BLOCK:
    for (int i = 0; i < node->nodes->len; i++) {
      gen(node->nodes->data[i]);
      if (i != node->nodes->len - 1)
        printf("  pop rax\n");
    }
    return;
  case ND_CALL:
    for (int i = 0; i < node->nodes->len; i++) {
      gen(node->nodes->data[i]);
    }
    for (int i = node->nodes->len - 1; i >= 0; i--) {
      if (i == 0)
        printf("  pop rdi\n");
      else if (i == 1)
        printf("  pop rsi\n");
      else if (i == 2)
        printf("  pop rdx\n");
      else if (i == 3)
        printf("  pop rcx\n");
      else if (i == 4)
        printf("  pop r8\n");
      else if (i == 5)
        printf("  pop r9\n");
    }
    printf("  mov rax, %d\n", node->nodes->len);
    printf("  mov al, 0\n");
    // 16バイト境界に合うようにスタックを調整
    printf("  mov r10, rsp\n");
    printf("  sub r10, 8\n");
    printf("  and r10, 15\n");
    printf("  sub rsp, r10\n");
    printf("  push r10\n");
    printf("  call %.*s\n", node->len, node->name);
    printf("  pop r10\n");
    printf("  add rsp, r10\n");
    printf("  push rax\n");
    return;
  case ND_ADDR:
    gen_lval(node->lhs);
    return;
  case ND_DEREF:
    gen(node->lhs);
    if (node->type->ty == ARRAY) {
      return;
    }
    printf("  pop rax\n");
    if (node->type->ty == CHAR)
      printf("  movsx rax, byte ptr [rax]\n");
    else
      printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  case ND_VARDEF:
    // dummy
    printf("  push 0\n");
    return;
  case ND_BREAK:
    if (loop_label == NULL)
      error("break文がループの外にあります");
    printf("  jmp %s\n", loop_label);
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
  case ND_ADD:
    if (node->type->ty == PTR || node->type->ty == ARRAY) {
      if (node->lhs->type->ty == PTR || node->lhs->type->ty == ARRAY)
        printf("  imul rdi, %d\n", size_of(node->type->ptr_to));
      else
        printf("  imul rax, %d\n", size_of(node->type->ptr_to));
    }

    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    if (node->type->ty == PTR || node->type->ty == ARRAY) {
      if (node->lhs->type->ty == PTR || node->lhs->type->ty == ARRAY)
        printf("  imul rdi, %d\n", size_of(node->type->ptr_to));
      else
        printf("  imul rax, %d\n", size_of(node->type->ptr_to));
    }

    printf("  sub rax, rdi\n");
    break;
  case ND_MUL:
    printf("  imul rax, rdi\n");
    break;
  case ND_DIV:
    printf("  cqo\n");
    printf("  idiv rdi\n");
    break;
  case ND_EQ:
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_NE:
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LT:
    printf("  cmp rax, rdi\n");
    printf("  setl al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_LE:
    printf("  cmp rax, rdi\n");
    printf("  setle al\n");
    printf("  movzb rax, al\n");
    break;
  }

  printf("  push rax\n");
}

// 関数定義等のコードを生成
void gen_definition(Node *node) {
  switch (node->kind) {
  case ND_FUNDEF: {
    // 関数のプロローグ
    // アセンブリの前半部分を出力
    printf(".text\n");
    printf(".globl %.*s\n", node->len, node->name);
    printf("%.*s:\n", node->len, node->name);

    // プロローグ
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    for (int i = 0; i < node->nodes->len; i++) {
      Node *arg = node->nodes->data[i];
      if (arg->kind != ND_VARDEF)
        error("代入の左辺値が変数ではありません");
      printf("  mov rax, rbp\n");
      printf("  sub rax, %d\n", arg->offset);
      if (i == 0)
        printf("  mov [rax], rdi\n");
      else if (i == 1)
        printf("  mov [rax], rsi\n");
      else if (i == 2)
        printf("  mov [rax], rdx\n");
      else if (i == 3)
        printf("  mov [rax], rcx\n");
      else if (i == 4)
        printf("  mov [rax], r8\n");
      else if (i == 5)
        printf("  mov [rax], r9\n");
    }
    printf("  mov rsp, rbp\n");
    printf("  sub rsp, %d\n", node->offset);
    // 関数の本体
    gen(node->rhs);
    printf("  pop rax\n");

    // エピローグ
    // 最後の式の結果がRAXに残っているのでそれが返り値になる
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    return;
  }
  case ND_VARDEF:
    printf(".bss\n");
    printf(".globl %.*s\n", node->len, node->name);
    printf("%.*s:\n", node->len, node->name);
    printf("  .zero %d\n", size_of(node->type));
    return;
  }
}
