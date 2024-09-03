int printf(const char *s, ...);

int main() {
  int x = 1;
  x++;
  ++x;
  x--;
  --x;
  printf("%d\n", ++x);
}
