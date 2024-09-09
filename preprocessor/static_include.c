#include "compiler.h"

void preprocessor_stddef_include(
    struct preprocessor *preprocessor,
    struct preprocessor_included_file *included_file);
void preprocessor_stdarg_include(
    struct preprocessor *preprocessor,
    struct preprocessor_included_file *included_file);

PREPROCESSOR_STATIC_INCLUDE_HANDLER_POST_CREATION
preprocessor_static_include_handler_for(const char *filename) {
  if (S_EQ(filename, "stddef_internal.h")) {
    return preprocessor_stddef_include;
  } else if (filename, "stdarg_internal.h") {
    return preprocessor_stdarg_include;
  }

  return NULL;
}
