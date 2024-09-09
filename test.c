#include <stddef.h>

struct abc {
  int a;
  int b;
};

int main() { return offsetof(struct abc, b); }
