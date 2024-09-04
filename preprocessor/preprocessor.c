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
  PREPROCESSOR_PARENTHESES_NODE,
  PREPROCESSOR_JOINED_NODE,
  PREPROCESSOR_TENARY_NODE,
};

struct preprocessor_node {
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

void preprocessor_exec_error(struct compile_process *compiler,
                             const char *message) {
  compiler_error(compiler, "#error %s", message);
}

void preprocessor_exec_warning(struct compile_process *compiler,
                               const char *message) {
  compiler_warning(compiler, "#warning %s", message);
}

struct preprocessor_included_file *
preprocessor_add_included_file(struct preprocessor *preprocessor,
                               const char *filename) {
  struct preprocessor_included_file *included_file =
      malloc(sizeof(struct preprocessor_included_file));
  strncpy(included_file->filename, filename, sizeof(included_file->filename));
  vector_push(preprocessor->includes, &included_file);
  return included_file;
}

void preprocessor_create_static_include(
    struct preprocessor *preprocessor, const char *filename,
    PREPROCESSOR_STATIC_INCLUDE_HANDLER_POST_CREATION creation_handler) {
  struct preprocessor_included_file *included_file =
      preprocessor_add_included_file(preprocessor, filename);
  creation_handler(preprocessor, included_file);
}

bool preprocessor_is_keyword(const char *type) { return S_EQ(type, "defined"); }

struct vector *preprocessor_build_value_vector_for_integer(int value) {
  struct vector *token_vec = vector_create(sizeof(struct token));
  struct token t1 = {};
  t1.type = TOKEN_TYPE_NUMBER;
  t1.llnum = value;
  vector_push(token_vec, &t1);
  return token_vec;
}

void preprocessor_token_vec_push_keyword_and_identifier(
    struct vector *token_vec, const char *keyword, const char *identifier) {
  struct token t1 = {};
  t1.type = TOKEN_TYPE_KEYWORD;
  t1.sval = keyword;
  vector_push(token_vec, &t1);

  struct token t2 = {};
  t2.type = TOKEN_TYPE_IDENTIFIER;
  t2.sval = identifier;
  vector_push(token_vec, &t2);
}

struct preprocessor_node *
preprocessor_node_create(struct preprocessor_node *node) {
  struct preprocessor_node *res = malloc(sizeof(struct preprocessor_node));
  memcpy(res, node, sizeof(struct preprocessor_node));
  return res;
}

int preprocessor_definition_argument_exists(struct preprocessor_definition *def,
                                            const char *arg) {
  vector_set_peek_pointer(def->standard.args, 0);
  int i = 0;
  const char *current = vector_peek(def->standard.args);
  while (current) {
    if (S_EQ(current, arg)) {
      return i;
    }

    i++;
    current = vector_peek(def->standard.args);
  }

  return -1;
}

struct preprocessor_function_arg *
preprocessor_function_argument_at(struct preprocessor_function_args *args,
                                  int index) {
  return vector_at(args->args, index);
}

void preprocessor_token_push_to_function_arguments(
    struct preprocessor_function_args *args, struct token *token) {
  struct preprocessor_function_arg arg = {};
  arg.tokens = vector_create(sizeof(struct token));
  vector_push(arg.tokens, token);
  vector_push(args->args, &arg);
}

void preprocessor_function_argument_push_to_vec(
    struct preprocessor_function_arg *arg, struct vector *vec) {
  vector_set_peek_pointer(arg->tokens, 0);
  struct token *token = vector_peek(arg->tokens);
  while (token) {
    vector_push(vec, token);
    token = vector_peek(arg->tokens);
  }
}

void preprocessor_token_push_to_dst(struct vector *token_vec,
                                    struct token *token) {
  struct token t = *token;
  vector_push(token_vec, &t);
}

void preprocessor_token_push_dst(struct compile_process *compiler,
                                 struct token *token) {
  preprocessor_token_push_to_dst(compiler->token_vec, token);
}

void preprocessor_token_vec_push_src_to_dst(struct compile_process *compiler,
                                            struct vector *src_vec,
                                            struct vector *dst_vec) {
  vector_set_peek_pointer(src_vec, 0);
  struct token *token = vector_peek(src_vec);
  while (token) {
    vector_push(dst_vec, token);
    token = vector_peek(src_vec);
  }
}

void preprocessor_token_vec_push_src(struct compile_process *compiler,
                                     struct vector *src_vec) {
  preprocessor_token_vec_push_src_to_dst(compiler, src_vec,
                                         compiler->token_vec);
}

void preprocessor_token_vec_push_src_token(struct compile_process *compiler,
                                           struct token *token) {
  vector_push(compiler->token_vec, token);
}

void preprocessor_init(struct preprocessor *preprocessor) {
  memset(preprocessor, 0, sizeof(struct preprocessor));
  preprocessor->defs = vector_create(sizeof(struct preprocessor_definition *));
  preprocessor->includes =
      vector_create(sizeof(struct preprocessor_included_file *));
#warning "create proprocessor default definitions"
}

struct preprocessor *preprocessor_create(struct compile_process *compiler) {
  struct preprocessor *preprocessor = malloc(sizeof(struct preprocessor));
  preprocessor_init(preprocessor);
  preprocessor->compiler = compiler;
  return preprocessor;
}

struct token *preprocessor_next_token(struct compile_process *compiler) {
  return vector_peek(compiler->token_vec_original);
}

void *preprocessor_handle_number_token(struct expressionable *expressionable) {
  struct token *token = expressionable_token_next(expressionable);
  return preprocessor_node_create(&(struct preprocessor_node){
      .type = PREPROCESSOR_NUMBER_NODE, .const_val = token->llnum});
}

void *
preprocessor_handle_identifier_token(struct expressionable *expressionable) {
  struct token *token = expressionable_token_next(expressionable);
  bool is_preprocessor_keyword = preprocessor_is_keyword(token->sval);
  int type = PREPROCESSOR_IDENTIFIER_NODE;
  if (is_preprocessor_keyword) {
    type = PREPROCESSOR_KEYWORD_NODE;
  }

  return preprocessor_node_create(
      &(struct preprocessor_node){.type = type, .sval = token->sval});
}

void preprocessor_make_unary_node(struct expressionable *expressionable,
                                  const char *op, void *operand) {
  struct preprocessor_node *operand_node = operand;
  void *unary_node = preprocessor_node_create(
      &(struct preprocessor_node){.type = PREPROCESSOR_UNARY_NODE,
                                  .unary_node = {
                                      .op = op,
                                      .operand = operand_node,
                                  }});
  expressionable_node_push(expressionable, unary_node);
}

void preprocessor_make_expression_node(struct expressionable *expressionable,
                                       void *left, void *right,
                                       const char *op) {
  struct preprocessor_node exp_node = {
      .type = PREPROCESSOR_EXPRESSION_NODE,
      .exp =
          {
              .left = left,
              .right = right,
              .op = op,
          },
  };
  expressionable_node_push(expressionable, preprocessor_node_create(&exp_node));
}

void preprocessor_make_parentheses_node(struct expressionable *expressionable,
                                        void *exp) {
  struct preprocessor_node *exp_node = exp;
  struct preprocessor_node paren_node = {
      .type = PREPROCESSOR_PARENTHESES_NODE,
      .paren_node =
          {
              .exp = exp_node,
          },
  };
  expressionable_node_push(expressionable,
                           preprocessor_node_create(&paren_node));
}

void preprocessor_make_tenary_node(struct expressionable *expressionable,
                                   void *true_node_ptr, void *false_node_ptr) {
  struct preprocessor_node *true_node = true_node_ptr;
  struct preprocessor_node *false_node = false_node_ptr;
  expressionable_node_push(expressionable,
                           preprocessor_node_create(&(struct preprocessor_node){
                               .type = PREPROCESSOR_TENARY_NODE,
                               .tenary_node =
                                   {
                                       .true_node = true_node,
                                       .false_node = false_node,
                                   },
                           }));
}

int preprocessor_get_node_type(struct expressionable *expressionable,
                               void *node) {
  int generic_type = EXPRESSIONABLE_GENERIC_TYPE_NON_GENERIC;
  struct preprocessor_node *preprocessor_node = node;
  switch (preprocessor_node->type) {
  case PREPROCESSOR_NUMBER_NODE:
    generic_type = EXPRESSIONABLE_GENERIC_TYPE_NUMBER;
    break;

  case PREPROCESSOR_IDENTIFIER_NODE:
  case PREPROCESSOR_KEYWORD_NODE:
    generic_type = EXPRESSIONABLE_GENERIC_TYPE_IDENTIFIER;
    break;

  case PREPROCESSOR_UNARY_NODE:
    generic_type = EXPRESSIONABLE_GENERIC_TYPE_UNARY;
    break;

  case PREPROCESSOR_EXPRESSION_NODE:
    generic_type = EXPRESSIONABLE_GENERIC_TYPE_EXPRESSION;
    break;

  case PREPROCESSOR_PARENTHESES_NODE:
    generic_type = EXPRESSIONABLE_GENERIC_TYPE_PARENTHESES;
    break;
  }

  return generic_type;
}

void *preprocessor_get_node_left(struct expressionable *expressionable,
                                 void *node) {
  struct preprocessor_node *preprocessor_node = node;
  return preprocessor_node->exp.left;
}

void *preprocessor_get_node_right(struct expressionable *expressionable,
                                  void *node) {
  struct preprocessor_node *preprocessor_node = node;
  return preprocessor_node->exp.right;
}

const char *preprocessor_get_node_op(struct expressionable *expressionable,
                                     void *node) {
  struct preprocessor_node *preprocessor_node = node;
  return preprocessor_node->exp.op;
}

void **preprocessor_get_left_node_address(struct expressionable *expressionable,
                                          void *node) {
  struct preprocessor_node *preprocessor_node = node;
  return (void **)&preprocessor_node->exp.left;
}

void **
preprocessor_get_right_node_address(struct expressionable *expressionable,
                                    void *node) {
  struct preprocessor_node *preprocessor_node = node;
  return (void **)&preprocessor_node->exp.right;
}

void preprocessor_set_expression_node(struct expressionable *expressionable,
                                      void *node, void *left, void *right,
                                      const char *op) {
  struct preprocessor_node *preprocessor_node = node;
  preprocessor_node->exp.left = left;
  preprocessor_node->exp.right = right;
  preprocessor_node->exp.op = op;
}

bool preprocessor_should_join_nodes(struct expressionable *expressionable,
                                    void *prev_node, void *node) {
  return true;
}

void *preprocessor_join_nodes(struct expressionable *expressionable, void *prev,
                              void *node) {
  struct preprocessor_node *prev_node = prev;
  struct preprocessor_node *current_node = node;
  struct preprocessor_node joined_node = {
      .type = PREPROCESSOR_JOINED_NODE,
      .joined_node =
          {
              .left = prev_node,
              .right = current_node,
          },
  };
  return preprocessor_node_create(&joined_node);
}

bool preprocessor_expecting_additional_node(
    struct expressionable *expressionable, void *node) {
  struct preprocessor_node *preprocessor_node = node;
  return preprocessor_node->type == PREPROCESSOR_KEYWORD_NODE &&
         S_EQ(preprocessor_node->sval, "defined");
}

bool preprocessor_is_custom_operator(struct expressionable *expressionable,
                                     struct token *token) {
  return false;
}

struct expressionable_config preprocessor_expressionable_config = {
    .callbacks = {
        .handle_number = preprocessor_handle_number_token,
        .handle_identifier = preprocessor_handle_identifier_token,
        .make_unary_node = preprocessor_make_unary_node,
        .make_expression_node = preprocessor_make_expression_node,
        .make_parentheses_node = preprocessor_make_parentheses_node,
        .make_tenary_node = preprocessor_make_tenary_node,
        .get_node_type = preprocessor_get_node_type,
        .get_node_left = preprocessor_get_node_left,
        .get_node_right = preprocessor_get_node_right,
        .get_node_op = preprocessor_get_node_op,
        .get_left_node_address = preprocessor_get_left_node_address,
        .get_right_node_address = preprocessor_get_right_node_address,
        .set_expression_node = preprocessor_set_expression_node,
        .should_join_nodes = preprocessor_should_join_nodes,
        .join_nodes = preprocessor_join_nodes,
        .expecting_additional_node = preprocessor_expecting_additional_node,
        .is_custom_operator = preprocessor_is_custom_operator,
    }};

bool preprocessor_is_preprocessor_keyword(const char *keyword) {
  return S_EQ(keyword, "define") || S_EQ(keyword, "undef") ||
         S_EQ(keyword, "warning") || S_EQ(keyword, "error") ||
         S_EQ(keyword, "if") || S_EQ(keyword, "ifdef") ||
         S_EQ(keyword, "ifndef") || S_EQ(keyword, "elif") ||
         S_EQ(keyword, "else") || S_EQ(keyword, "endif") ||
         S_EQ(keyword, "include") || S_EQ(keyword, "typedef");
}

bool preprocessor_token_is_preprocessor_keyword(struct token *token) {
  return token->type == TOKEN_TYPE_IDENTIFIER ||
         token->type == TOKEN_TYPE_KEYWORD &&
             preprocessor_is_preprocessor_keyword(token->sval);
}

bool preprocessor_token_is_define(struct token *token) {
  if (!preprocessor_token_is_preprocessor_keyword(token)) {
    return false;
  }

  return S_EQ(token->sval, "define");
}

void preprocessor_multi_value_insert_to_vector(struct compile_process *compiler,
                                               struct vector *vec) {
  struct token *value_token = preprocessor_next_token(compiler);
  while (value_token) {
    if (value_token->type == TOKEN_TYPE_NEWLINE) {
      break;
    }

    if (token_is_symbol(value_token, '\\')) {
      // ignore
      preprocessor_next_token(compiler);
      value_token = preprocessor_next_token(compiler);
      continue;
    }

    vector_push(vec, value_token);
    value_token = preprocessor_next_token(compiler);
  }
}

void preprocessor_definition_remove(struct preprocessor *preprocessor,
                                    const char *name) {
  vector_set_peek_pointer(preprocessor->defs, 0);
  struct preprocessor_definition *cur_def = vector_peek_ptr(preprocessor->defs);
  while (cur_def) {
    if (S_EQ(cur_def->name, name)) {
      // remove definition
      vector_pop_last_peek(preprocessor->defs);
      return;
    }

    cur_def = vector_peek_ptr(preprocessor->defs);
  }
}

struct preprocessor_definition *
preprocessor_definition_create(const char *name, struct vector *value,
                               struct vector *args,
                               struct preprocessor *preprocessor) {
  // unset definition if it's already created
  preprocessor_definition_remove(preprocessor, name);

  struct preprocessor_definition *def =
      malloc(sizeof(struct preprocessor_definition));
  def->type = PREPROCESSOR_DEFINITION_STANDARD;
  def->name = name;
  def->standard.value = value;
  def->standard.args = args;
  def->preprocessor = preprocessor;
  if (args && vector_count(def->standard.args)) {
    def->type = PREPROCESSOR_DEFINITION_MACRO_FUNCTION;
  }

  vector_push(preprocessor->defs, &def);
  return def;
}

void preprocessor_handle_definition_token(struct compile_process *compiler) {
  struct token *name_token = preprocessor_next_token(compiler);
  struct vector *args = vector_create(sizeof(const char *));
#warning "handle macro function arguments"

  struct vector *value_token_vec = vector_create(sizeof(struct token));
  preprocessor_multi_value_insert_to_vector(compiler, value_token_vec);

  struct preprocessor *preprocessor = compiler->preprocessor;
  preprocessor_definition_create(name_token->sval, value_token_vec, args,
                                 preprocessor);
}

int preprocessor_handle_hashtag_token(struct compile_process *compiler,
                                      struct token *token) {
  bool is_preprocessed = false;
  struct token *next_token = preprocessor_next_token(compiler);
  if (preprocessor_token_is_define(next_token)) {
    preprocessor_handle_definition_token(compiler);
    is_preprocessed = true;
  }

  return is_preprocessed;
}

void preprocessor_handle_symbol(struct compile_process *compiler,
                                struct token *token) {
  int is_preprocessed = false;
  if (token->cval == '#') {
    is_preprocessed = preprocessor_handle_hashtag_token(compiler, token);
  }

  if (!is_preprocessed) {
    preprocessor_token_push_dst(compiler, token);
  }
}

void preprocessor_handle_token(struct compile_process *compiler,
                               struct token *token) {
  switch (token->type) {
  case TOKEN_TYPE_NEWLINE:
    // ignored
    break;

  case TOKEN_TYPE_SYMBOL:
    preprocessor_handle_symbol(compiler, token);
    break;

  default:
    preprocessor_token_push_dst(compiler, token);
    break;
  }
}

int preprocessor_run(struct compile_process *compiler) {
#warning "add source file as an included file"
  vector_set_peek_pointer(compiler->token_vec_original, 0);
  struct token *token = preprocessor_next_token(compiler);
  while (token) {
    preprocessor_handle_token(compiler, token);
    token = preprocessor_next_token(compiler);
  }

  return PREPROCESS_ALL_OK;
}
