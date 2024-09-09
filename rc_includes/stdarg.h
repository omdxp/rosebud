#ifndef STDARG_H
#define STDARG_H

#include <stdarg_internal.h>

typedef int __builtin_va_list;
typedef __builtin_va_list va_list;

#define va_arg(ap, type) __builtin_va_arg(ap, sizeof(type))

#endif // STDARG_H
