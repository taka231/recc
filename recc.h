#include <stdio.h>
// トークンの種類
typedef enum {
  TK_RESERVED,   // 記号
  TK_IDENT,      // 識別子
  TK_NUM,        // 整数トークン
  TK_STRING_LIT, // 文字列リテラル
  TK_EOF,        // 入力の終わりを表すトークン
  TK_RETURN,     // return
  TK_IF,         // if
  TK_ELSE,       // else
  TK_WHILE,      // while
  TK_FOR,        // for
  TK_INT,        // int
  TK_CHAR,       // char
  TK_BREAK,      // break
  TK_SIZEOF,     // sizeof
} TokenKind;

typedef struct Token Token;

// トークン型
struct Token {
  TokenKind kind; // トークンの型
  Token *next;    // 次の入力トークン
  int val;        // kindがTK_NUMの場合、その数値
  char *str;      // トークン文字列
  int len;        // トークンの長さ
};

// 現在着目しているトークン
extern Token *token;

typedef struct Type Type;

struct Type {
  enum { INT, PTR, ARRAY, CHAR } ty;
  Type *ptr_to;
  size_t array_size;
};

int size_of(Type *type);

// 抽象構文木のノードの種類
typedef enum {
  ND_ADD,        // +
  ND_SUB,        // -
  ND_MUL,        // *
  ND_DIV,        // /
  ND_EQ,         // ==
  ND_NE,         // !=
  ND_LT,         // <
  ND_LE,         // <=
  ND_ASSIGN,     // =
  ND_ADDR,       // &
  ND_DEREF,      // *
  ND_LVAR,       // ローカル変数
  ND_GVAR,       // グローバル変数
  ND_RETURN,     // return
  ND_NUM,        // 整数
  ND_STRING_LIT, // 文字列リテラル
  ND_IF,         // if
  ND_WHILE,      // while
  ND_FOR,        // for
  ND_BLOCK,      // { ... }
  ND_CALL,       // 関数呼び出し
  ND_VARDEF,     // 変数定義
  ND_FUNDEF,      // 関数定義
  ND_BREAK,      // break
} NodeKind;

typedef struct Node Node;

typedef struct NodeArray {
  int len;
  int capacity;
  Node **data;
} NodeArray;

// 抽象構文木のノードの型
struct Node {
  NodeKind kind;   // ノードの型
  Node *lhs;       // 左辺
  Node *rhs;       // 右辺
  Node *else_stmt; // else
  Node *for_third; // forの条件部の3番目の式
  Node *for_body;  // for
  int val;         // kindがND_NUMの場合のみ使う
  int offset;      // kindがND_LVARの場合のみ使う
  char *name;
  int len;
  NodeArray *nodes; // ND_BLOCK or ND_FUNDEF or ND_CALL
  Type *type;       //
};

extern Node *code[10000];

extern char *string_literals[10000];
extern int string_literals_len;

typedef struct LVar LVar;

// ローカル変数の型
struct LVar {
  LVar *next; // 次の変数かNULL
  char *name; // 変数の名前
  int len;    // 名前の長さ
  int offset; // RBPからのオフセット
  Type *type; // 型
};

typedef struct Scope Scope;

struct Scope {
  Scope *next;
  LVar *locals;
};

// ローカル変数
extern Scope *locals;

extern int next_offset;

// グローバル変数
extern LVar *globals;

// 関数の返り値の型のマップ
extern LVar *funtypes;

// 変数を名前で検索する。見つからなかった場合はNULLを返す。
LVar *find_lvar(Token *tok);

// 入力プログラム
extern char *user_input;

void error(char *fmt, ...);

char *read_file(char *path);

extern char *filename;

void gen_definition(Node *node);

// 入力文字列pをトークナイズしてそれを返す
Token *tokenize(char *p);

void program();

// debug
void print_node(Node *node);
