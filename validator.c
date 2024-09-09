#include "compiler.h"

void validate_init(struct compile_process *process) {
  // TODO: Implement
}

void validate_destroy(struct compile_process *process) {
  // TODO: Implement
}

int validate_tree() { return VALIDATION_ALL_OK; }

int validate(struct compile_process *process) {
  int res = 0;
  validate_init(process);
  res = validate_tree();
  validate_destroy(process);
  return res;
}
