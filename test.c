#include <stdarg.h>

int sum(int num, ...) {
  int res = 0;
  va_list args;
  va_start(args, num);
  int i;
  for (i = 0; i < num; i++) {
    res += va_arg(args, int);
  }
  va_end(args);
  return res;
}

int main() { return sum(10, 20, 30); }
