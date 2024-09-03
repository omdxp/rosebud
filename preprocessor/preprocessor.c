#include "compiler.h"
#include "helpers/buffer.h"
#include "helpers/vector.h"

enum {
  TYPEDEF_TYPE_STANDARD,
  TYPEDEF_TYPE_STRUCT_TYPEDEF, // typedef struct { ... } name;
};

struct typedef_type {
  int type;
  const char *def_name;
  struct vector *value;
  struct typedef_struct {
    const char *name;
  } _struct;
};

enum {
  PREPROCESSOR_FLAG_EVALUATE_NODE = 0b00000001,
};

enum {
  PREPROCESSOR_NUMBER_NODE,
  PREPROCESSOR_IDENTIFIER_NODE,
  PREPROCESSOR_KEYWORD_NODE,
  PREPROCESSOR_UNARY_NODE,
  PREPROCESSOR_EXPRESSION_NODE,
  PREPROCESSOR_JOINED_NODE,
  PREPROCESSOR_TENARY_NODE,
};

struct preprocess_node {
  int type;
  struct preprocessor_const_val {
    union {
      char cval;
      unsigned int inum;
      long lnum;
      long long llnum;
      unsigned long ulnum;
      unsigned long long ullnum;
    };
  } const_val;

  union {
    struct preprocessor_exp_node {
      struct preprocessor_node *left;
      struct preprocessor_node *right;
      const char *op;
    } exp;

    struct preprocessor_unary_node {
      const char *op;
      struct preprocessor_node *operand;
      struct preprocessor_unary_indirection {
        int depth;
      } indirection;
    } unary_node;

    struct preprocessor_parentheses_node {
      struct preprocessor_node *exp;
    } paren_node;

    struct preprocessor_joined_node {
      struct preprocessor_node *left;
      struct preprocessor_node *right;
    } joined_node;

    struct preprocessor_tenary_node {
      struct preprocessor_node *true_node;
      struct preprocessor_node *false_node;
    } tenary_node;
  };

  const char *sval;
};
