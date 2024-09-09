#include <stdio.h>

int main() {
  FILE *f = fopen("test.txt", "w");
  fwrite("Hello, World!\n", 1, 14, f);
  fclose(f);
}
