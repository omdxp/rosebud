#ifndef STDIO_H
#define STDIO_H
#include <stdlib.h>

typedef struct _iobuf {
  char *_ptr;
  int _cnt;
  char *_base;
  int _flag;
  int _file;
  int _charbuf;
  int _bufsiz;
  char *_tmpfname;
} FILE;

FILE *fopen(const char *filename, const char *mode);
int fclose(FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream);
size_t fread(void *ptr, size_t size, size_t count, FILE *stream);
int printf(const char *format, ...);

#endif // STDIO_H
