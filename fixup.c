#include "compiler.h"
#include "helpers/vector.h"
#include <stdlib.h>

struct fixup;

// return true if the fixup is done
typedef bool (*FIXUP_FIX)(struct fixup *fixup);

// signify the end of the fixup and free any resources
typedef void (*FIXUP_END)(struct fixup *fixup);

struct fixup_config {
  FIXUP_FIX fix;
  FIXUP_END end;
  void *private;
};

struct fixup_system {
  struct vector *fixups;
};

enum {
  FIXUP_FLAG_RESOLVED = 0b00000001,
};

struct fixup {
  int flags;
  struct fixup_system *system;
  struct fixup_config config;
};

struct fixup_system *fixup_sys_new() {
  struct fixup_system *system = calloc(1, sizeof(struct fixup_system));
  system->fixups = vector_create(sizeof(struct fixup));
  return system;
}

struct fixup_config *fixup_config(struct fixup *fixup) {
  return &fixup->config;
}

void fixup_free(struct fixup *fixup) {
  fixup->config.end(fixup);
  free(fixup);
}

void fixup_start_iteration(struct fixup_system *system) {
  vector_set_peek_pointer(system->fixups, 0);
}

struct fixup *fixup_next(struct fixup_system *system) {
  return vector_peek_ptr(system->fixups);
}

void fixup_sys_fixups_free(struct fixup_system *system) {
  fixup_start_iteration(system);
  struct fixup *fixup = fixup_next(system);
  while (fixup) {
    fixup_free(fixup);
    fixup = fixup_next(system);
  }
}

void fixup_sys_free(struct fixup_system *system) {
  fixup_sys_fixups_free(system);
  vector_free(system->fixups);
  free(system);
}

int fixup_sys_unresolved_count(struct fixup_system *system) {
  int count = 0;
  fixup_start_iteration(system);
  struct fixup *fixup = fixup_next(system);
  while (fixup) {
    if (!(fixup->flags & FIXUP_FLAG_RESOLVED)) {
      count++;
    }
    fixup = fixup_next(system);
  }
  return count;
}

struct fixup *fixup_register(struct fixup_system *system,
                             struct fixup_config *config) {
  struct fixup *fixup = calloc(1, sizeof(struct fixup));
  fixup->system = system;
  memcpy(&fixup->config, config, sizeof(struct fixup_config));
  vector_push(system->fixups, fixup);
  return fixup;
}

bool fixup_resolve(struct fixup *fixup) {
  if (fixup_config(fixup)->fix(fixup)) {
    fixup->flags |= FIXUP_FLAG_RESOLVED;
    return true;
  }

  return false;
}

void *fixup_private(struct fixup *fixup) {
  return fixup_config(fixup)->private;
}

bool fixups_resolve(struct fixup_system *system) {
  fixup_start_iteration(system);
  struct fixup *fixup = fixup_next(system);
  while (fixup) {
    if (fixup->flags & FIXUP_FLAG_RESOLVED) {
      continue;
    }

    fixup_resolve(fixup);
    fixup = fixup_next(system);
  }

  return fixup_sys_unresolved_count(system) == 0;
}
