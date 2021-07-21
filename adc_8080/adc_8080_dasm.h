// adc_8080_dasm Intel 8080 disassembler library by Anthony Del Ciotto.
// Implements a disassembler for the Intel 8080 instruction set.

#ifndef _ADC_8080_DASM_H_
#define _ADC_8080_DASM_H_

#ifdef __cplusplus
extern "C" {
#endif

// 0.1.0
#define ADC_8080_DASM_VERSION_MAJOR 0
#define ADC_8080_DASM_VERSION_MINOR 1
#define ADC_8080_DASM_VERSION_PATCH 0

#include <stdint.h> // For int types.
#include <stdlib.h> // For size_t, malloc, free, bsearch

// Condition bits affected by an operation.
enum adc_8080_dasm_condbits_affected {
  ADC_8080_DASM_CONDBITS_NONE,
  ADC_8080_DASM_CONDBITS_CY,
  ADC_8080_DASM_CONDBITS_SZACP,
  ADC_8080_DASM_CONDBITS_ALL
};

// An operation definition.
typedef struct {
  uint8_t code;
  const char *mnemonic;
  size_t size;
  enum adc_8080_dasm_condbits_affected condbits_affected;
  const char *desc;
} adc_8080_dasm_opdef;

// A disassembled operation.
typedef struct {
  adc_8080_dasm_opdef def;
  uint16_t addr;
  int index;
} adc_8080_dasm_op;

// ADC_8080_DASM_OP_NOTFOUND() - Macro for checking if an op is not found.
#define ADC_8080_DASM_OP_NOTFOUND(op)                                          \
  ((op) && (op->def.size == 0 && op->addr == 0))

typedef struct {
  const uint8_t *memory;
  size_t program_size;
  adc_8080_dasm_op *ops;
  size_t num_ops;
} adc_8080_dasm_disassembly;

// adc_8080_dasm_disasssemble() - Disassembles the given program data.
//
// memory       - Pointer to memory containing the program data.
// program_size - Size of the program in bytes.
// org_addr     - Address in memory where the program starts.
//
// Returns a pointer to a new adc_8080_dasm_disassembly struct.
// Returns NULL on fatal error conditions.
adc_8080_dasm_disassembly *adc_8080_dasm_disassemble(const uint8_t *memory,
                                                     size_t program_size,
                                                     uint16_t org_addr);

// adc_8080_dasm_free() - Free the disassembly resources.
void adc_8080_dasm_free(adc_8080_dasm_disassembly **dasm);

// adc_8080_dasm_find() - Find a disassembled op given an address.
//
// Returns a pointer to a const adc_8080_dasm_op.
// Returns special not_found op if no disassembly exists at the given address.
// Use ADC_8080_DASM_NOTFOUND(op) macro to test for this special case.
const adc_8080_dasm_op *adc_8080_dasm_find(adc_8080_dasm_disassembly *dasm,
                                           uint16_t addr);

// adc_8080_dasm_list() - List n disassembled ops around the given address.
// Will fill the given dst buffer with num_lines/2 ops before and after the
// op at the given address.
//
// The start and end ops are bounds checked and the lowest possible op
// will be the first in the program, while the highest will be the last.
//
// Returns the number of ops filled into the dst buffer.
int adc_8080_dasm_list(adc_8080_dasm_disassembly *dasm,
                       const adc_8080_dasm_op *dst[], size_t num_lines,
                       uint16_t addr);

// adc_8080_dasm_op_to_string() - Returns a string representation of the op.
//
// Returns a pointer to the string which is dynamically allocated. It must be
// free'd by the caller.
char *adc_8080_dasm_op_to_string(adc_8080_dasm_disassembly *dasm,
                                 const adc_8080_dasm_op *op);

#ifdef __cplusplus
}
#endif

#endif // _ADC_8080_DASM_H_
