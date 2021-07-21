#include "adc_8080_dasm.h"

#include <stdio.h>

// TODO: Proper unit testing!

#define ORG_ADDR 0x100

static int read_program(const char *filepath, uint8_t *memory) {
  FILE *file = fopen(filepath, "rb");
  if (!file) {
    fprintf(stderr,
            "\n\n##### Test '%s' failed!\n"
            "Error: Failed to fopen() the program file!\n",
            filepath);
    return -1;
  }

  fseek(file, 0, SEEK_END);
  int size = ftell(file);
  rewind(file);

  int bytes_read = fread(memory + ORG_ADDR, 1, size, file);
  if (bytes_read != size) {
    fprintf(stderr,
            "\n\n##### Test '%s' failed!\n"
            "Error: Failed to read the rom file into memory! Read %d "
            "bytes, total is %d bytes\n",
            filepath, bytes_read, size);
    return -1;
  }

  return size;
}

int main() {
  uint8_t memory[0x10000];

  int size = read_program("roms/TST8080.COM", memory);
  if (size < 0)
    return -1;

  adc_8080_dasm_disassembly *dasm =
      adc_8080_dasm_disassemble(memory, size, ORG_ADDR);

  // NOTE: Demonstrating how to list disassembly!
  // const adc_8080_dasm_op *ops[16];
  // int num_ops = adc_8080_dasm_list(dasm, ops, 16, 0x0256);
  // for (int i = 0; i < num_ops; i++) {
  // char *opstr = adc_8080_dasm_op_to_string(dasm, ops[i]);
  // if (opstr) {
  // printf("%s\n", opstr);
  // free(opstr);
  // }
  // }

  // Print all disassembly.
  for (int i = 0; i < dasm->num_ops; i++) {
    char *opstr = adc_8080_dasm_op_to_string(dasm, &dasm->ops[i]);
    if (opstr) {
      printf("%s\n", opstr);
      free(opstr);
    }
  }

  adc_8080_dasm_free(&dasm);
  return 0;
}
