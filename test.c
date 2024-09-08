#define TEST_FUNC(s) #s

int printf(const char *format, ...);

int main() {
  const char *s = TEST_FUNC(Hello World !);
  printf("%s\n", s);
}
