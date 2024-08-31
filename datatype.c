#include "compiler.h"
#include <stdlib.h>

bool datatype_is_struct_or_union_for_name(const char *name) {
  return S_EQ(name, "struct") || S_EQ(name, "union");
}

bool datatype_is_struct_or_union(struct datatype *dtype) {
  return dtype->type == DATA_TYPE_STRUCT || dtype->type == DATA_TYPE_UNION;
}

size_t datatype_size_for_array_access(struct datatype *dtype) {
  if (datatype_is_struct_or_union(dtype) &&
      dtype->flags & DATATYPE_FLAG_IS_POINTER && dtype->pointer_depth == 1) {
    // struct abc* abc; abc[0];
    return dtype->size;
  }

  return datatype_size(dtype);
}

size_t datatype_element_size(struct datatype *dtype) {
  if (dtype->flags & DATATYPE_FLAG_IS_POINTER) {
    return DATA_SIZE_DWORD;
  }

  return dtype->size;
}

size_t datatype_size_no_ptr(struct datatype *dtype) {
  if (dtype->flags & DATATYPE_FLAG_IS_ARRAY) {
    return dtype->array.size;
  }

  return dtype->size;
}

size_t datatype_size(struct datatype *dtype) {
  if (dtype->flags & DATATYPE_FLAG_IS_POINTER && dtype->pointer_depth > 0) {
    return DATA_SIZE_DWORD;
  }

  if (dtype->flags & DATATYPE_FLAG_IS_ARRAY) {
    return dtype->array.size;
  }

  return dtype->size;
}

bool datatype_is_primitive(struct datatype *dtype) {
  return !datatype_is_struct_or_union(dtype);
}

bool datatype_is_struct_or_union_no_pointer(struct datatype *dtype) {
  return dtype->type != DATA_TYPE_UNKNOWN && !datatype_is_primitive(dtype) &&
         !(dtype->flags & DATATYPE_FLAG_IS_POINTER);
}

struct datatype datatype_for_numeric() {
  struct datatype dtype = {};
  dtype.flags |= DATATYPE_FLAG_IS_LITERAL;
  dtype.type = DATA_TYPE_INT;
  dtype.type_str = "int";
  dtype.size = DATA_SIZE_DWORD;
  return dtype;
}

struct datatype *datatype_thats_a_pointer(struct datatype *d1,
                                          struct datatype *d2) {
  if (d1->flags & DATATYPE_FLAG_IS_POINTER) {
    return d1;
  }

  if (d2->flags & DATATYPE_FLAG_IS_POINTER) {
    return d2;
  }

  return NULL;
}

struct datatype *datatype_pointer_reduce(struct datatype *dtype, int by) {
  struct datatype *new_datatype = calloc(1, sizeof(struct datatype));
  memcpy(new_datatype, dtype, sizeof(struct datatype));
  new_datatype->pointer_depth -= by;
  if (new_datatype->pointer_depth <= 0) {
    new_datatype->flags &= ~DATATYPE_FLAG_IS_POINTER;
    new_datatype->pointer_depth = 0;
  }

  return new_datatype;
}
