#include "recc.h"
#include <stdio.h>

int labelseq = 1;

void gen_lval(Node *node) {
  if (node->kind != ND_LVAR)
    error("代入の左辺値が変数ではありません");
  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->offset);
  printf("  push rax\n");
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
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
  case ND_ADD:
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
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
