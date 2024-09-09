#include "compiler.h"
#include <stdarg.h>
#include <stdlib.h>

struct lex_process_functions compiler_lex_functions = {
    .next_char = compile_process_next_char,
    .peek_char = compile_process_peek_char,
    .push_char = compile_process_push_char};

void compiler_error(struct compile_process *compiler, const char *msg, ...) {
  va_list args;
  va_start(args, msg);
  vfprintf(stderr, msg, args);
  va_end(args);
  fprintf(stderr, " on line %i, col %i in file %s\n", compiler->pos.line,
          compiler->pos.col, compiler->pos.filename);
  exit(-1);
}

void compiler_warning(struct compile_process *compiler, const char *msg, ...) {
  va_list args;
  va_start(args, msg);
  vfprintf(stderr, msg, args);
  va_end(args);
  fprintf(stderr, " on line %i, col %i in file %s\n", compiler->pos.line,
          compiler->pos.col, compiler->pos.filename);
}

struct compile_process *
compile_include_for_include_dir(const char *include_dir, const char *filename,
                                struct compile_process *parent_process) {
  char tmp_filename[512];
  sprintf(tmp_filename, "%s/%s", include_dir, filename);
  if (file_exists(tmp_filename)) {
    filename = tmp_filename;
  }

  struct compile_process *new_process = compile_process_create(
      filename, NULL, parent_process->flags, parent_process);
  if (!new_process) {
    return NULL;
  }

  struct lex_process *lex_process =
      lex_process_create(new_process, &compiler_lex_functions, NULL);
  if (!lex_process) {
    return NULL;
  }

  if (lex(lex_process) != LEXICAL_ANALYSIS_ALL_OK) {
    return NULL;
  }

  new_process->token_vec_original = lex_process_tokens(lex_process);

  if (preprocessor_run(new_process) != PREPROCESS_ALL_OK) {
    return NULL;
  }

  return new_process;
}

// Compile include file with only lexing and preprocessing
struct compile_process *
compile_include(const char *filename, struct compile_process *parent_process) {
  struct compile_process *new_process = NULL;
  const char *include_dir = compiler_include_dir_begin(parent_process);
  while (include_dir && !new_process) {
    new_process =
        compile_include_for_include_dir(include_dir, filename, parent_process);
    include_dir = compiler_include_dir_next(parent_process);
  }

  return new_process;
}

int compile_file(const char *filename, const char *out_filename, int flags) {
  struct compile_process *process =
      compile_process_create(filename, out_filename, flags, NULL);
  if (!process)
    return COMPILER_FAILED_WITH_ERRORS;

  // Perform lexical analysis
  struct lex_process *lex_process =
      lex_process_create(process, &compiler_lex_functions, NULL);
  if (!lex_process) {
    return COMPILER_FAILED_WITH_ERRORS;
  }

  if (lex(lex_process) != LEXICAL_ANALYSIS_ALL_OK) {
    return COMPILER_FAILED_WITH_ERRORS;
  }

  process->token_vec_original = lex_process_tokens(lex_process);

  // Perform preprocessing
  if (preprocessor_run(process) != PREPROCESS_ALL_OK) {
    return COMPILER_FAILED_WITH_ERRORS;
  }

  // Perform parsing
  if (parse(process) != PARSE_ALL_OK) {
    return COMPILER_FAILED_WITH_ERRORS;
  }

  // Perform validation
  if (validate(process) != VALIDATION_ALL_OK) {
    return COMPILER_FAILED_WITH_ERRORS;
  }

  // Perfom code generation
  if (codegen(process) != CODEGEN_ALL_OK) {
    return COMPILER_FAILED_WITH_ERRORS;
  }

  fclose(process->ofile);

  return COMPILER_FILE_COMPILED_OK;
}
