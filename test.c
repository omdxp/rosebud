struct abc {
  int x;
  int y;
};

int main() { return &((struct abc *)0x00)->y; }
