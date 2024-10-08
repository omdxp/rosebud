#include "compiler.h"
#include "helpers/buffer.h"
#include "helpers/vector.h"
#include <assert.h>

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

void preprocessor_handle_token(struct compile_process *compiler,
                               struct token *token);
int preprocessor_evaluate(struct compile_process *compiler,
                          struct preprocessor_node *root_node);
int preprocessor_parse_evaluate(struct compile_process *compiler,
                                struct vector *token_vec);
int preprocessor_handle_identifier_for_token_vector(
    struct compile_process *compiler, struct vector *src_vec,
    struct vector *dst_vec, struct token *token);
void preprocessor_token_vec_push_src_resolve_definition(
    struct compile_process *compiler, struct vector *src_vec,
    struct vector *dst_vec, struct token *token);
struct vector *
preprocessor_definition_value(struct preprocessor_definition *def);
void preprocessor_macro_function_push_something(
    struct compile_process *compiler, struct preprocessor_definition *def,
    struct preprocessor_function_args *args, struct token *arg_token,
    struct vector *def_token_vec, struct vector *value_vec_target);

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

void *preprocessor_node_create(struct preprocessor_node *node) {
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
  preprocessor_create_defs(preprocessor);
}

struct preprocessor *preprocessor_create(struct compile_process *compiler) {
  struct preprocessor *preprocessor = malloc(sizeof(struct preprocessor));
  preprocessor_init(preprocessor);
  preprocessor->compiler = compiler;
  return preprocessor;
}

struct token *preprocessor_prev_token(struct compile_process *compiler) {
  return vector_peek_at(compiler->token_vec_original,
                        compiler->token_vec_original->pindex - 1);
}

struct token *preprocessor_next_token(struct compile_process *compiler) {
  return vector_peek(compiler->token_vec_original);
}

struct token *
preprocessor_next_token_no_increment(struct compile_process *compiler) {
  return vector_peek_no_increment(compiler->token_vec_original);
}

struct token *
preprocessor_peek_next_token_skip_nl(struct compile_process *compiler) {
  struct token *token = preprocessor_next_token_no_increment(compiler);
  while (token && token->type == TOKEN_TYPE_NEWLINE) {
    token = preprocessor_next_token(compiler);
  }

  token = preprocessor_next_token_no_increment(compiler);
  return token;
}

struct token *preprocessor_peek_next_token_with_vector_no_increment(
    struct compile_process *compiler, struct vector *priority_token_vec,
    bool overflow_use_compiler_tokens) {
  struct token *token = vector_peek_no_increment(priority_token_vec);
  if (!token && overflow_use_compiler_tokens) {
    token = preprocessor_peek_next_token_skip_nl(compiler);
  }

  return token;
}

struct token *
preprocessor_peek_next_token_with_vector(struct compile_process *compiler,
                                         struct vector *priority_token_vec,
                                         bool overflow_use_compiler_tokens) {
  struct token *token = vector_peek(priority_token_vec);
  if (!token && overflow_use_compiler_tokens) {
    token = preprocessor_peek_next_token_skip_nl(compiler);
  }

  return token;
}

void *preprocessor_handle_number_token(struct expressionable *expressionable) {
  struct token *token = expressionable_token_next(expressionable);
  return preprocessor_node_create(&(struct preprocessor_node){
      .type = PREPROCESSOR_NUMBER_NODE, .const_val.llnum = token->llnum});
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

bool preprocessor_token_is_undef(struct token *token) {
  if (!preprocessor_token_is_preprocessor_keyword(token)) {
    return false;
  }

  return S_EQ(token->sval, "undef");
}

bool preprocessor_token_is_warning(struct token *token) {
  if (!preprocessor_token_is_preprocessor_keyword(token)) {
    return false;
  }

  return S_EQ(token->sval, "warning");
}

bool preprocessor_token_is_error(struct token *token) {
  if (!preprocessor_token_is_preprocessor_keyword(token)) {
    return false;
  }

  return S_EQ(token->sval, "error");
}

bool preprocessor_token_is_ifdef(struct token *token) {
  if (!preprocessor_token_is_preprocessor_keyword(token)) {
    return false;
  }

  return S_EQ(token->sval, "ifdef");
}

bool preprocessor_token_is_ifndef(struct token *token) {
  if (!preprocessor_token_is_preprocessor_keyword(token)) {
    return false;
  }

  return S_EQ(token->sval, "ifndef");
}

bool preprocessor_token_is_if(struct token *token) {
  if (!preprocessor_token_is_preprocessor_keyword(token)) {
    return false;
  }

  return S_EQ(token->sval, "if");
}

bool preprocessor_token_is_typedef(struct token *token) {
  if (!preprocessor_token_is_preprocessor_keyword(token)) {
    return false;
  }

  return S_EQ(token->sval, "typedef");
}

bool preprocessor_token_is_include(struct token *token) {
  if (!preprocessor_token_is_preprocessor_keyword(token)) {
    return false;
  }

  return S_EQ(token->sval, "include");
}

struct buffer *
preprocessor_multi_value_string(struct compile_process *compiler) {
  struct buffer *str_buf = buffer_create();
  struct token *value_token = preprocessor_next_token(compiler);
  while (value_token) {
    if (value_token->type == TOKEN_TYPE_NEWLINE) {
      break;
    }

    if (token_is_symbol(value_token, '\\')) {
      // skip next token
      preprocessor_next_token(compiler);
      value_token = preprocessor_next_token(compiler);
      continue;
    }

    buffer_printf(str_buf, "%s", value_token->sval);
    value_token = preprocessor_next_token(compiler);
  }

  return str_buf;
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

struct preprocessor_definition *preprocessor_definition_create_native(
    const char *name, PREPROCESSOR_DEFINITION_NATIVE_CALL_EVALUATE evaluate,
    PREPROCESSOR_DEFINITION_NATIVE_CALL_VALUE value,
    struct preprocessor *preprocessor) {
  struct preprocessor_definition *def =
      malloc(sizeof(struct preprocessor_definition));
  def->type = PREPROCESSOR_DEFINITION_NATIVE_CALLBACK;
  def->name = name;
  def->native.evaluate = evaluate;
  def->native.value = value;
  def->preprocessor = preprocessor;
  vector_push(preprocessor->defs, &def);
  return def;
}

struct preprocessor_definition *
preprocessor_definition_create_typedef(const char *name,
                                       struct vector *value_vec,
                                       struct preprocessor *preprocessor) {
  struct preprocessor_definition *def =
      malloc(sizeof(struct preprocessor_definition));
  def->type = PREPROCESSOR_DEFINITION_TYPEDEF;
  def->name = name;
  def->_typedef.value = value_vec;
  def->preprocessor = preprocessor;
  vector_push(preprocessor->defs, &def);
  return def;
}

struct preprocessor_definition *
preprocessor_get_definition(struct preprocessor *preprocessor,
                            const char *name) {
  vector_set_peek_pointer(preprocessor->defs, 0);
  struct preprocessor_definition *def = vector_peek_ptr(preprocessor->defs);
  while (def) {
    if (S_EQ(def->name, name)) {
      return def;
    }

    def = vector_peek_ptr(preprocessor->defs);
  }

  return NULL;
}

struct vector *preprocessor_definition_value_for_standard(
    struct preprocessor_definition *def) {
  return def->standard.value;
}

struct vector *preprocessor_definition_value_for_typedef_or_other(
    struct preprocessor_definition *def) {
  if (def->type != PREPROCESSOR_DEFINITION_TYPEDEF) {
    return preprocessor_definition_value(def);
  }

  return def->_typedef.value;
}

struct vector *
preprocessor_definition_value_for_typedef(struct preprocessor_definition *def) {
  return preprocessor_definition_value_for_typedef_or_other(def);
}

struct vector *preprocessor_definition_value_for_native(
    struct preprocessor_definition *def,
    struct preprocessor_function_args *args) {
  return def->native.value(def, args);
}

struct vector *preprocessor_definition_value_with_arguments(
    struct preprocessor_definition *def,
    struct preprocessor_function_args *args) {
  if (def->type == PREPROCESSOR_DEFINITION_NATIVE_CALLBACK) {
    return preprocessor_definition_value_for_native(def, args);
  } else if (def->type == PREPROCESSOR_DEFINITION_TYPEDEF) {
    return preprocessor_definition_value_for_typedef(def);
  }

  return preprocessor_definition_value_for_standard(def);
}

struct vector *
preprocessor_definition_value(struct preprocessor_definition *def) {
  return preprocessor_definition_value_with_arguments(def, NULL);
}

int preprocessor_parse_evaluate_token(struct compile_process *compiler,
                                      struct token *token) {
  struct vector *token_vec = vector_create(sizeof(struct token));
  vector_push(token_vec, token);
  return preprocessor_parse_evaluate(compiler, token_vec);
}

int preprocessor_definition_evaluated_value_for_standard(
    struct preprocessor_definition *def) {
  struct token *token = vector_back(def->standard.value);
  if (token->type == TOKEN_TYPE_IDENTIFIER) {
    return preprocessor_parse_evaluate_token(def->preprocessor->compiler,
                                             token);
  }

  if (token->type != TOKEN_TYPE_NUMBER) {
    compiler_error(def->preprocessor->compiler,
                   "Definition must hold number value");
  }

  return token->llnum;
}

int preprocess_evaluated_value_for_native(
    struct preprocessor_definition *def,
    struct preprocessor_function_args *args) {
  return def->native.evaluate(def, args);
}

int preprocessor_definition_evaluated_value(
    struct preprocessor_definition *def,
    struct preprocessor_function_args *args) {
  if (def->type == PREPROCESSOR_DEFINITION_STANDARD) {
    return preprocessor_definition_evaluated_value_for_standard(def);
  } else if (def->type == PREPROCESSOR_DEFINITION_NATIVE_CALLBACK) {
    return preprocess_evaluated_value_for_native(def, args);
  }

  compiler_error(def->preprocessor->compiler, "Cannot evaluate to number");
  return 0;
}

bool preprocessor_is_next_macro_arguments(struct compile_process *compiler) {
  bool res = false;
  vector_save(compiler->token_vec_original);
  struct token *last_token = preprocessor_prev_token(compiler);
  struct token *cur_token = preprocessor_next_token(compiler);
  if (token_is_operator(cur_token, "(") &&
      (!last_token || !last_token->whitespace)) {
    res = true;
  }

  vector_restore(compiler->token_vec_original);
  return res;
}

void preprocessor_parse_macro_argument_declaration(
    struct compile_process *compiler, struct vector *args) {
  if (token_is_operator(preprocessor_next_token_no_increment(compiler), "(")) {
    // skip (
    preprocessor_next_token(compiler);
    struct token *next_token = preprocessor_next_token(compiler);
    while (!token_is_symbol(next_token, ')')) {
      if (next_token->type != TOKEN_TYPE_IDENTIFIER) {
        compiler_error(compiler, "expected identifier");
      }

      vector_push(args, (void *)next_token->sval);
      next_token = preprocessor_next_token(compiler);
      if (!token_is_operator(next_token, ",") &&
          !token_is_symbol(next_token, ')')) {
        compiler_error(compiler, "expected , or )");
      }

      if (token_is_symbol(next_token, ')')) {
        break;
      }

      // skip comma
      next_token = preprocessor_next_token(compiler);
    }
  }
}

void preprocessor_handle_definition_token(struct compile_process *compiler) {
  struct token *name_token = preprocessor_next_token(compiler);
  struct vector *args = vector_create(sizeof(const char *));

  if (preprocessor_is_next_macro_arguments(compiler)) {
    preprocessor_parse_macro_argument_declaration(compiler, args);
  }

  struct vector *value_token_vec = vector_create(sizeof(struct token));
  preprocessor_multi_value_insert_to_vector(compiler, value_token_vec);

  struct preprocessor *preprocessor = compiler->preprocessor;
  preprocessor_definition_create(name_token->sval, value_token_vec, args,
                                 preprocessor);
}

void preprocessor_handle_undef_token(struct compile_process *compiler) {
  struct token *name_token = preprocessor_next_token(compiler);
  preprocessor_definition_remove(compiler->preprocessor, name_token->sval);
}

void preprocessor_handle_warning_token(struct compile_process *compiler) {
  struct buffer *str_buf = preprocessor_multi_value_string(compiler);
  preprocessor_exec_warning(compiler, buffer_ptr(str_buf));
}

void preprocessor_handle_error_token(struct compile_process *compiler) {
  struct buffer *str_buf = preprocessor_multi_value_string(compiler);
  preprocessor_exec_error(compiler, buffer_ptr(str_buf));
}

struct token *
preprocessor_hashtag_and_identifier(struct compile_process *compiler,
                                    const char *str) {
  if (!preprocessor_next_token_no_increment(compiler)) {
    return NULL;
  }

  if (!token_is_symbol(preprocessor_next_token_no_increment(compiler), '#')) {
    return NULL;
  }

  vector_save(compiler->token_vec_original);
  // skip #
  preprocessor_next_token(compiler);

  struct token *token = preprocessor_next_token_no_increment(compiler);
  if ((token_is_identifier(token) && S_EQ(token->sval, str)) ||
      token_is_keyword(token, str)) {
    // pop off target token
    preprocessor_next_token(compiler);

    // purge vector save
    vector_save_purge(compiler->token_vec_original);
    return token;
  }

  vector_restore(compiler->token_vec_original);
  return NULL;
}

bool preprocessor_is_hashtag_and_any_starting_if(
    struct compile_process *compiler) {
  return preprocessor_hashtag_and_identifier(compiler, "if") ||
         preprocessor_hashtag_and_identifier(compiler, "ifdef") ||
         preprocessor_hashtag_and_identifier(compiler, "ifndef");
}

void preprocessor_skip_to_endif(struct compile_process *compiler) {
  while (!preprocessor_hashtag_and_identifier(compiler, "endif")) {
    if (preprocessor_is_hashtag_and_any_starting_if(compiler)) {
      preprocessor_skip_to_endif(compiler);
      continue;
    }

    preprocessor_next_token(compiler);
  }
}

void preprocessor_read_to_endif(struct compile_process *compiler,
                                bool true_clause) {
  while (preprocessor_next_token_no_increment(compiler) &&
         !preprocessor_hashtag_and_identifier(compiler, "endif")) {
    if (true_clause) {
      preprocessor_handle_token(compiler, preprocessor_next_token(compiler));
      continue;
    }

    // skip unexpanded tokens
    preprocessor_next_token(compiler);

    if (preprocessor_is_hashtag_and_any_starting_if(compiler)) {
      preprocessor_skip_to_endif(compiler);
    }
  }
}

int preprocessor_evaluate_number(struct preprocessor_node *node) {
  return node->const_val.llnum;
}

int preprocessor_evaluate_identifier(struct compile_process *compiler,
                                     struct preprocessor_node *node) {
  struct preprocessor *preprocessor = compiler->preprocessor;
  struct preprocessor_definition *def =
      preprocessor_get_definition(preprocessor, node->sval);
  if (!def) {
    return true;
  }

  if (vector_count(preprocessor_definition_value(def)) > 1) {
    struct vector *node_vec = vector_create(sizeof(struct preprocessor_node *));
    struct expressionable *expressionable = expressionable_create(
        &preprocessor_expressionable_config, preprocessor_definition_value(def),
        node_vec, EXPRESSIONABLE_FLAG_IS_PREPROCESSER_EXPRESSION);
    expressionable_parse(expressionable);
    struct preprocessor_node *node = expressionable_node_pop(expressionable);
    int val = preprocessor_evaluate(compiler, node);
    return val;
  }

  if (vector_count(preprocessor_definition_value(def)) == 0) {
    return false;
  }

  return preprocessor_definition_evaluated_value(def, NULL);
}

int preprocessor_arithmetic(struct compile_process *compiler, long left,
                            long right, const char *op) {
  bool success = false;
  long res = arithmetic(compiler, left, right, op, &success);
  if (!success) {
    compiler_error(compiler, "arithmetic error");
  }

  return res;
}

struct preprocessor_function_args *preprocessor_function_args_create() {
  struct preprocessor_function_args *args =
      malloc(sizeof(struct preprocessor_function_args));
  args->args = vector_create(sizeof(struct preprocessor_function_arg));
  return args;
}

bool preprocessor_exp_is_macro_function_call(struct preprocessor_node *node) {
  return node->type == PREPROCESSOR_EXPRESSION_NODE &&
         S_EQ(node->exp.op, "()") &&
         node->exp.left->type == PREPROCESSOR_IDENTIFIER_NODE;
}

void preprocessor_number_push_to_function_arguments(
    struct preprocessor_function_args *args, int64_t value) {
  struct token t;
  t.type = TOKEN_TYPE_NUMBER;
  t.llnum = value;
  preprocessor_token_push_to_function_arguments(args, &t);
}

void preprocessor_evaluate_function_call_arg(
    struct compile_process *compiler, struct preprocessor_node *node,
    struct preprocessor_function_args *args) {
  if (node->type == PREPROCESSOR_EXPRESSION_NODE && S_EQ(node->exp.op, ",")) {
    preprocessor_evaluate_function_call_arg(compiler, node->exp.left, args);
    preprocessor_evaluate_function_call_arg(compiler, node->exp.right, args);
    return;
  } else if (node->type == PREPROCESSOR_PARENTHESES_NODE) {
    preprocessor_evaluate_function_call_arg(compiler, node->paren_node.exp,
                                            args);
    return;
  }

  preprocessor_number_push_to_function_arguments(
      args, preprocessor_evaluate(compiler, node));
}

void preprocessor_evaluate_function_call_args(
    struct compile_process *compiler, struct preprocessor_node *node,
    struct preprocessor_function_args *args) {
  preprocessor_evaluate_function_call_arg(compiler, node, args);
}

bool preprocessor_is_macro_function(struct preprocessor_definition *def) {
  return def->type == PREPROCESSOR_DEFINITION_MACRO_FUNCTION ||
         def->type == PREPROCESSOR_DEFINITION_NATIVE_CALLBACK;
}

int preprocessor_function_args_count(struct preprocessor_function_args *args) {
  if (!args) {
    return 0;
  }

  return vector_count(args->args);
}

int preprocessor_macro_function_push_arg(
    struct compile_process *compiler, struct preprocessor_definition *def,
    struct preprocessor_function_args *args, const char *arg_name,
    struct vector *def_token_vec, struct vector *value_vec_target) {
  int arg_index = preprocessor_definition_argument_exists(def, arg_name);
  if (arg_index != -1) {
    preprocessor_function_argument_push_to_vec(
        preprocessor_function_argument_at(args, arg_index), value_vec_target);
  }

  return arg_index;
}

void preprocessor_token_vec_push_src_token_to_dst(
    struct compile_process *compiler, struct token *token,
    struct vector *dst_vec) {
  vector_push(dst_vec, token);
}

void preprocessor_handle_typedef_body_for_no_struct_or_union(
    struct compile_process *compiler, struct vector *token_vec,
    struct typedef_type *td, struct vector *src_vec,
    bool overflow_use_token_vec) {
  td->type = TYPEDEF_TYPE_STANDARD;
  struct token *token = preprocessor_peek_next_token_with_vector(
      compiler, src_vec, overflow_use_token_vec);
  while (token) {
    if (token_is_symbol(token, ';')) {
      break;
    }

    preprocessor_token_vec_push_src_resolve_definition(compiler, src_vec,
                                                       token_vec, token);
    token = preprocessor_peek_next_token_with_vector(compiler, src_vec,
                                                     overflow_use_token_vec);
  }
}

void preprocessor_handle_typedef_body_for_braces(
    struct compile_process *compiler, struct vector *token_vec,
    struct vector *src_vec, bool overflow_use_token_vec) {
  struct token *token = preprocessor_peek_next_token_with_vector(
      compiler, src_vec, overflow_use_token_vec);
  while (token) {
    if (token_is_symbol(token, '{')) {
      vector_push(token_vec, token);
      preprocessor_handle_typedef_body_for_braces(compiler, token_vec, src_vec,
                                                  overflow_use_token_vec);
      token = preprocessor_peek_next_token_with_vector(compiler, src_vec,
                                                       overflow_use_token_vec);
      continue;
    }

    vector_push(token_vec, token);
    if (token_is_symbol(token, '}')) {
      break;
    }

    token = preprocessor_peek_next_token_with_vector(compiler, src_vec,
                                                     overflow_use_token_vec);
  }
}

void preprocessor_handle_typedef_body_for_struct_or_union(
    struct compile_process *compiler, struct vector *token_vec,
    struct typedef_type *td, struct vector *src_vec,
    bool overflow_use_token_vec) {
  struct token *token = preprocessor_peek_next_token_with_vector(
      compiler, src_vec, overflow_use_token_vec);
  assert(token_is_keyword(token, "struct"));

  td->type = TYPEDEF_TYPE_STRUCT_TYPEDEF;
  // push struct keyword
  vector_push(token_vec, token);

  token = preprocessor_peek_next_token_with_vector(compiler, src_vec,
                                                   overflow_use_token_vec);

  // do we have a name?
  if (token->type == TOKEN_TYPE_IDENTIFIER) {
    td->_struct.name = token->sval;
    vector_push(token_vec, token);
    token = preprocessor_peek_next_token_with_vector(compiler, src_vec,
                                                     overflow_use_token_vec);
    if (token->type == TOKEN_TYPE_IDENTIFIER) {
      // just declaration
      vector_push(token_vec, token);
      return;
    }
  }

  // handle struct body
  while (token) {
    if (token_is_symbol(token, '{')) {
      td->type = TYPEDEF_TYPE_STRUCT_TYPEDEF;
      vector_push(token_vec, token);
      preprocessor_handle_typedef_body_for_braces(compiler, token_vec, src_vec,
                                                  overflow_use_token_vec);
      token = preprocessor_peek_next_token_with_vector(compiler, src_vec,
                                                       overflow_use_token_vec);
      continue;
    }

    if (token_is_symbol(token, ';')) {
      break;
    }

    preprocessor_token_vec_push_src_resolve_definition(compiler, src_vec,
                                                       token_vec, token);
    token = preprocessor_peek_next_token_with_vector(compiler, src_vec,
                                                     overflow_use_token_vec);
  }
}

void preprocessor_handle_typedef_body(struct compile_process *compiler,
                                      struct vector *token_vec,
                                      struct typedef_type *td,
                                      struct vector *src_vec,
                                      bool overflow_use_token_vec) {
  memset(td, 0, sizeof(struct typedef_type));

  struct token *token = preprocessor_peek_next_token_with_vector_no_increment(
      compiler, src_vec, overflow_use_token_vec);
  if (token_is_keyword(token, "struct") || token_is_keyword(token, "union")) {
    preprocessor_handle_typedef_body_for_struct_or_union(
        compiler, token_vec, td, src_vec, overflow_use_token_vec);
  } else {
    preprocessor_handle_typedef_body_for_no_struct_or_union(
        compiler, token_vec, td, src_vec, overflow_use_token_vec);
  }
}

void preprocessor_token_push_semicolon(struct compile_process *compiler) {
  struct token t1;
  t1.type = TOKEN_TYPE_SYMBOL;
  t1.cval = ';';
  vector_push(compiler->token_vec, &t1);
}

void preprocessor_handle_typedef_token(struct compile_process *compiler,
                                       struct vector *src_vec,
                                       bool overflow_use_token_vec) {
  // "typdef unsigned int x;"
  struct vector *token_vec = vector_create(sizeof(struct token));
  struct typedef_type td;
  preprocessor_handle_typedef_body(compiler, token_vec, &td, src_vec,
                                   overflow_use_token_vec);
  struct token *name_token = vector_back_or_null(token_vec);
  if (!name_token) {
    compiler_error(compiler, "expected name");
  }

  if (name_token->type != TOKEN_TYPE_IDENTIFIER) {
    compiler_error(compiler, "expected identifier");
  }

  td.def_name = name_token->sval;

  // pop off name token
  vector_pop(token_vec);

  if (td.type == TYPEDEF_TYPE_STRUCT_TYPEDEF) {
    preprocessor_token_vec_push_src(compiler, token_vec);
    preprocessor_token_push_semicolon(compiler);

    token_vec = vector_create(sizeof(struct token));
    preprocessor_token_vec_push_keyword_and_identifier(token_vec, "struct",
                                                       td._struct.name);
  }

  struct preprocessor *preprocessor = compiler->preprocessor;
  preprocessor_definition_create_typedef(td.def_name, token_vec, preprocessor);
}

void preprocessor_token_vec_push_src_resolve_definition(
    struct compile_process *compiler, struct vector *src_vec,
    struct vector *dst_vec, struct token *token) {
  if (preprocessor_token_is_typedef(token)) {
    preprocessor_handle_typedef_token(compiler, src_vec, true);
    return;
  }

  if (token->type == TOKEN_TYPE_IDENTIFIER) {
    preprocessor_handle_identifier_for_token_vector(compiler, src_vec, dst_vec,
                                                    token);
    return;
  }

  preprocessor_token_vec_push_src_token_to_dst(compiler, token, dst_vec);
}

void preprocessor_token_vec_push_src_resolve_definitions(
    struct compile_process *compiler, struct vector *src_vec,
    struct vector *dst_vec) {
  assert(src_vec != compiler->token_vec);
  vector_set_peek_pointer(src_vec, 0);
  struct token *token = vector_peek(src_vec);
  while (token) {
    preprocessor_token_vec_push_src_resolve_definition(compiler, src_vec,
                                                       dst_vec, token);
    token = vector_peek(src_vec);
  }
}

int preprocessor_macro_function_push_something_definition(
    struct compile_process *compiler, struct preprocessor_definition *def,
    struct preprocessor_function_args *args, struct token *arg_token,
    struct vector *def_token_vec, struct vector *value_vec_target) {
  if (arg_token->type != TOKEN_TYPE_IDENTIFIER) {
    return -1;
  }

  const char *arg_name = arg_token->sval;
  int res = preprocessor_macro_function_push_arg(
      compiler, def, args, arg_name, def_token_vec, value_vec_target);
  if (res != -1) {
    return 0;
  }

  // there is no argument with this name
  struct preprocessor_definition *arg_def =
      preprocessor_get_definition(compiler->preprocessor, arg_name);
  if (arg_def) {
    preprocessor_token_vec_push_src_resolve_definitions(
        compiler, preprocessor_definition_value(arg_def), compiler->token_vec);
    return 0;
  }

  return -1;
}

bool preprocessor_is_next_double_hash(struct vector *def_token_vec) {
  bool is_double_hash = true;
  vector_save(def_token_vec);
  struct token *next_token = vector_peek(def_token_vec);
  if (!token_is_symbol(next_token, '#')) {
    is_double_hash = false;
    goto out;
  }

  next_token = vector_peek(def_token_vec);
  if (!token_is_symbol(next_token, '#')) {
    is_double_hash = false;
    goto out;
  }

out:
  vector_restore(def_token_vec);
  return is_double_hash;
}

void preprocessor_handle_concat_part(struct compile_process *compiler,
                                     struct preprocessor_definition *def,
                                     struct preprocessor_function_args *args,
                                     struct token *token,
                                     struct vector *def_token_vec,
                                     struct vector *value_vec_target) {
  preprocessor_macro_function_push_something(compiler, def, args, token,
                                             def_token_vec, value_vec_target);
}

void preprocessor_handle_concat_finalize(struct compile_process *compiler,
                                         struct vector *tmp_vec,
                                         struct vector *value_vec_target) {
  struct vector *joined_vec = tokens_join_vector(compiler, tmp_vec);
  vector_insert(value_vec_target, joined_vec, 0);
}

void preprocessor_handle_concat(struct compile_process *compiler,
                                struct preprocessor_definition *def,
                                struct preprocessor_function_args *args,
                                struct token *arg_token,
                                struct vector *def_token_vec,
                                struct vector *value_vec_target) {
  // skip ##
  vector_peek(def_token_vec);
  vector_peek(def_token_vec);

  struct token *right_token = vector_peek(def_token_vec);
  if (!right_token) {
    compiler_error(compiler, "Expected an operand");
  }

  struct vector *tmp_vec = vector_create(sizeof(struct token));
  preprocessor_handle_concat_part(compiler, def, args, arg_token, def_token_vec,
                                  tmp_vec);
  preprocessor_handle_concat_part(compiler, def, args, right_token,
                                  def_token_vec, tmp_vec);
  preprocessor_handle_concat_finalize(compiler, tmp_vec, value_vec_target);
}

void preprocessor_macro_function_push_something(
    struct compile_process *compiler, struct preprocessor_definition *def,
    struct preprocessor_function_args *args, struct token *arg_token,
    struct vector *def_token_vec, struct vector *value_vec_target) {
  if (preprocessor_is_next_double_hash(def_token_vec)) {
    preprocessor_handle_concat(compiler, def, args, arg_token, def_token_vec,
                               value_vec_target);
    return;
  }

  int res = preprocessor_macro_function_push_something_definition(
      compiler, def, args, arg_token, def_token_vec, value_vec_target);
  if (res == -1) {
    vector_push(value_vec_target, arg_token);
  }
}

void preprocessor_handle_function_arg_to_string(
    struct compile_process *compiler, struct vector *src_vec,
    struct vector *value_vec_target, struct preprocessor_definition *def,
    struct preprocessor_function_args *args) {
  struct token *next_token = vector_peek(src_vec);
  if (!next_token || next_token->type != TOKEN_TYPE_IDENTIFIER) {
    compiler_error(compiler, "expected identifier");
  }

  int arg_index =
      preprocessor_definition_argument_exists(def, next_token->sval);
  if (arg_index < 0) {
    compiler_error(compiler, "argument not found");
  }

  struct preprocessor_function_arg *arg =
      preprocessor_function_argument_at(args, arg_index);
  if (!arg) {
    compiler_error(compiler, "argument not found");
  }

  struct token *first_token_for_argument = vector_peek_at(arg->tokens, 0);

  // create string token
  struct token str_token;
  str_token.type = TOKEN_TYPE_STRING;
  str_token.sval = first_token_for_argument->between_brackets;
  vector_push(value_vec_target, &str_token);
}

int preprocessor_macro_function_execute(struct compile_process *compiler,
                                        const char *func_name,
                                        struct preprocessor_function_args *args,
                                        int flags) {
  struct preprocessor *preprocessor = compiler->preprocessor;
  struct preprocessor_definition *def =
      preprocessor_get_definition(preprocessor, func_name);
  if (!def) {
    compiler_error(compiler, "macro function not found");
  }

  if (!preprocessor_is_macro_function(def)) {
    compiler_error(compiler, "not a macro function");
  }

  if (vector_count(def->standard.args) !=
          preprocessor_function_args_count(args) &&
      def->type != PREPROCESSOR_DEFINITION_NATIVE_CALLBACK) {
    compiler_error(compiler, "macro function argument count mismatch to %s",
                   func_name);
  }

  struct vector *value_vec_target = vector_create(sizeof(struct token));
  struct vector *def_token_vec =
      preprocessor_definition_value_with_arguments(def, args);
  vector_set_peek_pointer(def_token_vec, 0);
  struct token *token = vector_peek(def_token_vec);
  while (token) {
    if (token_is_symbol(token, '#')) {
      preprocessor_handle_function_arg_to_string(compiler, def_token_vec,
                                                 value_vec_target, def, args);
      token = vector_peek(def_token_vec);
      continue;
    }

    preprocessor_macro_function_push_something(compiler, def, args, token,
                                               def_token_vec, value_vec_target);
    token = vector_peek(def_token_vec);
  }

  if (flags & PREPROCESSOR_FLAG_EVALUATE_NODE) {
    return preprocessor_parse_evaluate(compiler, value_vec_target);
  }

  preprocessor_token_vec_push_src(compiler, value_vec_target);
  return 0;
}

int preprocessor_evaluate_function_call(struct compile_process *compiler,
                                        struct preprocessor_node *node) {
  const char *macro_func_name = node->exp.left->sval;
  struct preprocessor_node *call_args = node->exp.right->paren_node.exp;
  struct preprocessor_function_args *args = preprocessor_function_args_create();

  // evaluate preprocessor arguments
  preprocessor_evaluate_function_call_args(compiler, call_args, args);

  return preprocessor_macro_function_execute(compiler, macro_func_name, args,
                                             PREPROCESSOR_FLAG_EVALUATE_NODE);
}

int preprocessor_evaluate_exp(struct compile_process *compiler,
                              struct preprocessor_node *node) {
  if (preprocessor_exp_is_macro_function_call(node)) {
    return preprocessor_evaluate_function_call(compiler, node);
  }

  long left_operand = preprocessor_evaluate(compiler, node->exp.left);
  if (node->exp.right->type == PREPROCESSOR_TENARY_NODE) {
    if (left_operand) {
      return preprocessor_evaluate(compiler,
                                   node->exp.right->tenary_node.true_node);
    } else {
      return preprocessor_evaluate(compiler,
                                   node->exp.right->tenary_node.false_node);
    }
  }

  long right_operand = preprocessor_evaluate(compiler, node->exp.right);
  return preprocessor_arithmetic(compiler, left_operand, right_operand,
                                 node->exp.op);
}

int preprocessor_evaluate_unary(struct compile_process *compiler,
                                struct preprocessor_node *node) {
  int res = 0;
  const char *op = node->unary_node.op;
  struct preprocessor_node *right_operand = node->unary_node.operand;
  if (S_EQ(op, "!")) {
    res = !preprocessor_evaluate(compiler, right_operand);
  } else if (S_EQ(op, "~")) {
    res = ~preprocessor_evaluate(compiler, right_operand);
  } else if (S_EQ(op, "-")) {
    res = -preprocessor_evaluate(compiler, right_operand);
  } else {
    compiler_error(compiler, "unknown unary operator");
  }

  return res;
}

int preprocessor_evaluate_parentheses(struct compile_process *compiler,
                                      struct preprocessor_node *node) {
  return preprocessor_evaluate(compiler, node->paren_node.exp);
}

const char *preprocessor_pull_string_from(struct preprocessor_node *node) {
  const char *res = NULL;
  switch (node->type) {
  case PREPROCESSOR_PARENTHESES_NODE:
    res = preprocessor_pull_string_from(node->paren_node.exp);
    break;

  case PREPROCESSOR_KEYWORD_NODE:
  case PREPROCESSOR_IDENTIFIER_NODE:
    res = node->sval;
    break;

  case PREPROCESSOR_EXPRESSION_NODE:
    res = preprocessor_pull_string_from(node->exp.left);
    break;
  }

  return res;
}

const char *preprocessor_pull_defined_value(struct compile_process *compiler,
                                            struct preprocessor_node *node) {
  const char *val = preprocessor_pull_string_from(node->joined_node.right);
  if (!val) {
    compiler_error(compiler, "expected identifier");
  }

  return val;
}

int preprocessor_evaluate_joined_node_defined(struct compile_process *compiler,
                                              struct preprocessor_node *node) {
  const char *right_val = preprocessor_pull_defined_value(compiler, node);
  return preprocessor_get_definition(compiler->preprocessor, right_val) != NULL;
}

int preprocessor_evaluate_joined_node(struct compile_process *compiler,
                                      struct preprocessor_node *node) {
  if (node->joined_node.left->type != PREPROCESSOR_KEYWORD_NODE) {
    return 0;
  }

  int res = 0;
  if (S_EQ(node->joined_node.left->sval, "defined")) {
    res = preprocessor_evaluate_joined_node_defined(compiler, node);
  }

  return res;
}

int preprocessor_evaluate(struct compile_process *compiler,
                          struct preprocessor_node *root_node) {
  struct preprocessor_node *current = root_node;
  int res = 0;
  switch (current->type) {
  case PREPROCESSOR_NUMBER_NODE:
    res = preprocessor_evaluate_number(current);
    break;

  case PREPROCESSOR_IDENTIFIER_NODE:
    res = preprocessor_evaluate_identifier(compiler, current);
    break;

  case PREPROCESSOR_EXPRESSION_NODE:
    res = preprocessor_evaluate_exp(compiler, current);
    break;

  case PREPROCESSOR_UNARY_NODE:
    res = preprocessor_evaluate_unary(compiler, current);
    break;

  case PREPROCESSOR_PARENTHESES_NODE:
    res = preprocessor_evaluate_parentheses(compiler, current);
    break;

  case PREPROCESSOR_JOINED_NODE:
    res = preprocessor_evaluate_joined_node(compiler, current);
    break;
  }

  return res;
}

int preprocessor_parse_evaluate(struct compile_process *compiler,
                                struct vector *token_vec) {
  struct vector *node_vec = vector_create(sizeof(struct preprocessor_node *));
  struct expressionable *expressionable = expressionable_create(
      &preprocessor_expressionable_config, token_vec, node_vec, 0);
  expressionable_parse(expressionable);

  struct preprocessor_node *root_node = expressionable_node_pop(expressionable);
  return preprocessor_evaluate(compiler, root_node);
}

void preprocessor_handle_if_token(struct compile_process *compiler) {
  int res = preprocessor_parse_evaluate(compiler, compiler->token_vec_original);
  preprocessor_read_to_endif(compiler, res > 0);
}

void preprocessor_handle_ifdef_token(struct compile_process *compiler) {
  struct token *cond_token = preprocessor_next_token(compiler);
  if (!cond_token) {
    compiler_error(compiler, "expected identifier\n");
  }

  struct preprocessor_definition *def =
      preprocessor_get_definition(compiler->preprocessor, cond_token->sval);
  // read body of ifdef
  preprocessor_read_to_endif(compiler, def != NULL);
}

void preprocessor_handle_ifndef_token(struct compile_process *compiler) {
  struct token *cond_token = preprocessor_next_token(compiler);
  if (!cond_token) {
    compiler_error(compiler, "expected identifier\n");
  }

  struct preprocessor_definition *def =
      preprocessor_get_definition(compiler->preprocessor, cond_token->sval);
  preprocessor_read_to_endif(compiler, def == NULL);
}

struct token *
preprocessor_next_token_skip_nl(struct compile_process *compiler) {
  struct token *token = preprocessor_next_token(compiler);
  while (token && token->type == TOKEN_TYPE_NEWLINE) {
    token = preprocessor_next_token(compiler);
  }

  return token;
}

void preprocessor_handle_include_token(struct compile_process *compiler) {
  struct token *file_path_token = preprocessor_next_token_skip_nl(compiler);
  if (!file_path_token) {
    compiler_error(compiler, "expected file path");
  }

  struct compile_process *new_compile_process =
      compile_include(file_path_token->sval, compiler);
  if (!new_compile_process) {
    PREPROCESSOR_STATIC_INCLUDE_HANDLER_POST_CREATION handler =
        preprocessor_static_include_handler_for(file_path_token->sval);
    if (handler) {
      // handle static include
      preprocessor_create_static_include(compiler->preprocessor,
                                         file_path_token->sval, handler);
      return;
    }

    compiler_error(compiler, "failed to include file");
  }

  preprocessor_token_vec_push_src(compiler, new_compile_process->token_vec);
}

int preprocessor_handle_hashtag_token(struct compile_process *compiler,
                                      struct token *token) {
  bool is_preprocessed = false;
  struct token *next_token = preprocessor_next_token(compiler);
  if (preprocessor_token_is_define(next_token)) {
    preprocessor_handle_definition_token(compiler);
    is_preprocessed = true;
  } else if (preprocessor_token_is_undef(next_token)) {
    preprocessor_handle_undef_token(compiler);
    is_preprocessed = true;
  } else if (preprocessor_token_is_warning(next_token)) {
    preprocessor_handle_warning_token(compiler);
    is_preprocessed = true;
  } else if (preprocessor_token_is_error(next_token)) {
    preprocessor_handle_error_token(compiler);
    is_preprocessed = true;
  } else if (preprocessor_token_is_ifdef(next_token)) {
    preprocessor_handle_ifdef_token(compiler);
    is_preprocessed = true;
  } else if (preprocessor_token_is_ifndef(next_token)) {
    preprocessor_handle_ifndef_token(compiler);
    is_preprocessed = true;
  } else if (preprocessor_token_is_if(next_token)) {
    preprocessor_handle_if_token(compiler);
    is_preprocessed = true;
  } else if (preprocessor_token_is_include(next_token)) {
    preprocessor_handle_include_token(compiler);
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

struct token *preprocessor_handle_identifier_macro_call_arg_parse_paren(
    struct compile_process *compiler, struct vector *src_vec,
    struct vector *value_vec, struct preprocessor_function_args *args,
    struct token *left_paren_token) {
  // push left paren
  vector_push(value_vec, left_paren_token);

  struct token *next_token = vector_peek(src_vec);
  while (next_token && !token_is_symbol(next_token, ')')) {
    if (token_is_operator(next_token, "(")) {
      next_token = preprocessor_handle_identifier_macro_call_arg_parse_paren(
          compiler, src_vec, value_vec, args, next_token);
    }

    vector_push(value_vec, next_token);
    next_token = vector_peek(src_vec);
  }

  if (!next_token) {
    compiler_error(compiler, "expected )");
  }

  vector_push(value_vec, next_token);

  return vector_peek(src_vec);
}

void preprocessor_function_argument_push(
    struct preprocessor_function_args *args, struct vector *value_vec) {
  struct preprocessor_function_arg arg;
  arg.tokens = vector_clone(value_vec);
  vector_push(args->args, &arg);
}

void preprocessor_handle_identifier_macro_call_arg(
    struct preprocessor_function_args *args, struct vector *token_vec) {
  preprocessor_function_argument_push(args, token_vec);
}

struct token *preprocessor_handle_identifier_macro_call_arg_parse(
    struct compile_process *compiler, struct vector *src_vec,
    struct vector *value_vec, struct preprocessor_function_args *args,
    struct token *token) {
  if (token_is_operator(token, "(")) {
    return preprocessor_handle_identifier_macro_call_arg_parse_paren(
        compiler, src_vec, value_vec, args, token);
  }

  if (token_is_symbol(token, ')')) {
    // end of arguments
    preprocessor_handle_identifier_macro_call_arg(args, value_vec);
    return NULL;
  }

  if (token_is_operator(token, ",")) {
    // next argument
    preprocessor_handle_identifier_macro_call_arg(args, value_vec);
    vector_clear(value_vec);
    return vector_peek(src_vec);
  }

  vector_push(value_vec, token);
  token = vector_peek(src_vec);
  return token;
}

struct preprocessor_function_args *
preprocessor_handle_identifier_macro_call_args(struct compile_process *compiler,
                                               struct vector *src_vec) {
  // skip (
  vector_peek(src_vec);

  struct preprocessor_function_args *args = preprocessor_function_args_create();
  struct token *token = vector_peek(src_vec);
  struct vector *value_vec = vector_create(sizeof(struct token));
  while (token) {
    token = preprocessor_handle_identifier_macro_call_arg_parse(
        compiler, src_vec, value_vec, args, token);
  }

  vector_free(value_vec);

  return args;
}

int preprocessor_handle_identifier_for_token_vector(
    struct compile_process *compiler, struct vector *src_vec,
    struct vector *dst_vec, struct token *token) {
  struct preprocessor_definition *def =
      preprocessor_get_definition(compiler->preprocessor, token->sval);
  if (!def) {
    // not a macro
    preprocessor_token_push_to_dst(dst_vec, token);
    return -1;
  }

  if (def->type == PREPROCESSOR_DEFINITION_TYPEDEF) {
    preprocessor_token_vec_push_src_to_dst(
        compiler, preprocessor_definition_value(def), dst_vec);
    return 0;
  }

  if (token_is_operator(vector_peek_no_increment(src_vec), "(")) {
    struct preprocessor_function_args *args =
        preprocessor_handle_identifier_macro_call_args(compiler, src_vec);
    const char *func_name = token->sval;
    preprocessor_macro_function_execute(compiler, func_name, args, 0);
    return 0;
  }

  struct vector *def_val = preprocessor_definition_value(def);
  preprocessor_token_vec_push_src_resolve_definitions(
      compiler, preprocessor_definition_value(def), dst_vec);
  return 0;
}

int preprocessor_handle_identifier(struct compile_process *compiler,
                                   struct token *token) {
  return preprocessor_handle_identifier_for_token_vector(
      compiler, compiler->token_vec_original, compiler->token_vec, token);
}

void preprocessor_handle_keyword(struct compile_process *compiler,
                                 struct token *token) {
  if (preprocessor_token_is_typedef(token)) {
    preprocessor_handle_typedef_token(compiler, compiler->token_vec_original,
                                      false);
    return;
  }

  preprocessor_token_push_dst(compiler, token);
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

  case TOKEN_TYPE_IDENTIFIER:
    preprocessor_handle_identifier(compiler, token);
    break;

  case TOKEN_TYPE_KEYWORD:
    preprocessor_handle_keyword(compiler, token);
    break;

  default:
    preprocessor_token_push_dst(compiler, token);
    break;
  }
}

int preprocessor_run(struct compile_process *compiler) {
  preprocessor_add_included_file(compiler->preprocessor,
                                 compiler->cfile.abs_path);
  vector_set_peek_pointer(compiler->token_vec_original, 0);
  struct token *token = preprocessor_next_token(compiler);
  while (token) {
    preprocessor_handle_token(compiler, token);
    token = preprocessor_next_token(compiler);
  }

  return PREPROCESS_ALL_OK;
}
