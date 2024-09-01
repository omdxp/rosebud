union abc {
  int x;
  int y;
};

union abc a;

int main() {
  a.x = 20;
  return a.y;
}
