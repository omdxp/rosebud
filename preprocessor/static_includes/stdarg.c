#include "compiler.h"

void native_test_function(struct generator *generator,
                          struct native_function *func, struct vector *args) {
  generator->asm_push("; test");
  struct datatype dtype;
  dtype.type = DATA_TYPE_INT;
  dtype.size = sizeof(int);
  generator->ret(&dtype, "42");
}

void preprocessor_stdarg_include(
    struct preprocessor *preprocessor,
    struct preprocessor_included_file *included_file) {
#warning "create va_list"
  native_create_function(
      preprocessor->compiler, "test",
      &(struct native_function_callbacks){.call = native_test_function});
}
