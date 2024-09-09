#ifndef STDDEF_H
#define STDDEF_H

#include "stddef_internal.h"

#define offsetof(type, member) &((type *)0)->member

#endif // STDDEF_H
