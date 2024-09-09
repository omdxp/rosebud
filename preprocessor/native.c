#include "compiler.h"

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
