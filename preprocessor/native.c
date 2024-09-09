#include "compiler.h"
#include <stdlib.h>

int preprocessor_line_macro_evaluate(struct preprocessor_definition *def,
                                     struct preprocessor_function_args *args) {
  struct preprocessor *preprocessor = def->preprocessor;
  struct compile_process *compiler = preprocessor->compiler;
  if (args) {
    compiler_error(compiler, "line macro does not accept arguments");
  }

  struct token *prev_token = preprocessor_prev_token(compiler);
  return prev_token->pos.line;
}

struct vector *
preprocessor_line_macro_value(struct preprocessor_definition *def,
                              struct preprocessor_function_args *args) {
  struct preprocessor *preprocessor = def->preprocessor;
  struct compile_process *compiler = preprocessor->compiler;
  if (args) {
    compiler_error(compiler, "line macro does not accept arguments");
  }

  struct token *prev_token = preprocessor_prev_token(compiler);
  return preprocessor_build_value_vector_for_integer(prev_token->pos.line);
}

void preprocessor_create_defs(struct preprocessor *preprocessor) {
  preprocessor_definition_create_native(
      "__LINE__", preprocessor_line_macro_evaluate,
      preprocessor_line_macro_value, preprocessor);
}

struct symbol *
native_create_function(struct compile_process *compiler, const char *name,
                       struct native_function_callbacks *callbacks) {
  struct native_function *native_function =
      malloc(sizeof(struct native_function));
  memcpy(&native_function->callbacks, callbacks,
         sizeof(native_function->callbacks));
  native_function->name = name;
  return symresolver_register_symbol(
      compiler, name, SYMBOL_TYPE_NATIVE_FUNCTION, native_function);
}

struct native_function *native_function_get(struct compile_process *compiler,
                                            const char *name) {
  struct symbol *symbol =
      symresolver_get_symbol_for_native_function(compiler, name);
  if (!symbol) {
    return NULL;
  }

  return symbol->data;
}
