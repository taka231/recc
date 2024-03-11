#include "recc.h"
#include <stdio.h>

int labelseq = 1;

void gen(Node *node);

void gen_lval(Node *node) {
  switch (node->kind) {
  case ND_LVAR:
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", node->offset);
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
  case ND_LVAR:
    gen_lval(node);
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  case ND_ASSIGN:
    gen_lval(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");
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
    gen(node->lhs); // cond
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    if (node->else_stmt) {
      printf("  je .Lelse%d\n", label);
      gen(node->rhs); // then
      printf("  jmp .Lend%d\n", label);
      printf(".Lelse%d:\n", label);
      gen(node->else_stmt); // else
    } else {
      printf("  je .Lend%d\n", label);
      gen(node->rhs); // then
    }
    printf(".Lend%d:\n", label);
    return;
  }
  case ND_WHILE: {
    int label = labelseq++;
    printf(".Lbegin%d:\n", label);
    gen(node->lhs); // cond
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je .Lend%d\n", label);
    gen(node->rhs); // body
    printf("  jmp .Lbegin%d\n", label);
    printf(".Lend%d:\n", label);
    return;
  }
  case ND_FOR: {
    int label = labelseq++;
    if (node->lhs) {
      gen(node->lhs); // init
    }
    printf(".Lbegin%d:\n", label);
    if (node->rhs) {
      gen(node->rhs); // cond
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je .Lend%d\n", label);
    }
    gen(node->for_body);
    if (node->for_third) {
      gen(node->for_third); // post
    }
    printf("  jmp .Lbegin%d\n", label);
    printf(".Lend%d:\n", label);
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
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  case ND_VARDEF:
    // dummy
    printf("  push 0\n");
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
  case ND_ADD:
    if (node->lhs->type->ty == PTR) {
      if (node->lhs->type->ptr_to->ty == INT)
        printf("  imul rdi, 4\n");
      else if (node->lhs->type->ptr_to->ty == PTR)
        printf("  imul rdi, 8\n");
    }
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    if (node->lhs->type->ty == PTR) {
      if (node->lhs->type->ptr_to->ty == INT)
        printf("  imul rdi, 4\n");
      else if (node->lhs->type->ptr_to->ty == PTR)
        printf("  imul rdi, 8\n");
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
  }
  }
}
