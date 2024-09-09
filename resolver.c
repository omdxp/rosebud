#include "compiler.h"
#include "helpers/vector.h"
#include <assert.h>
#include <stdlib.h>

void resolver_follow_part(struct resolver_process *process, struct node *node,
                          struct resolver_result *result);
struct resolver_entity *
resolver_follow_expression(struct resolver_process *process, struct node *node,
                           struct resolver_result *result);
struct resolver_result *resolver_follow(struct resolver_process *process,
                                        struct node *node);
struct resolver_entity *
resolver_follow_array_bracket(struct resolver_process *process,
                              struct node *node,
                              struct resolver_result *result);
struct resolver_entity *
resolver_follow_part_return_entity(struct resolver_process *process,
                                   struct node *node,
                                   struct resolver_result *result);

bool resolver_result_failed(struct resolver_result *result) {
  return result->flags & RESOLVER_RESULT_FLAG_FAILED;
}

bool resolver_result_ok(struct resolver_result *result) {
  return !resolver_result_failed(result);
}

bool resolver_result_finished(struct resolver_result *result) {
  return result->flags & RESOLVER_RESULT_FLAG_RUNTIME_NEEDED_TO_FINISH_PATH;
}

struct resolver_entity *
resolver_result_entity_root(struct resolver_result *result) {
  return result->entity;
}

struct resolver_entity *resolver_entity_next(struct resolver_entity *entity) {
  return entity->next;
}

struct resolver_entity *resolver_entity_clone(struct resolver_entity *entity) {
  if (!entity) {
    return NULL;
  }

  struct resolver_entity *clone = calloc(1, sizeof(struct resolver_entity));
  memcpy(clone, entity, sizeof(struct resolver_entity));
  return clone;
}

struct resolver_entity *resolver_result_entity(struct resolver_result *result) {
  if (resolver_result_failed(result)) {
    return NULL;
  }

  return result->entity;
}

struct resolver_result *resolver_new_result(struct resolver_process *process) {
  struct resolver_result *result = calloc(1, sizeof(struct resolver_result));
  result->array_data.entities = vector_create(sizeof(struct resolver_entity *));
  return result;
}

void resolver_result_free(struct resolver_result *result) {
  if (!result) {
    return;
  }

  vector_free(result->array_data.entities);
  free(result);
}

struct resolver_scope *
resolver_process_scope_current(struct resolver_process *process) {
  return process->scopes.current;
}

void resolver_runtime_needed(struct resolver_result *result,
                             struct resolver_entity *last_entity) {
  result->entity = last_entity;
  result->flags &= ~RESOLVER_RESULT_FLAG_RUNTIME_NEEDED_TO_FINISH_PATH;
}

void resolver_result_entity_push(struct resolver_result *result,
                                 struct resolver_entity *entity) {
  if (!result->first_entity_const) {
    result->first_entity_const = entity;
  }

  if (!result->last_entity) {
    result->entity = entity;
    result->last_entity = entity;
    result->count++;
    return;
  }

  result->last_entity->next = entity;
  entity->prev = result->last_entity;
  result->last_entity = entity;
  result->count++;
}

struct resolver_entity *resolver_result_peek(struct resolver_result *result) {
  return result->last_entity;
}

struct resolver_entity *
resolver_result_peek_ignore_rule_entity(struct resolver_result *result) {
  struct resolver_entity *entity = resolver_result_peek(result);
  while (entity && entity->type == RESOLVER_ENTITY_TYPE_RULE) {
    entity = entity->prev;
  }
  return entity;
}

struct resolver_entity *resolver_result_pop(struct resolver_result *result) {
  if (!result->entity) {
    return NULL;
  }

  struct resolver_entity *entity = result->last_entity;
  if (result->entity == result->last_entity) {
    result->entity = result->last_entity->prev;
    result->last_entity = result->last_entity->prev;
    result->count--;
    goto out;
  }

  result->last_entity = result->last_entity->prev;
  result->count--;

out:
  if (result->count == 0) {
    result->entity = NULL;
    result->first_entity_const = NULL;
    result->last_entity = NULL;
  }

  entity->prev = NULL;
  entity->next = NULL;
  return entity;
}

struct vector *resolver_array_data_vec(struct resolver_result *result) {
  return result->array_data.entities;
}

struct compile_process *
resolver_process_compile(struct resolver_process *process) {
  return process->compile_process;
}

struct resolver_scope *
resolver_scope_current(struct resolver_process *process) {
  return process->scopes.current;
}

struct resolver_scope *resolver_scope_root(struct resolver_process *process) {
  return process->scopes.root;
}

struct resolver_scope *resolver_new_scope_create() {
  struct resolver_scope *scope = calloc(1, sizeof(struct resolver_scope));
  scope->entities = vector_create(sizeof(struct resolver_entity *));
  return scope;
}

struct resolver_scope *resolver_new_scope(struct resolver_process *process,
                                          void *private, int flags) {
  struct resolver_scope *scope = resolver_new_scope_create();
  if (!scope) {
    return NULL;
  }

  process->scopes.current->next = scope;
  scope->prev = process->scopes.current;
  process->scopes.current = scope;
  scope->private = private;
  scope->flags = flags;
  return scope;
}

void resolver_finish_scope(struct resolver_process *process) {
  struct resolver_scope *scope = process->scopes.current;
  process->scopes.current = scope->prev;
  process->callbacks.delete_scope(scope);
  free(scope);
}

struct resolver_process *
resolver_new_process(struct compile_process *compile_process,
                     struct resolver_callbacks *callbacks) {
  struct resolver_process *process = calloc(1, sizeof(struct resolver_process));
  process->compile_process = compile_process;
  memcpy(&process->callbacks, callbacks, sizeof(process->callbacks));
  process->scopes.root = resolver_new_scope_create();
  process->scopes.current = process->scopes.root;
  return process;
}

struct resolver_entity *
resolver_create_new_entity(struct resolver_result *result, int type,
                           void *private) {
  struct resolver_entity *entity = calloc(1, sizeof(struct resolver_entity));
  if (!entity) {
    return NULL;
  }

  entity->type = type;
  entity->private = private;
  return entity;
}

struct resolver_entity *
resolver_create_new_entity_for_unsupported_node(struct resolver_result *result,
                                                struct node *node) {
  struct resolver_entity *entity = resolver_create_new_entity(
      result, RESOLVER_ENTITY_TYPE_UNSUPPORTED, NULL);
  if (!entity) {
    return NULL;
  }

  entity->node = node;
  entity->flags = RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_LEFT_ENTITY |
                  RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_NEXT_ENTITY;
  return entity;
}

struct resolver_entity *resolver_create_new_entity_for_array_bracket(
    struct resolver_result *result, struct resolver_process *process,
    struct node *node, struct node *array_index_node, int index,
    struct datatype *dtype, void *private, struct resolver_scope *scope) {
  struct resolver_entity *entity = resolver_create_new_entity(
      result, RESOLVER_ENTITY_TYPE_ARRAY_BRACKET, private);
  if (!entity) {
    return NULL;
  }

  entity->scope = scope;
  assert(entity->scope);
  entity->name = NULL;
  entity->dtype = *dtype;
  entity->node = node;
  entity->array.index = index;
  entity->array.dtype = *dtype;
  entity->array.array_index_node = array_index_node;
  int array_index_val = 1;
  if (array_index_node->type == NODE_TYPE_NUMBER) {
    array_index_val = array_index_node->llnum;
  }

  entity->offset_from_bp = array_offset(dtype, index, array_index_val);
  return entity;
}

struct resolver_entity *resolver_create_new_entity_for_merged_array_bracket(
    struct resolver_result *result, struct resolver_process *process,
    struct node *node, struct node *array_index_node, int index,
    struct datatype *dtype, void *private, struct resolver_scope *scope) {
  struct resolver_entity *entity = resolver_create_new_entity(
      result, RESOLVER_ENTITY_TYPE_ARRAY_BRACKET, private);
  if (!entity) {
    return NULL;
  }

  entity->scope = scope;
  assert(entity->scope);
  entity->name = NULL;
  entity->dtype = *dtype;
  entity->node = node;
  entity->array.index = index;
  entity->array.dtype = *dtype;
  entity->array.array_index_node = array_index_node;
  return entity;
}

struct resolver_entity *
resolver_create_new_unknown_entity(struct resolver_process *process,
                                   struct resolver_result *result,
                                   struct datatype *dtype, struct node *node,
                                   struct resolver_scope *scope, int offset) {
  struct resolver_entity *entity =
      resolver_create_new_entity(result, RESOLVER_ENTITY_TYPE_GENERAL, NULL);
  if (!entity) {
    return NULL;
  }

  entity->scope = scope;
  entity->flags |= RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_NEXT_ENTITY |
                   RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_LEFT_ENTITY;
  entity->dtype = *dtype;
  entity->node = node;
  entity->offset_from_bp = offset;
  return entity;
}

struct resolver_entity *resolver_create_new_unary_indirection_entity(
    struct resolver_process *process, struct resolver_result *result,
    struct node *node, int indirection_depth) {
  struct resolver_entity *entity = resolver_create_new_entity(
      result, RESOLVER_ENTITY_TYPE_UNARY_INDIRECTION, NULL);
  if (!entity) {
    return NULL;
  }

  entity->flags = RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_NEXT_ENTITY |
                  RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_LEFT_ENTITY;
  entity->node = node;
  entity->indirection.depth = indirection_depth;
  return entity;
}

struct resolver_entity *resolver_create_new_unary_get_address_entity(
    struct resolver_process *process, struct resolver_result *result,
    struct datatype *dtype, struct node *node, struct resolver_scope *scope,
    int offset) {
  struct resolver_entity *entity = resolver_create_new_entity(
      result, RESOLVER_ENTITY_TYPE_UNARY_GET_ADDRESS, NULL);
  if (!entity) {
    return NULL;
  }

  entity->scope = scope;
  entity->flags = RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_NEXT_ENTITY |
                  RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_LEFT_ENTITY;
  entity->node = node;
  entity->dtype = *dtype;
  entity->dtype.flags |= DATATYPE_FLAG_IS_POINTER;
  entity->dtype.pointer_depth++;
  return entity;
}

struct resolver_entity *
resolver_create_new_cast_entity(struct resolver_process *process,
                                struct resolver_scope *scope,
                                struct datatype *cast_dtype) {
  struct resolver_entity *entity =
      resolver_create_new_entity(NULL, RESOLVER_ENTITY_TYPE_CAST, NULL);
  if (!entity) {
    return NULL;
  }

  entity->scope = scope;
  entity->flags = RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_NEXT_ENTITY |
                  RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_LEFT_ENTITY;
  entity->dtype = *cast_dtype;
  return entity;
}

struct resolver_entity *resolver_create_new_entity_for_var_node_custom_scope(
    struct resolver_process *process, struct node *var_node, void *private,
    struct resolver_scope *scope, int offset) {
  assert(var_node->type == NODE_TYPE_VARIABLE);
  struct resolver_entity *entity =
      resolver_create_new_entity(NULL, RESOLVER_ENTITY_TYPE_VARIABLE, private);
  if (!entity) {
    return NULL;
  }

  entity->scope = scope;
  assert(entity->scope);
  entity->name = var_node->var.name;
  entity->dtype = var_node->var.type;
  entity->var_data.dtype = var_node->var.type;
  entity->node = var_node;
  entity->offset_from_bp = offset;
  return entity;
}

struct resolver_entity *
resolver_create_new_entity_for_var_node(struct resolver_process *process,
                                        struct node *var_node, void *private,
                                        int offset) {
  return resolver_create_new_entity_for_var_node_custom_scope(
      process, var_node, private, resolver_scope_current(process), offset);
}

struct resolver_entity *resolver_new_entity_for_var_node_no_push(
    struct resolver_process *process, struct node *var_node, void *private,
    int offset, struct resolver_scope *scope) {
  struct resolver_entity *entity =
      resolver_create_new_entity_for_var_node_custom_scope(
          process, var_node, private, scope, offset);
  if (!entity) {
    return NULL;
  }

  if (scope->flags & RESOLVER_SCOPE_FLAG_IS_STACK) {
    entity->flags |= RESOLVER_ENTITY_FLAG_IS_STACK;
  }

  return entity;
}

struct resolver_entity *
resolver_new_entity_for_var_node(struct resolver_process *process,
                                 struct node *var_node, void *private,
                                 int offset) {
  struct resolver_entity *entity = resolver_new_entity_for_var_node_no_push(
      process, var_node, private, offset,
      resolver_process_scope_current(process));
  if (!entity) {
    return NULL;
  }

  vector_push(process->scopes.current->entities, &entity);
  return entity;
}

void resolver_new_entity_for_rule(struct resolver_process *process,
                                  struct resolver_result *result,
                                  struct resolver_entity_rule *rule) {
  struct resolver_entity *entity =
      resolver_create_new_entity(result, RESOLVER_ENTITY_TYPE_RULE, NULL);
  entity->rule = *rule;
  resolver_result_entity_push(result, entity);
}

struct resolver_entity *resolver_make_entity(
    struct resolver_process *process, struct resolver_result *result,
    struct datatype *custom_dtype, struct node *node,
    struct resolver_entity *guided_entity, struct resolver_scope *scope) {
  struct resolver_entity *entity = NULL;
  int offset = guided_entity->offset_from_bp;
  int flags = guided_entity->flags;
  switch (node->type) {
  case NODE_TYPE_VARIABLE:
    entity = resolver_new_entity_for_var_node_no_push(process, node, NULL,
                                                      offset, scope);
    break;

  default:
    entity = resolver_create_new_unknown_entity(process, result, custom_dtype,
                                                node, scope, offset);
    break;
  }

  if (entity) {
    entity->flags |= flags;
    if (custom_dtype) {
      entity->dtype = *custom_dtype;
    }

    entity->private =
        process->callbacks.make_private(entity, node, offset, scope);
  }

  return entity;
}

struct resolver_entity *resolver_create_new_entity_for_function_call(
    struct resolver_result *result, struct resolver_process *process,
    struct resolver_entity *left_operand_entity, void *private) {
  struct resolver_entity *entity = resolver_create_new_entity(
      result, RESOLVER_ENTITY_TYPE_FUNCTION_CALL, private);
  if (!entity) {
    return NULL;
  }

  entity->dtype = left_operand_entity->dtype;
  entity->function_call_data.args = vector_create(sizeof(struct node *));
  return entity;
}

struct resolver_entity *
resolver_register_function(struct resolver_process *process,
                           struct node *func_node, void *private) {
  struct resolver_entity *entity =
      resolver_create_new_entity(NULL, RESOLVER_ENTITY_TYPE_FUNCTION, private);
  if (!entity) {
    return NULL;
  }

  entity->name = func_node->func.name;
  entity->node = func_node;
  entity->dtype = func_node->func.rtype;
  entity->scope = resolver_process_scope_current(process);
  vector_push(process->scopes.root->entities, &entity);
  return entity;
}

struct resolver_entity *resolver_get_entity_in_scope_with_entity_type(
    struct resolver_result *result, struct resolver_process *process,
    struct resolver_scope *scope, const char *entity_name, int entity_type) {
  // accessing a struct or union
  if (result && result->last_struct_union_entity) {
    struct resolver_scope *scope = result->last_struct_union_entity->scope;
    struct node *out_node = NULL;
    struct datatype *node_var_datatype =
        &result->last_struct_union_entity->dtype;
    int offset = struct_offset(resolver_process_compile(process),
                               node_var_datatype->type_str, entity_name,
                               &out_node, 0, 0);
    if (node_var_datatype->type == DATA_TYPE_UNION) {
      offset = 0;
    }

    return resolver_make_entity(
        process, result, NULL, out_node,
        &(struct resolver_entity){.type = RESOLVER_ENTITY_TYPE_VARIABLE,
                                  .offset_from_bp = offset},
        scope);
  }

  // accessing a primitive variable
  vector_set_peek_pointer_end(scope->entities);
  vector_set_flag(scope->entities, VECTOR_FLAG_PEEK_DECREMENT);
  struct resolver_entity *current = vector_peek_ptr(scope->entities);
  while (current) {
    if (entity_type != -1 && current->type != entity_type) {
      current = vector_peek_ptr(scope->entities);
      continue;
    }

    if (S_EQ(current->name, entity_name)) {
      break;
    }

    current = vector_peek_ptr(scope->entities);
  }

  return current;
}

struct resolver_entity *
resolver_get_entity_for_type(struct resolver_result *result,
                             struct resolver_process *process,
                             const char *entity_name, int entity_type) {
  struct resolver_scope *scope = process->scopes.current;
  struct resolver_entity *entity = NULL;
  while (scope) {
    entity = resolver_get_entity_in_scope_with_entity_type(
        result, process, scope, entity_name, entity_type);
    if (entity) {
      break;
    }

    scope = scope->prev;
  }

  if (entity) {
    memset(&entity->last_resolve, 0,
           sizeof(entity->last_resolve)); // spent days to fix this one line
  }

  return entity;
}

struct resolver_entity *resolver_get_entity(struct resolver_result *result,
                                            struct resolver_process *process,
                                            const char *entity_name) {
  return resolver_get_entity_for_type(result, process, entity_name, -1);
}

struct resolver_entity *resolver_get_entity_in_scope(
    struct resolver_result *result, struct resolver_process *process,
    struct resolver_scope *scope, const char *entity_name) {
  return resolver_get_entity_in_scope_with_entity_type(result, process, scope,
                                                       entity_name, -1);
}

struct resolver_entity *resolver_get_variable(struct resolver_result *result,
                                              struct resolver_process *process,
                                              const char *var_name) {
  return resolver_get_entity_for_type(result, process, var_name,
                                      RESOLVER_ENTITY_TYPE_VARIABLE);
}

struct resolver_entity *resolver_get_function_in_scope(
    struct resolver_result *result, struct resolver_process *process,
    const char *func_name, struct resolver_scope *scope) {
  return resolver_get_entity_for_type(result, process, func_name,
                                      RESOLVER_ENTITY_TYPE_FUNCTION);
}

struct resolver_entity *resolver_get_function(struct resolver_result *result,
                                              struct resolver_process *process,
                                              const char *func_name) {
  return resolver_get_function_in_scope(result, process, func_name,
                                        process->scopes.root);
}

struct resolver_entity *
resolver_follow_for_name(struct resolver_process *process, const char *name,
                         struct resolver_result *result) {
  struct resolver_entity *entity =
      resolver_entity_clone(resolver_get_entity(result, process, name));
  if (!entity) {
    return NULL;
  }

  resolver_result_entity_push(result, entity);

  // first found identifier
  if (!result->identifier) {
    result->identifier = entity;
  }

  if (entity->type == RESOLVER_ENTITY_TYPE_VARIABLE &&
          datatype_is_struct_or_union(&entity->var_data.dtype) ||
      entity->type == RESOLVER_ENTITY_TYPE_FUNCTION &&
          datatype_is_struct_or_union(&entity->dtype)) {
    result->last_struct_union_entity = entity;
  }

  return entity;
}

struct resolver_entity *
resolver_follow_identifier(struct resolver_process *process, struct node *node,
                           struct resolver_result *result) {
  struct resolver_entity *entity =
      resolver_follow_for_name(process, node->sval, result);
  if (entity) {
    entity->last_resolve.referencing_node = node;
  }

  return entity;
}

struct resolver_entity *
resolver_follow_variable(struct resolver_process *process,
                         struct node *var_node,
                         struct resolver_result *result) {
  return resolver_follow_for_name(process, var_node->var.name, result);
}

bool resolver_do_indirection(struct resolver_entity *entity) {
  struct resolver_result *res = entity->result;
  return entity->type != RESOLVER_ENTITY_TYPE_FUNCTION_CALL &&
         !(res->flags & RESOLVER_RESULT_FLAG_DOES_GET_ADDRESS) &&
         entity->type != RESOLVER_ENTITY_TYPE_CAST;
}

struct resolver_entity *
resolver_follow_struct_expression(struct resolver_process *process,
                                  struct node *node,
                                  struct resolver_result *result) {
  resolver_follow_part(process, node->exp.left, result);
  struct resolver_entity *left_entity = resolver_result_peek(result);
  struct resolver_entity_rule rule = {};
  if (is_access_node_with_op(node, "->")) {
    // do not merge with the next entity (.e.g. a->b)
    rule.left.flags = RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_NEXT_ENTITY;
    if (left_entity->type != RESOLVER_ENTITY_TYPE_FUNCTION_CALL) {
      rule.right.flags = RESOLVER_ENTITY_FLAG_DO_INDIRECTION;
    }
  }

  resolver_new_entity_for_rule(process, result, &rule);
  resolver_follow_part(process, node->exp.right, result);
  return NULL;
}

struct resolver_entity *
resolver_follow_array_expression(struct resolver_process *process,
                                 struct node *node,
                                 struct resolver_result *result) {
  resolver_follow_part(process, node->exp.left, result);
  struct resolver_entity *left_entity = resolver_result_peek(result);
  resolver_follow_part(process, node->exp.right, result);
  return left_entity;
}

struct datatype *resolver_get_datatype(struct resolver_process *process,
                                       struct node *node) {
  struct resolver_result *result = resolver_follow(process, node);
  if (!resolver_result_ok(result)) {
    return NULL;
  }

  return &result->last_entity->dtype;
}

void resolver_build_function_call_args(
    struct resolver_process *process, struct node *arg_node,
    struct resolver_entity *root_func_call_entity, size_t *total_size_out) {
  if (is_argument_node(arg_node)) {
    resolver_build_function_call_args(process, arg_node->exp.left,
                                      root_func_call_entity, total_size_out);
    resolver_build_function_call_args(process, arg_node->exp.right,
                                      root_func_call_entity, total_size_out);
  } else if (arg_node->type == NODE_TYPE_EXPRESSION_PARENTHESIS) {
    resolver_build_function_call_args(process, arg_node->paren.exp,
                                      root_func_call_entity, total_size_out);
  } else if (node_valid(arg_node)) {
    vector_push(root_func_call_entity->function_call_data.args, &arg_node);
    size_t stack_change = DATA_SIZE_DWORD;
    struct datatype *dtype = resolver_get_datatype(process, arg_node);
    if (dtype) {
      // 4 bytes for pointers unless it's a struct or union
      stack_change = datatype_element_size(dtype);
      if (stack_change < DATA_SIZE_DWORD) {
        stack_change = DATA_SIZE_DWORD;
      }

      stack_change = align_value(stack_change, DATA_SIZE_DWORD);
    }
    *total_size_out += stack_change;
  }
}

struct resolver_entity *
resolver_follow_function_call(struct resolver_process *process,
                              struct resolver_result *result,
                              struct node *node) {
  resolver_follow_part(process, node->exp.left, result);
  struct resolver_entity *left_entity = resolver_result_peek(result);
  struct resolver_entity *func_call_entity =
      resolver_create_new_entity_for_function_call(result, process, left_entity,
                                                   NULL);
  assert(func_call_entity);
  func_call_entity->flags |= RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_NEXT_ENTITY |
                             RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_LEFT_ENTITY;
  resolver_build_function_call_args(
      process, node->exp.right, func_call_entity,
      &func_call_entity->function_call_data.stack_size);

  // push function call entity to the stack
  resolver_result_entity_push(result, func_call_entity);
  return func_call_entity;
}

struct resolver_entity *
resolver_follow_parentheses_expression(struct resolver_process *process,
                                       struct node *node,
                                       struct resolver_result *result) {
  if (node->exp.left->type == NODE_TYPE_IDENTIFIER) {
    return resolver_follow_function_call(process, result, node);
  }

  return resolver_follow_expression(process, node->paren.exp, result);
}

struct resolver_entity *
resolver_follow_expression(struct resolver_process *process, struct node *node,
                           struct resolver_result *result) {
  struct resolver_entity *entity = NULL;
  if (is_access_node(node)) {
    entity = resolver_follow_struct_expression(process, node, result);
  } else if (is_array_node(node)) {
    entity = resolver_follow_array_expression(process, node, result);
  } else if (is_parentheses_node(node)) {
    entity = resolver_follow_parentheses_expression(process, node, result);
  }

  return entity;
}

void resolver_array_bracket_set_flags(struct resolver_entity *entity,
                                      struct datatype *dtype, struct node *node,
                                      int index) {
  if (!(dtype->flags & DATATYPE_FLAG_IS_ARRAY) ||
      array_brackets_count(dtype) <= index) {
    entity->flags = RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_NEXT_ENTITY |
                    RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_LEFT_ENTITY |
                    RESOLVER_ENTITY_FLAG_IS_POINTER_ARRAY_ENTITY;

  } else if (node->bracket.inner->type != NODE_TYPE_NUMBER) {
    entity->flags = RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_LEFT_ENTITY |
                    RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_NEXT_ENTITY;
  } else {
    entity->flags = RESOLVER_ENTITY_FLAG_JUST_USE_OFFSET;
  }
}

struct resolver_entity *
resolver_follow_array_bracket(struct resolver_process *process,
                              struct node *node,
                              struct resolver_result *result) {
  assert(node->type == NODE_TYPE_BRACKET);
  int index = 0;
  struct datatype dtype;
  struct resolver_scope *scope = NULL;
  struct resolver_entity *last_entity =
      resolver_result_peek_ignore_rule_entity(result);
  scope = last_entity->scope;
  dtype = last_entity->dtype;
  if (last_entity->type == RESOLVER_ENTITY_TYPE_ARRAY_BRACKET) {
    index = last_entity->array.index + 1;
  }

  if (dtype.flags & DATATYPE_FLAG_IS_ARRAY) {
    dtype.array.size = array_brackets_calculate_size_from_index(
        &dtype, dtype.array.brackets, index + 1);
  }

  // reduce dtype
  void *private = process->callbacks.new_array_entity(result, node);
  struct resolver_entity *array_bracket_entity =
      resolver_create_new_entity_for_array_bracket(result, process, node,
                                                   node->bracket.inner, index,
                                                   &dtype, private, scope);
  struct resolver_entity_rule rule = {};
  resolver_array_bracket_set_flags(array_bracket_entity, &dtype, node, index);
  last_entity->flags |= RESOLVER_ENTITY_FLAG_USES_ARRAY_BRACKETS;
  if (array_bracket_entity->flags &
      RESOLVER_ENTITY_FLAG_IS_POINTER_ARRAY_ENTITY) {
    datatype_decrement_pointer(&array_bracket_entity->dtype);
  }

  resolver_result_entity_push(result, array_bracket_entity);
  return array_bracket_entity;
}

struct resolver_entity *
resolver_follow_exp_parenheses(struct resolver_process *process,
                               struct node *node,
                               struct resolver_result *result) {
  return resolver_follow_part_return_entity(process, node->paren.exp, result);
}

struct resolver_entity *
resolver_follow_unsupported_unary_node(struct resolver_process *process,
                                       struct node *node,
                                       struct resolver_result *result) {
  return resolver_follow_part_return_entity(process, node->unary.operand,
                                            result);
}

struct resolver_entity *
resolver_follow_unsupported_node(struct resolver_process *process,
                                 struct node *node,
                                 struct resolver_result *result) {
  bool followed = false;
  switch (node->type) {
  case NODE_TYPE_UNARY:
    resolver_follow_unsupported_unary_node(process, node, result);
    followed = false;
    break;

  default:
    followed = false;
    break;
  }

  struct resolver_entity *unsupported_entity =
      resolver_create_new_entity_for_unsupported_node(result, node);
  assert(unsupported_entity);
  resolver_result_entity_push(result, unsupported_entity);
  return unsupported_entity;
}

struct resolver_entity *resolver_follow_cast(struct resolver_process *process,
                                             struct node *node,
                                             struct resolver_result *result) {
  struct resolver_entity *operand_entity = NULL;
  resolver_follow_unsupported_node(process, node->cast.exp, result);
  operand_entity = resolver_result_peek(result);
  operand_entity->flags |= RESOLVER_ENTITY_FLAG_WAS_CASTED;

  struct resolver_entity *cast_entity = resolver_create_new_cast_entity(
      process, operand_entity->scope, &node->cast.dtype);
  if (datatype_is_struct_or_union(&node->cast.dtype)) {
    if (!cast_entity->scope) {
      cast_entity->scope = process->scopes.current;
    }

    result->last_struct_union_entity = cast_entity;
  }

  resolver_result_entity_push(result, cast_entity);
  return cast_entity;
}

struct resolver_entity *
resolver_follow_indirection(struct resolver_process *process, struct node *node,
                            struct resolver_result *result) {
  // indirection (e.g. **a)
  resolver_follow_part(process, node->unary.operand, result);
  struct resolver_entity *last_entity = resolver_result_peek(result);
  if (!last_entity) {
    last_entity =
        resolver_follow_unsupported_node(process, node->unary.operand, result);
  }

  struct resolver_entity *unary_indirection_entity =
      resolver_create_new_unary_indirection_entity(
          process, result, node, node->unary.indirection.depth);
  resolver_result_entity_push(result, unary_indirection_entity);
  return unary_indirection_entity;
}

struct resolver_entity *
resolver_follow_unary_address(struct resolver_process *process,
                              struct node *node,
                              struct resolver_result *result) {
  // address (e.g. &a)
  result->flags |= RESOLVER_RESULT_FLAG_DOES_GET_ADDRESS;
  resolver_follow_part(process, node->unary.operand, result);
  struct resolver_entity *last_entity = resolver_result_peek(result);
  struct resolver_entity *unary_address_entity =
      resolver_create_new_unary_get_address_entity(
          process, result, &last_entity->dtype, node, last_entity->scope,
          last_entity->offset_from_bp);
  resolver_result_entity_push(result, unary_address_entity);
  return unary_address_entity;
}

struct resolver_entity *resolver_follow_unary(struct resolver_process *process,
                                              struct node *node,
                                              struct resolver_result *result) {
  struct resolver_entity *result_entity = NULL;
  if (op_is_indirection(node->unary.op)) {
    result_entity = resolver_follow_indirection(process, node, result);
  } else if (op_is_address(node->unary.op)) {
    result_entity = resolver_follow_unary_address(process, node, result);
  }

  return result_entity;
}

struct resolver_entity *
resolver_follow_part_return_entity(struct resolver_process *process,
                                   struct node *node,
                                   struct resolver_result *result) {
  struct resolver_entity *entity = NULL;
  switch (node->type) {
  case NODE_TYPE_IDENTIFIER:
    entity = resolver_follow_identifier(process, node, result);
    break;

  case NODE_TYPE_VARIABLE:
    entity = resolver_follow_variable(process, node, result);
    break;

  case NODE_TYPE_EXPRESSION:
    entity = resolver_follow_expression(process, node, result);
    break;

  case NODE_TYPE_BRACKET:
    entity = resolver_follow_array_bracket(process, node, result);
    break;

  case NODE_TYPE_EXPRESSION_PARENTHESIS:
    entity = resolver_follow_exp_parenheses(process, node, result);
    break;

  case NODE_TYPE_CAST:
    entity = resolver_follow_cast(process, node, result);
    break;

  case NODE_TYPE_UNARY:
    entity = resolver_follow_unary(process, node, result);
    break;

  default:
    // can't do anything, this will require runtime computation
    entity = resolver_follow_unsupported_node(process, node, result);
    break;
  }

  if (entity) {
    entity->result = result;
    entity->process = process;
  }

  return entity;
}

void resolver_follow_part(struct resolver_process *process, struct node *node,
                          struct resolver_result *result) {
  resolver_follow_part_return_entity(process, node, result);
}

void resolver_rule_apply_rules(struct resolver_entity *rule_entity,
                               struct resolver_entity *left_entity,
                               struct resolver_entity *right_entity) {
  assert(rule_entity->type == RESOLVER_ENTITY_TYPE_RULE);
  if (left_entity) {
    left_entity->flags |= rule_entity->rule.left.flags;
  }

  if (right_entity) {
    right_entity->flags |= rule_entity->rule.right.flags;
  }
}

void resolver_push_vector_of_entities(struct resolver_result *result,
                                      struct vector *entities) {
  vector_set_peek_pointer_end(entities);
  vector_set_flag(entities, VECTOR_FLAG_PEEK_DECREMENT);
  struct resolver_entity *entity = vector_peek_ptr(entities);
  while (entity) {
    resolver_result_entity_push(result, entity);
    entity = vector_peek_ptr(entities);
  }
}

void resolver_execute_rules(struct resolver_process *process,
                            struct resolver_result *result) {
  struct vector *saved_entities =
      vector_create(sizeof(struct resolver_entity *));
  struct resolver_entity *entity = resolver_result_pop(result);
  struct resolver_entity *last_processed_entity = NULL;
  while (entity) {
    if (entity->type == RESOLVER_ENTITY_TYPE_RULE) {
      struct resolver_entity *left_entity = resolver_result_pop(result);
      resolver_rule_apply_rules(entity, left_entity, last_processed_entity);
      entity = left_entity;
    }

    vector_push(saved_entities, &entity);
    last_processed_entity = entity;
    entity = resolver_result_pop(result);
  }

  resolver_push_vector_of_entities(result, saved_entities);
}

struct resolver_entity *resolver_merge_compile_time_result(
    struct resolver_process *process, struct resolver_result *result,
    struct resolver_entity *left_entity, struct resolver_entity *right_entity) {
  if (left_entity && right_entity) {
    if (left_entity->flags & RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_NEXT_ENTITY ||
        right_entity->flags & RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_LEFT_ENTITY) {
      goto no_merge_possible;
    }

    struct resolver_entity *result_entity = process->callbacks.merge_entities(
        process, result, left_entity, right_entity);
    if (!result_entity) {
      goto no_merge_possible;
    }

    return result_entity;
  }

no_merge_possible:
  return NULL;
}

void _resolver_merge_compile_times(struct resolver_process *process,
                                   struct resolver_result *result) {
  struct vector *saved_entities =
      vector_create(sizeof(struct resolver_entity *));
  while (42) {
    struct resolver_entity *right_entity = resolver_result_pop(result);
    struct resolver_entity *left_entity = resolver_result_pop(result);
    if (!right_entity) {
      break;
    }

    if (!left_entity) {
      // only one entity
      resolver_result_entity_push(result, right_entity);
      break;
    }

    struct resolver_entity *merged_entity = resolver_merge_compile_time_result(
        process, result, left_entity, right_entity);
    if (merged_entity) {
      resolver_result_entity_push(result, merged_entity);
      continue;
    }

    right_entity->flags |= RESOLVER_ENTITY_FLAG_NO_MERGE_WITH_LEFT_ENTITY;
    vector_push(saved_entities, &right_entity);
    // left entity goes back to the stack as we may merge it with the next
    resolver_result_entity_push(result, left_entity);
  }

  resolver_push_vector_of_entities(result, saved_entities);
  vector_free(saved_entities);
}

void resolver_merge_compile_times(struct resolver_process *process,
                                  struct resolver_result *result) {
  size_t total_entites = 0;
  do {
    total_entites = result->count;
    _resolver_merge_compile_times(process, result);
  } while (total_entites != 1 && total_entites != result->count);
}

void resolver_finalize_result_flags(struct resolver_process *process,
                                    struct resolver_result *result) {
  int flags = RESOLVER_RESULT_FLAG_FIRST_ENTITY_PUSH_VALUE;
  // iterate over all results
  struct resolver_entity *entity = result->entity;
  struct resolver_entity *first_entity = entity;
  struct resolver_entity *last_entity = result->last_entity;
  bool does_get_address = false;
  if (entity == last_entity) {
    // only one entity
    if (last_entity->type == RESOLVER_ENTITY_TYPE_VARIABLE &&
        datatype_is_struct_or_union_no_pointer(&last_entity->dtype)) {
      flags |= RESOLVER_RESULT_FLAG_FIRST_ENTITY_LOAD_TO_EBX;
      flags &= ~RESOLVER_RESULT_FLAG_FIRST_ENTITY_PUSH_VALUE;
    }

    result->flags = flags;
    return;
  }

  while (entity) {
    if (entity->flags & RESOLVER_ENTITY_FLAG_DO_INDIRECTION) {
      // load the address of first entity
      flags |= RESOLVER_RESULT_FLAG_FIRST_ENTITY_LOAD_TO_EBX |
               RESOLVER_RESULT_FLAG_FINAL_INDIRECTION_REQUIRED_FOR_VALUE;
      flags &= ~RESOLVER_RESULT_FLAG_FIRST_ENTITY_PUSH_VALUE;
    }

    if (entity->type == RESOLVER_ENTITY_TYPE_UNARY_GET_ADDRESS) {
      flags |= RESOLVER_RESULT_FLAG_FIRST_ENTITY_LOAD_TO_EBX |
               RESOLVER_RESULT_FLAG_DOES_GET_ADDRESS;
      flags &= ~RESOLVER_RESULT_FLAG_FIRST_ENTITY_PUSH_VALUE |
               RESOLVER_RESULT_FLAG_FINAL_INDIRECTION_REQUIRED_FOR_VALUE;
      does_get_address = true;
    }

    if (entity->type == RESOLVER_ENTITY_TYPE_FUNCTION_CALL) {
      flags |= RESOLVER_RESULT_FLAG_FIRST_ENTITY_LOAD_TO_EBX;
      flags &= ~RESOLVER_RESULT_FLAG_FIRST_ENTITY_PUSH_VALUE;
    }

    if (entity->type == RESOLVER_ENTITY_TYPE_ARRAY_BRACKET) {
      if (entity->dtype.flags & DATATYPE_FLAG_IS_POINTER) {
        flags |= RESOLVER_RESULT_FLAG_FIRST_ENTITY_PUSH_VALUE;
        flags &= ~RESOLVER_RESULT_FLAG_FIRST_ENTITY_LOAD_TO_EBX;
      } else {
        flags |= RESOLVER_RESULT_FLAG_FIRST_ENTITY_LOAD_TO_EBX;
        flags &= ~RESOLVER_RESULT_FLAG_FIRST_ENTITY_PUSH_VALUE;
      }

      if (entity->flags & RESOLVER_ENTITY_FLAG_IS_POINTER_ARRAY_ENTITY) {
        flags |= RESOLVER_RESULT_FLAG_FINAL_INDIRECTION_REQUIRED_FOR_VALUE;
      }
    }

    if (entity->type == RESOLVER_ENTITY_TYPE_GENERAL) {
      flags |= RESOLVER_RESULT_FLAG_FIRST_ENTITY_LOAD_TO_EBX |
               RESOLVER_RESULT_FLAG_FINAL_INDIRECTION_REQUIRED_FOR_VALUE;
      flags &= ~RESOLVER_RESULT_FLAG_FIRST_ENTITY_PUSH_VALUE;
    }

    entity = entity->next;
  }

  if (last_entity->dtype.flags & DATATYPE_FLAG_IS_ARRAY &&
      (!does_get_address &&
       last_entity->type == RESOLVER_ENTITY_TYPE_VARIABLE &&
       !(last_entity->flags & RESOLVER_ENTITY_FLAG_USES_ARRAY_BRACKETS))) {
    flags &= ~RESOLVER_RESULT_FLAG_FINAL_INDIRECTION_REQUIRED_FOR_VALUE;
  } else if (last_entity->type == RESOLVER_ENTITY_TYPE_VARIABLE) {
    flags |= RESOLVER_RESULT_FLAG_FINAL_INDIRECTION_REQUIRED_FOR_VALUE;
  }

  if (does_get_address) {
    flags &= ~RESOLVER_RESULT_FLAG_FINAL_INDIRECTION_REQUIRED_FOR_VALUE;
  }

  result->flags |= flags;
}

void resolver_finalize_unary(struct resolver_process *process,
                             struct resolver_result *result,
                             struct resolver_entity *entity) {
  struct resolver_entity *prev_entity = entity->prev;
  if (!prev_entity) {
    return;
  }

  entity->scope = prev_entity->scope;
  entity->dtype = prev_entity->dtype;
  entity->offset_from_bp = prev_entity->offset_from_bp;
  if (entity->type == RESOLVER_ENTITY_TYPE_UNARY_INDIRECTION) {
    int indirection_depth = entity->indirection.depth;
    entity->dtype.pointer_depth -= indirection_depth;
    if (entity->dtype.pointer_depth <= 0) {
      entity->dtype.pointer_depth = 0;
      entity->dtype.flags &= ~DATATYPE_FLAG_IS_POINTER;
    }
  } else if (entity->type == RESOLVER_ENTITY_TYPE_UNARY_GET_ADDRESS) {
    entity->dtype.flags |= DATATYPE_FLAG_IS_POINTER;
    entity->dtype.pointer_depth++;
  }
}

void resolver_finalize_last_entity(struct resolver_process *process,
                                   struct resolver_result *result) {
  struct resolver_entity *last_entity = resolver_result_peek(result);
  switch (last_entity->type) {
  case RESOLVER_ENTITY_TYPE_UNARY_INDIRECTION:
  case RESOLVER_ENTITY_TYPE_UNARY_GET_ADDRESS:
    resolver_finalize_unary(process, result, last_entity);
    break;
  }
}

void resolver_finalize_result(struct resolver_process *process,
                              struct resolver_result *result) {
  struct resolver_entity *first_entity = resolver_result_entity_root(result);
  if (!first_entity) {
    // nothing on the stack
    return;
  }

  process->callbacks.set_result_base(result, first_entity);
  resolver_finalize_result_flags(process, result);
  resolver_finalize_last_entity(process, result);
}

struct resolver_result *resolver_follow(struct resolver_process *process,
                                        struct node *node) {
  assert(process);
  assert(node);

  struct resolver_result *result = resolver_new_result(process);
  resolver_follow_part(process, node, result);
  if (!resolver_result_entity_root(result)) {
    result->flags |= RESOLVER_RESULT_FLAG_FAILED;
  }

  resolver_execute_rules(process, result);
  resolver_merge_compile_times(process, result);
  resolver_finalize_result(process, result);
  return result;
}
