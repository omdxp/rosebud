#define TEST(a, b) a##b

int printf(const char *fmt, ...);

int main() { printf("%i", TEST(20, 30)); }
