#include "compiler.h"
#include "helpers/vector.h"

static struct compile_process *validator_current_compile_process;
static struct node *current_function;

void validate_variable(struct node *var_node);
void validate_body(struct body *body);

void validation_new_scope(int flags) {
  resolver_default_new_scope(validator_current_compile_process->resolver,
                             flags);
}

void validation_finish_scope() {
  resolver_default_finish_scope(validator_current_compile_process->resolver);
}

struct node *validation_next_tree_node() {
  return vector_peek_ptr(validator_current_compile_process->node_tree_vec);
}

void validate_init(struct compile_process *process) {
  validator_current_compile_process = process;
  vector_set_peek_pointer(process->node_tree_vec, 0);
  symresolver_new_table(process);
}

void validate_destroy(struct compile_process *process) {
  symresolver_end_table(process);
  vector_set_peek_pointer(process->node_tree_vec, 0);
}

void validate_symbol_unique(const char *name, const char *type,
                            struct node *node) {
  struct symbol *sym =
      symresolver_get_symbol(validator_current_compile_process, name);
  if (sym) {
    compiler_node_error(node, "symbol %s already defined", name);
  }
}

void validate_identifier(struct node *node) {
  struct resolver_result *result =
      resolver_follow(validator_current_compile_process->resolver, node);
  if (!resolver_result_ok(result)) {
    compiler_error(validator_current_compile_process, "identifier %s not found",
                   node->sval);
  }
}

void validate_expressionable(struct node *node) {
  switch (node->type) {
  case NODE_TYPE_IDENTIFIER:
    validate_identifier(node);
    break;
  }
}

void validate_return_node(struct node *node) {
  if (node->stmt.return_stmt.exp) {
    if (datatype_is_void_no_ptr(&current_function->func.rtype)) {
      compiler_node_error(node, "returning value from void function");
    }

    validate_expressionable(node->stmt.return_stmt.exp);
  }
}

void validate_if_stmt(struct node *node) {
  validation_new_scope(0);
  validate_expressionable(node->stmt.if_stmt.cond_node);
  validate_body(&node->stmt.if_stmt.body_node->body);
  validation_finish_scope();
}

void validate_for_stmt(struct node *node) {
  validation_new_scope(0);
  validate_expressionable(node->stmt.for_stmt.init);
  validate_expressionable(node->stmt.for_stmt.cond);
  validate_expressionable(node->stmt.for_stmt.inc);
  validate_body(&node->stmt.for_stmt.body->body);
  validation_finish_scope();
}

void validate_while_stmt(struct node *node) {
  validation_new_scope(0);
  validate_expressionable(node->stmt.while_stmt.cond);
  validate_body(&node->stmt.while_stmt.body->body);
  validation_finish_scope();
}

void validate_do_while_stmt(struct node *node) {
  validation_new_scope(0);
  validate_expressionable(node->stmt.do_while_stmt.cond);
  validate_body(&node->stmt.do_while_stmt.body->body);
  validation_finish_scope();
}

void validate_switch_stmt(struct node *node) {
  validation_new_scope(0);
  validate_expressionable(node->stmt.switch_stmt.exp);
  validate_body(&node->stmt.switch_stmt.body->body);
  validation_finish_scope();
}

void validate_statement(struct node *node) {
  switch (node->type) {
  case NODE_TYPE_VARIABLE:
    validate_variable(node);
    break;

  case NODE_TYPE_STATEMENT_RETURN:
    validate_return_node(node);
    break;

  case NODE_TYPE_STATEMENT_IF:
    validate_if_stmt(node);
    break;

  case NODE_TYPE_STATEMENT_FOR:
    validate_for_stmt(node);
    break;

  case NODE_TYPE_STATEMENT_WHILE:
    validate_while_stmt(node);
    break;

  case NODE_TYPE_STATEMENT_DO_WHILE:
    validate_do_while_stmt(node);
    break;

  case NODE_TYPE_STATEMENT_SWITCH:
    validate_switch_stmt(node);
    break;
  }
}

void validate_body(struct body *body) {
  vector_set_peek_pointer(body->statements, 0);
  struct node *statement = vector_peek_ptr(body->statements);
  while (statement) {
    validate_statement(statement);
    statement = vector_peek_ptr(body->statements);
  }
}

void validate_function_body(struct node *node) { validate_body(&node->body); }

void validate_variable(struct node *var_node) {
  struct resolver_entity *entity = resolver_get_variable_from_local_scope(
      validator_current_compile_process->resolver, var_node->var.name);
  if (entity) {
    compiler_node_error(var_node, "variable %s already defined",
                        var_node->var.name);
  }

  resolver_default_new_scope_entity(validator_current_compile_process->resolver,
                                    var_node, 0, 0);
}

void validate_function_arg(struct node *func_arg_var_node) {
  validate_variable(func_arg_var_node);
}

void validate_function_args(struct function_args *func_args) {
  struct vector *func_arg_vec = func_args->args;
  vector_set_peek_pointer(func_arg_vec, 0);
  struct node *current = vector_peek_ptr(func_arg_vec);
  while (current) {
    validate_function_arg(current);
    current = vector_peek_ptr(func_arg_vec);
  }
}

void validate_function_node(struct node *node) {
  current_function = node;
  if (!(node->flags & NODE_FLAG_IS_FORWARD_DECLARATION)) {
    validate_symbol_unique(node->func.name, "function", node);
  }

  symresolver_register_symbol(validator_current_compile_process,
                              node->func.name, SYMBOL_TYPE_NODE, node);
  validation_new_scope(0);
  // validate function args
  validate_function_args(&node->func.args);

  // validate function body
  if (node->func.body_n) {
    validate_function_body(node->func.body_n);
  }

  validation_finish_scope();
}

void validate_struct_node(struct node *node) {
  if (!(node->flags & NODE_FLAG_IS_FORWARD_DECLARATION)) {
    validate_symbol_unique(node->_struct.name, "struct", node);
  }

  symresolver_register_symbol(validator_current_compile_process,
                              node->_struct.name, SYMBOL_TYPE_NODE, node);
}

void validate_union_node(struct node *node) {
  if (!(node->flags & NODE_FLAG_IS_FORWARD_DECLARATION)) {
    validate_symbol_unique(node->_union.name, "union", node);
  }

  symresolver_register_symbol(validator_current_compile_process,
                              node->_union.name, SYMBOL_TYPE_NODE, node);
}

void validate_node(struct node *node) {
  switch (node->type) {
  case NODE_TYPE_FUNCTION:
    validate_function_node(node);
    break;

  case NODE_TYPE_STRUCT:
    validate_struct_node(node);
    break;

  case NODE_TYPE_UNION:
    validate_union_node(node);
    break;
  }
}

int validate_tree() {
  validation_new_scope(0);
  struct node *node = validation_next_tree_node();
  while (node) {
    validate_node(node);
    node = validation_next_tree_node();
  }
  validation_finish_scope();
  return VALIDATION_ALL_OK;
}

int validate(struct compile_process *process) {
  int res = 0;
  validate_init(process);
  res = validate_tree();
  validate_destroy(process);
  return res;
}
