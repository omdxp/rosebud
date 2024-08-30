#include "compiler.h"
#include "helpers/vector.h"
#include <stdio.h>

int main(int argc, char **argv) {
  const char *input_file = "./test.c";
  const char *output_file = "./test";
  const char *option = "exec";
  if (argc > 1) {
    input_file = argv[1];
  }

  if (argc > 2) {
    output_file = argv[2];
  }

  if (argc > 3) {
    option = argv[3];
  }

  int compile_flags = COMPILE_PROCESS_EXEC_NASM;
  if (S_EQ(option, "object")) {
    compile_flags |= COMPILE_PROCESS_EXPORT_AS_OBJECT;
  }

  int res = compile_file(input_file, output_file, compile_flags);
  if (res == COMPILER_FILE_COMPILED_OK) {
    printf("compiled successfuly\n");
  } else if (res == COMPILER_FAILED_WITH_ERRORS) {
    printf("compile failed\n");
  } else {
    printf("unknown response for compiled file\n");
  }

  if (compile_flags & COMPILE_PROCESS_EXEC_NASM) {
    char nasm_output_file[40];
    char nasm_cmd[512];
    sprintf(nasm_output_file, "%s.o", output_file);
    if (compile_flags & COMPILE_PROCESS_EXPORT_AS_OBJECT) {
      sprintf(nasm_cmd, "nasm -f elf32 %s -o %s", output_file,
              nasm_output_file);
    } else {
      sprintf(nasm_cmd, "nasm -f elf32 %s", output_file);
    }

    printf("executing nasm command: %s\n", nasm_cmd);
    int nasm_res = system(nasm_cmd);
    if (nasm_res != 0) {
      printf("nasm failed\n");
    } else {
      printf("nasm executed successfuly\n");
    }

    return nasm_res;
  }
  return 0;
}
