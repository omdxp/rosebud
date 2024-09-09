#include "compiler.h"
#include "helpers/vector.h"

// va_start(va_list, last_arg)
void native_va_start(struct generator *generator, struct native_function *func,
                     struct vector *args) {
  struct compile_process *compiler = generator->compiler;
  if (vector_count(args) != 2) {
    compiler_error(compiler, "va_start expects 2 arguments, %d given",
                   vector_count(args));
  }

  struct node *list_arg = vector_peek_ptr(args);
  struct node *stack_arg = vector_peek_ptr(args);
  if (stack_arg->type != NODE_TYPE_IDENTIFIER) {
    compiler_error(compiler,
                   "va_start expects an identifier as the second argument");
  }

  generator->asm_push("; va_start on %s", stack_arg->sval);
  vector_set_peek_pointer(args, 0);
  generator->generate_expression(generator, stack_arg, EXPRESSION_GET_ADDRESS);

  struct resolver_result *result =
      resolver_follow(compiler->resolver, list_arg);
  assert(resolver_result_ok(result));

  struct resolver_entity *list_arg_entity = resolver_result_entity_root(result);
  struct generator_entity_address address_out;
  generator->entity_address(generator, list_arg_entity, &address_out);
  generator->asm_push("mov dword [%s], ebx", address_out.address);
  generator->asm_push("; va_start end for %s", stack_arg->sval);

  struct datatype void_datatype;
  datatype_set_void(&void_datatype);
  generator->ret(&void_datatype, "0");
}

// va_arg(va_list, type)
void native_builtin_va_arg(struct generator *generator,
                           struct native_function *func, struct vector *args) {
  struct compile_process *compiler = generator->compiler;
  if (vector_count(args) != 2) {
    compiler_error(compiler, "va_arg expects 2 arguments, %d given",
                   vector_count(args));
  }

  generator->asm_push("; native __builtin_va_arg start");
  vector_set_peek_pointer(args, 0);
  struct node *list_arg = vector_peek_ptr(args);
  generator->generate_expression(generator, list_arg, EXPRESSION_GET_ADDRESS);

  struct node *size_arg = vector_peek_ptr(args);
  if (size_arg->type != NODE_TYPE_NUMBER) {
    compiler_error(compiler, "va_arg expects a number as the second argument");
  }

  generator->asm_push("add dword [ebx], %d", size_arg->llnum);
  generator->asm_push("mov dword eax, [ebx]");
  struct datatype void_dtype;
  datatype_set_void(&void_dtype);
  void_dtype.pointer_depth++;
  void_dtype.flags |= DATATYPE_FLAG_IS_POINTER;
  generator->ret(&void_dtype, "dword [eax]");
  generator->asm_push("; native __builtin_va_arg end");
}

// va_end(va_list)
void native_va_end(struct generator *generator, struct native_function *func,
                   struct vector *args) {
  struct compile_process *compiler = generator->compiler;
  if (vector_count(args) != 1) {
    compiler_error(compiler, "va_end expects 1 argument, %d given",
                   vector_count(args));
  }

  struct node *list_arg = vector_peek_ptr(args);
  if (list_arg->type != NODE_TYPE_IDENTIFIER) {
    compiler_error(compiler, "va_end expects an identifier as the argument");
  }

  generator->asm_push("; va_end start for %s", list_arg->sval);
  vector_set_peek_pointer(args, 0);
  generator->generate_expression(generator, list_arg, EXPRESSION_GET_ADDRESS);
  generator->asm_push("mov dword [ebx], 0");
  generator->asm_push("; va_end end for %s", list_arg->sval);

  struct datatype void_datatype;
  datatype_set_void(&void_datatype);
  generator->ret(&void_datatype, "0");
}

void preprocessor_stdarg_include(
    struct preprocessor *preprocessor,
    struct preprocessor_included_file *included_file) {
  native_create_function(preprocessor->compiler, "va_start",
                         &(struct native_function_callbacks){
                             .call = native_va_start,
                         });

  native_create_function(preprocessor->compiler, "__builtin_va_arg",
                         &(struct native_function_callbacks){
                             .call = native_builtin_va_arg,
                         });

  native_create_function(preprocessor->compiler, "va_end",
                         &(struct native_function_callbacks){
                             .call = native_va_end,
                         });
}
