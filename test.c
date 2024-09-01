struct abc {
  int a;
  int b;
  int c;
};

struct abc get_abc() {
  struct abc abc;
  abc.c = 2;
  return abc;
}

struct abc x;

int main() {
  x = get_abc();
  return x.c;
}
