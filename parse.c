#include "recc.h"
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// debug
void print_node(Node *node) {
  if (!node)
    return;
  switch (node->kind) {
  case ND_NUM:
    printf("%d\n", node->val);
    break;
  case ND_LVAR:
    printf("%.*s\n", node->len, node->name);
    break;
  case ND_VARDEF:
    printf("int %.*s\n", node->len, node->name);
    break;
  case ND_RETURN:
    printf("return ");
    print_node(node->lhs);
    printf("\n");
    break;
  case ND_IF:
    printf("if\n");
    break;
  case ND_WHILE:
    printf("while\n");
    break;
  case ND_FOR:
    printf("for\n");
    break;
  case ND_BLOCK:
    printf("{\n");
    for (int i = 0; i < node->nodes->len; i++)
      print_node(node->nodes->data[i]);
    printf("}\n");
    break;
  case ND_FUNDEF: {
    printf("%.*s(", node->len, node->name);
    for (int i = 0; i < node->nodes->len; i++) {
      print_node(node->nodes->data[i]);
      if (i != node->nodes->len - 1)
        printf(", ");
    }
    printf(")\n");
    print_node(node->rhs);
    break;
  }
  case ND_CALL: {
    printf("%.*s(", node->lhs->len, node->lhs->name);
    for (int i = 0; i < node->nodes->len; i++) {
      print_node(node->nodes->data[i]);
      if (i != node->nodes->len - 1)
        printf(", ");
    }
    printf(")\n");
    break;
  case ND_ASSIGN:
    print_node(node->lhs);
    printf(" = ");
    print_node(node->rhs);
    printf("\n");
    break;
  }
  }
}

// 指定されたファイルの内容を返す
char *read_file(char *path) {
  // ファイルを開く
  FILE *fp = fopen(path, "r");
  if (!fp)
    error("cannot open %s: %s", path, strerror(errno));

  // ファイルの長さを調べる
  if (fseek(fp, 0, SEEK_END) == -1)
    error("%s: fseek: %s", path, strerror(errno));
  size_t size = ftell(fp);
  if (fseek(fp, 0, SEEK_SET) == -1)
    error("%s: fseek: %s", path, strerror(errno));

  // ファイル内容を読み込む
  char *buf = calloc(1, size + 2);
  fread(buf, size, 1, fp);

  // ファイルが必ず"\n\0"で終わっているようにする
  if (size == 0 || buf[size - 1] != '\n')
    buf[size++] = '\n';
  buf[size] = '\0';
  fclose(fp);
  return buf;
}

Type *int_type() {
  Type *type = calloc(1, sizeof(Type));
  type->ty = INT;
  return type;
}

Type *char_type() {
  Type *type = calloc(1, sizeof(Type));
  type->ty = CHAR;
  return type;
}

Type *pointer_to(Type *base) {
  Type *type = calloc(1, sizeof(Type));
  type->ty = PTR;
  type->ptr_to = base;
  return type;
}

Type *array_type(Type *base, size_t size) {
  Type *type = calloc(1, sizeof(Type));
  type->ty = ARRAY;
  type->ptr_to = base;
  type->array_size = size;
  return type;
}

int size_of(Type *type) {
  switch (type->ty) {
  case INT:
    return 4;
  case CHAR:
    return 1;
  case PTR:
    return 8;
  case ARRAY:
    return size_of(type->ptr_to) * type->array_size;
  default:
    error("サイズが取得できません");
  }
}

void error_at(char *loc, char *msg);

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  switch (kind) {
  case ND_ADDR:
    node->type = pointer_to(lhs->type);
    break;
  case ND_DEREF:
    if (lhs->type->ty != PTR && lhs->type->ty != ARRAY)
      error_at(lhs->name, "ポインタではありません");
    node->type = lhs->type->ptr_to;
    break;
  default:
    if (lhs->type->ty == ARRAY)
      node->type = lhs->type;
    else if (rhs && rhs->type->ty == ARRAY)
      node->type = rhs->type;
    else if (rhs && rhs->type->ty == PTR)
      node->type = rhs->type;
    else
      node->type = lhs->type;
  }
  return node;
}

Node *new_node_num(int val) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  node->type = int_type();
  return node;
}

NodeArray *new_node_array() {
  NodeArray *array = calloc(1, sizeof(NodeArray));
  array->data = calloc(100, sizeof(Node *));
  array->capacity = 100;
  array->len = 0;
  return array;
}

void node_array_push(NodeArray *array, Node *node) {
  if (array->len == array->capacity) {
    array->capacity *= 2;
    array->data = realloc(array->data, sizeof(Node *) * array->capacity);
  }
  array->data[array->len++] = node;
}

// エラーを報告するための関数
// printfと同じ引数を取る
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// エラー箇所を報告する
void error_at(char *loc, char *msg) {
  // locが含まれている行の開始地点と終了地点を取得
  char *line = loc;
  while (user_input < line && line[-1] != '\n')
    line--;

  char *end = loc;
  while (*end != '\n')
    end++;

  // 見つかった行が全体の何行目なのかを調べる
  int line_num = 1;
  for (char *p = user_input; p < line; p++)
    if (*p == '\n')
      line_num++;

  // 見つかった行を、ファイル名と行番号と一緒に表示
  int indent = fprintf(stderr, "%s:%d: ", filename, line_num);
  fprintf(stderr, "%.*s\n", (int)(end - line), line);

  // エラー箇所を"^"で指し示して、エラーメッセージを表示
  int pos = loc - line + indent;
  fprintf(stderr, "%*s", pos, ""); // pos個の空白を出力
  fprintf(stderr, "^ %s\n", msg);
  exit(1);
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len ||
      memcmp(token->str, op, token->len))
    return false;
  token = token->next;
  return true;
}

bool consume_kind(TokenKind kind) {
  if (token->kind != kind)
    return false;
  token = token->next;
  return true;
}

Token *consume_ident() {
  if (token->kind != TK_IDENT)
    return NULL;
  Token *t = token;
  token = token->next;
  return t;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len ||
      memcmp(token->str, op, token->len)) {
    char *msg = calloc(1, 100);
    sprintf(msg, "'%s'ではありません", op);
    error_at(token->str, msg);
  }
  token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number() {
  if (token->kind != TK_NUM)
    error_at(token->str, "数ではありません");
  int val = token->val;
  token = token->next;
  return val;
}

int is_alnum(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
         ('0' <= c && c <= '9') || c == '_';
}

bool at_eof() { return token->kind == TK_EOF; }

Scope *locals;

int next_offset;

void newScope() {
  Scope *scope = calloc(1, sizeof(Scope));
  scope->next = locals;
  locals = scope;
}

LVar *find_lvar(Token *tok) {
  for (Scope *this = locals; this; this = this->next) {
    for (LVar *var = this->locals; var; var = var->next) {
      if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
        return var;
      }
    }
  }
  return NULL;
}

LVar *find_lvar_in_current_scope(Token *tok) {
  for (LVar *var = locals->locals; var; var = var->next) {
    if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
      return var;
    }
  }
  return NULL;
}

LVar *globals;

LVar *find_gvar(Token *tok) {
  for (LVar *var = globals; var; var = var->next) {
    if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
      return var;
    }
  }
  return NULL;
}

LVar *funtypes;

Type *get_funtype(Token *tok) {
  for (LVar *var = funtypes; var; var = var->next) {
    if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
      return var->type;
    }
  }
  return NULL;
}

char *string_literals[10000];
int string_literals_len = 0;

int find_string_literal(char *str) {
  for (int i = 0; i < string_literals_len; i++) {
    if (!strcmp(string_literals[i], str))
      return i;
  }
  string_literals[string_literals_len] = str;
  return string_literals_len++;
}

// 新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  cur->next = tok;
  return tok;
}

// 入力文字列pをトークナイズしてそれを返す
Token *tokenize(char *p) {
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    // 空白文字をスキップ
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (strncmp(p, "//", 2) == 0) {
      p += 2;
      while (*p != '\n')
        p++;
      continue;
    }

    if (strncmp(p, "/*", 2) == 0) {
      p += 2;
      while (strncmp(p, "*/", 2) != 0)
        p++;
      p += 2;
      continue;
    }

    if (*p == '"') {
      p++;
      cur = new_token(TK_STRING_LIT, cur, p);
      cur->len = 0;
      while (*p != '"') {
        cur->len++;
        p++;
      }
      p++;
      continue;
    }

    if (strncmp(p, ">=", 2) == 0 || strncmp(p, "<=", 2) == 0 ||
        strncmp(p, "==", 2) == 0 || strncmp(p, "!=", 2) == 0) {
      cur = new_token(TK_RESERVED, cur, p);
      cur->len = 2;
      p += 2;
      continue;
    }

    if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' ||
        *p == ')' || *p == '>' || *p == '<' || *p == '=' || *p == ';' ||
        *p == '{' || *p == '}' || *p == ',' || *p == '&' || *p == '[' ||
        *p == ']') {
      cur = new_token(TK_RESERVED, cur, p++);
      cur->len = 1;
      continue;
    }

    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p);
      cur->val = strtol(p, &p, 10);
      continue;
    }

    if (strncmp(p, "return", 6) == 0 && !is_alnum(p[6])) {
      cur = new_token(TK_RETURN, cur, p);
      p += 6;
      continue;
    }

    if (strncmp(p, "if", 2) == 0 && !is_alnum(p[2])) {
      cur = new_token(TK_IF, cur, p);
      p += 2;
      continue;
    }

    if (strncmp(p, "else", 4) == 0 && !is_alnum(p[4])) {
      cur = new_token(TK_ELSE, cur, p);
      p += 4;
      continue;
    }

    if (strncmp(p, "while", 5) == 0 && !is_alnum(p[5])) {
      cur = new_token(TK_WHILE, cur, p);
      p += 5;
      continue;
    }

    if (strncmp(p, "for", 3) == 0 && !is_alnum(p[3])) {
      cur = new_token(TK_FOR, cur, p);
      p += 3;
      continue;
    }

    if (strncmp(p, "int", 3) == 0 && !is_alnum(p[3])) {
      cur = new_token(TK_INT, cur, p);
      p += 3;
      continue;
    }

    if (strncmp(p, "char", 4) == 0 && !is_alnum(p[4])) {
      cur = new_token(TK_CHAR, cur, p);
      p += 4;
      continue;
    }

    if (strncmp(p, "sizeof", 6) == 0 && !is_alnum(p[6])) {
      cur = new_token(TK_SIZEOF, cur, p);
      p += 6;
      continue;
    }

    if (strncmp(p, "break", 5) == 0 && !is_alnum(p[5])) {
      cur = new_token(TK_BREAK, cur, p);
      p += 5;
      continue;
    }

    if (is_alnum(*p)) {
      cur = new_token(TK_IDENT, cur, p++);
      cur->len = 1;
      while (is_alnum(*p)) {
        cur->len++;
        p++;
      }
      continue;
    }

    error_at(p, "トークナイズできません");
  }

  new_token(TK_EOF, cur, p);
  return head.next;
}

Node *code[10000];

Node *expr();

Node *stmt();

Node *primary() {
  // 次のトークンが"("なら、"(" expr ")"のはず
  if (consume("(")) {
    if (consume("{")) {
      newScope();
      Node *node = calloc(1, sizeof(Node));
      node->kind = ND_BLOCK;
      NodeArray *nodes = new_node_array();
      while (!consume("}")) {
        if (consume(";")) {
          continue;
        }
        node_array_push(nodes, stmt());
      }
      node->nodes = nodes;
      expect(")");
      locals = locals->next;
      return node;
    } else {
      Node *node = expr();
      expect(")");
      return node;
    }
  }

  if (token->kind == TK_STRING_LIT) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_STRING_LIT;
    node->name = calloc(1, token->len + 1);
    strncpy(node->name, token->str, token->len);
    node->len = token->len;
    node->val = find_string_literal(node->name);
    node->type = pointer_to(char_type());
    token = token->next;
    return node;
  }

  Token *tok = consume_ident();
  if (tok) {
    Node *node = calloc(1, sizeof(Node));

    if (consume("(")) {
      node->kind = ND_CALL;
      node->name = tok->str;
      node->len = tok->len;
      NodeArray *args = new_node_array();
      while (!consume(")")) {
        node_array_push(args, expr());
        if (!consume(",")) {
          expect(")");
          break;
        }
      }
      node->nodes = args;
      Type *type = get_funtype(tok);
      if (!type)
        type = int_type();
      node->type = type;
      return node;
    }

    node->kind = ND_LVAR;
    LVar *lvar = find_lvar(tok);
    if (lvar) {
      node->offset = lvar->offset;
      node->name = tok->str;
      node->len = tok->len;
      node->type = lvar->type;
    } else {
      LVar *gvar = find_gvar(tok);
      if (gvar) {
        node->kind = ND_GVAR;
        node->name = tok->str;
        node->len = tok->len;
        node->type = gvar->type;
      } else
        error_at(tok->str, "定義されていない変数です");
    }
    return node;
  }

  // そうでなければ数値のはず
  return new_node_num(expect_number());
}

Node *unary_after() {
  Node *node = primary();
  while (consume("[")) {
    Node *index = expr();
    expect("]");
    node = new_node(ND_DEREF, new_node(ND_ADD, node, index), NULL);
  }
  return node;
}

Node *unary() {
  if (consume("+"))
    return unary();
  if (consume("-"))
    return new_node(ND_SUB, new_node_num(0), unary());
  if (consume("&"))
    return new_node(ND_ADDR, unary(), NULL);
  if (consume("*"))
    return new_node(ND_DEREF, unary(), NULL);
  if (consume_kind(TK_SIZEOF)) {
    Node *node = unary();
    return new_node_num(size_of(node->type));
  }
  return unary_after();
}

Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume("*"))
      node = new_node(ND_MUL, node, unary());
    else if (consume("/"))
      node = new_node(ND_DIV, node, unary());
    else
      return node;
  }
}

Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume("+"))
      node = new_node(ND_ADD, node, mul());
    else if (consume("-"))
      node = new_node(ND_SUB, node, mul());
    else
      return node;
  }
}

Node *relational() {
  Node *node = add();

  for (;;) {
    if (consume("<"))
      node = new_node(ND_LT, node, add());
    else if (consume("<="))
      node = new_node(ND_LE, node, add());
    else if (consume(">"))
      node = new_node(ND_LT, add(), node);
    else if (consume(">="))
      node = new_node(ND_LE, add(), node);
    else
      return node;
  }
}

Node *equality() {
  Node *node = relational();

  for (;;) {
    if (consume("=="))
      node = new_node(ND_EQ, node, relational());
    else if (consume("!="))
      node = new_node(ND_NE, node, relational());
    else
      return node;
  }
}

Node *assign() {
  Node *node = equality();
  if (consume("=")) {
    node = new_node(ND_ASSIGN, node, assign());
  }
  return node;
}

Node *expr() { return assign(); }

Type *type() {
  Type *type;
  if (consume_kind(TK_INT)) {
    type = int_type();
  } else if (consume_kind(TK_CHAR)) {
    type = char_type();
  } else {
    error_at(token->str, "型が不適切です");
  }
  while (consume("*")) {
    type = pointer_to(type);
  }
  return type;
}

Node *vardef() {
  Type *ty = type();
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_VARDEF;
  Token *tok = consume_ident();
  if (!tok)
    error_at(token->str, "変数名がありません");
  int size[20];
  int i = 0;
  while (consume("[")) {
    size[i++] = expect_number();
    expect("]");
  }
  for (int j = i - 1; j >= 0; j--) {
    ty = array_type(ty, size[j]);
  }
  LVar *lvar = find_lvar_in_current_scope(tok);
  if (lvar)
    error_at(tok->str, "変数が既に定義されています");
  lvar = calloc(1, sizeof(LVar));
  lvar->next = locals->locals;
  lvar->name = tok->str;
  lvar->len = tok->len;
  lvar->offset = next_offset ? next_offset : 8;
  lvar->type = ty;
  next_offset =
      lvar->offset + size_of(lvar->type) + (8 - size_of(lvar->type)) % 8;
  locals->locals = lvar;
  node->offset = lvar->offset;
  node->name = tok->str;
  node->len = tok->len;
  node->type = ty;
  return node;
}

Node *stmt() {
  Node *node;

  if (consume_kind(TK_RETURN)) {
    node = calloc(1, sizeof(Node));
    node->kind = ND_RETURN;
    node->lhs = expr();
    if (!consume(";"))
      error_at(token->str, "';'ではないトークンです");
  } else if (consume_kind(TK_IF)) {
    node = calloc(1, sizeof(Node));
    node->kind = ND_IF;
    expect("(");
    node->lhs = expr(); // cond
    expect(")");
    node->rhs = stmt(); // then
    if (consume_kind(TK_ELSE)) {
      node->else_stmt = stmt(); // else
    }
  } else if (consume_kind(TK_WHILE)) {
    node = calloc(1, sizeof(Node));
    node->kind = ND_WHILE;
    expect("(");
    node->lhs = expr(); // cond
    expect(")");
    node->rhs = stmt(); // body
  } else if (consume_kind(TK_FOR)) {
    newScope();
    node = calloc(1, sizeof(Node));
    node->kind = ND_FOR;
    expect("(");
    if (!consume(";")) {
      if (token->kind == TK_INT || token->kind == TK_CHAR) {
        node->lhs = vardef();
        expect("=");
        node->lhs->kind = ND_LVAR;
        node->lhs = new_node(ND_ASSIGN, node->lhs, assign());
      } else {
        node->lhs = expr();
      }

      expect(";");
    }
    if (!consume(";")) {
      node->rhs = expr();
      expect(";");
    }
    if (!consume(")")) {
      node->for_third = expr();
      expect(")");
    }
    node->for_body = stmt();
    locals = locals->next;
  } else if (consume("{")) {
    newScope();
    node = calloc(1, sizeof(Node));
    node->kind = ND_BLOCK;
    NodeArray *nodes = new_node_array();
    while (!consume("}")) {
      if (consume(";")) {
        continue;
      }
      node_array_push(nodes, stmt());
    }
    node->nodes = nodes;
    locals = locals->next;
  } else if (token->kind == TK_INT || token->kind == TK_CHAR) {
    node = vardef();
    if (consume("=")) {
      node->kind = ND_LVAR;
      node = new_node(ND_ASSIGN, node, assign());
    }
    expect(";");
  } else if (consume_kind(TK_BREAK)) {
    node = calloc(1, sizeof(Node));
    node->kind = ND_BREAK;
    if (!consume(";"))
      error_at(token->str, "';'ではないトークンです");
  } else {
    node = expr();
    if (!consume(";"))
      error_at(token->str, "';'ではないトークンです");
  }

  return node;
}

Node *fundef() {
  locals = NULL;
  newScope();
  Node *node = vardef();
  // 関数定義
  if (consume("(")) {
    locals = NULL;
    newScope();
    next_offset = 0;
    LVar *funtype = calloc(1, sizeof(LVar));
    funtype->next = funtypes;
    funtype->name = node->name;
    funtype->len = node->len;
    funtype->type = node->type;
    funtypes = funtype;
    node->kind = ND_FUNDEF;
    NodeArray *args = new_node_array();
    while (!consume(")")) {
      Node *arg = vardef();
      node_array_push(args, arg);
      if (!consume(",")) {
        expect(")");
        break;
      }
    }
    node->nodes = args;
    node->rhs = stmt();
    if (node->rhs->kind != ND_BLOCK)
      error_at(token->str, "関数定義が不適切です");
    int offset = next_offset;
    if (offset % 16)
      offset += 16 - offset % 16;
    node->offset = offset;
  } else {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = TK_IDENT;
    tok->str = node->name;
    tok->len = node->len;
    LVar *gvar = find_gvar(tok);
    if (gvar)
      error_at(node->name, "グローバル変数が既に定義されています");
    gvar = calloc(1, sizeof(LVar));
    gvar->next = globals;
    gvar->name = node->name;
    gvar->len = node->len;
    gvar->type = node->type;
    globals = gvar;
    expect(";");
  }
  return node;
}

void program() {
  int i = 0;
  while (!at_eof()) {
    code[i++] = fundef();
  }
  code[i] = NULL;
}
