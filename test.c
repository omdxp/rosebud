
int main() {
  int x = 3;
  switch (x) {
  case 1:
    x = 100;
    break;

  case 2:
    x = 120;
    break;

  default:
    x = 150;
  }

  return x;
}
