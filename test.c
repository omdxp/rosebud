struct book book;

struct book {
  char title[50];
  char author[50];
  char subject[100];
  int book_id;
};

int test(char *fmt) { return 1; }

int main() {
  struct book *books;
  return test(56, books[0].title, 100);
}
