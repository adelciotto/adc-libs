#include "adc_8080_dasm.h"

#include <assert.h> // For assert
#include <stdio.h>  // For snprintf
#include <string.h> // For strncpy

static const adc_8080_dasm_opdef s_dasm_lut[256];

static const adc_8080_dasm_op s_op_notfound = {
    .def =
        {
            .code = 0x00,
            .mnemonic = "NO DASM",
            .size = 0,
            .condbits_affected = ADC_8080_DASM_CONDBITS_NONE,
            .desc = "No disassembly available",
        },
    .addr = 0x000,
};

adc_8080_dasm_disassembly *adc_8080_dasm_disassemble(const uint8_t *memory,
                                                     size_t program_size,
                                                     uint16_t org_addr) {
  if (!memory)
    goto error;

  // Programs typically start at a specific address in memory (given the program
  // ORG address). So we offset the start and end by the org address.
  // Start = 0x0000 + org_addr.
  // End = program_size + org_addr.
  uint16_t start_addr = org_addr;
  uint16_t end_addr = program_size + org_addr;

  // Count the number of ops in the program and allocate space for ops array.
  size_t num_ops = 0;
  for (uint16_t addr = start_addr; addr < end_addr;
       addr += s_dasm_lut[memory[addr]].size)
    num_ops++;
  adc_8080_dasm_op *ops = malloc(sizeof(adc_8080_dasm_op) * num_ops);
  if (!ops)
    goto error;

  // Iterate through program bytes and disassemble into ops.
  uint16_t addr = start_addr;
  int i = 0;
  while (addr < end_addr) {
    adc_8080_dasm_opdef opdef = s_dasm_lut[memory[addr]];
    ops[i] = (adc_8080_dasm_op){.def = opdef, .addr = addr, .index = i};
    addr += opdef.size;
    i++;
  }

  // Allocate and initialize the disassembly struct.
  adc_8080_dasm_disassembly *dasm = malloc(sizeof(adc_8080_dasm_disassembly));
  if (!dasm)
    goto error;

  dasm->memory = memory;
  dasm->program_size = program_size;
  dasm->ops = ops;
  dasm->num_ops = num_ops;
  return dasm;

error:
  return NULL;
}

void adc_8080_dasm_free(adc_8080_dasm_disassembly **dasm) {
  if (dasm) {
    if ((*dasm)->ops)
      free((*dasm)->ops);

    free(*dasm);
  }
  *dasm = NULL;
}

static int compareops(const void *a, const void *b) {
  uint16_t addr = *(uint16_t *)a;
  adc_8080_dasm_op *op = (adc_8080_dasm_op *)b;
  if (addr < op->addr)
    return -1;
  if (addr == op->addr)
    return 0;
  // addr > op->addr
  return 1;
}

const adc_8080_dasm_op *adc_8080_dasm_find(adc_8080_dasm_disassembly *dasm,
                                           uint16_t addr) {
  if (!dasm)
    return &s_op_notfound;

  const adc_8080_dasm_op *op = (const adc_8080_dasm_op *)bsearch(
      &addr, dasm->ops, dasm->num_ops, sizeof(*dasm->ops), compareops);
  if (!op)
    return &s_op_notfound;

  return op;
}

int adc_8080_dasm_list(adc_8080_dasm_disassembly *dasm,
                       const adc_8080_dasm_op *dst[], size_t num_lines,
                       uint16_t addr) {
  const adc_8080_dasm_op *op = adc_8080_dasm_find(dasm, addr);
  if (ADC_8080_DASM_OP_NOTFOUND(op))
    return 0;

  // Get n/2 lines of disassembly below and above the input address.
  size_t n = num_lines / 2;

  // Ensure the min and max index are within bounds.
  int imin = op->index - n;
  if (imin < 0)
    imin = 0;
  int imax = op->index + n;
  if (imax >= dasm->num_ops)
    imax = dasm->num_ops - 1;

  for (int i = imin, j = 0; i <= imax; i++, j++)
    dst[j] = &dasm->ops[i];
  return imax - imin;
}

#define WORD_FROM_BYTES(high, low) (uint16_t)(((high) << 8) | (low))
#define SNPRINTF_BUFSIZE 64

static void mnemonic_to_string(adc_8080_dasm_disassembly *dasm, char *dst,
                               size_t num, const adc_8080_dasm_op *op) {
  assert(dst);
  assert(num == SNPRINTF_BUFSIZE);

  switch (op->def.size) {
  case 1:
    strncpy(dst, op->def.mnemonic, num);
    break;
  case 2:
    snprintf(dst, num, op->def.mnemonic, dasm->memory[op->addr + 1]);
    break;
  case 3:
    snprintf(dst, num, op->def.mnemonic,
             WORD_FROM_BYTES(dasm->memory[op->addr + 2],
                             dasm->memory[op->addr + 1]));
    break;
  default:
    strcpy(dst, "");
    break;
  }
}

static void condbits_to_string(char *dst, size_t num,
                               const adc_8080_dasm_op *op) {
  switch (op->def.condbits_affected) {
  case ADC_8080_DASM_CONDBITS_CY:
    strncpy(dst, "cy", num);
    break;
  case ADC_8080_DASM_CONDBITS_SZACP:
    strncpy(dst, "z,s,p,ac", num);
    break;
  case ADC_8080_DASM_CONDBITS_ALL:
    strncpy(dst, "z,s,p,ac,cy", num);
    break;
  case ADC_8080_DASM_CONDBITS_NONE:
    // Fallthrough
  default:
    strcpy(dst, "none");
    break;
  }
}

char *adc_8080_dasm_op_to_string(adc_8080_dasm_disassembly *dasm,
                                 const adc_8080_dasm_op *op) {
  assert(op);

  char mnemonicstr[SNPRINTF_BUFSIZE];
  mnemonic_to_string(dasm, mnemonicstr, SNPRINTF_BUFSIZE, op);

  char condbitsstr[SNPRINTF_BUFSIZE];
  condbits_to_string(condbitsstr, SNPRINTF_BUFSIZE, op);

  const char *fmt = "%04x    %-15s %-12s; condbits: %-12s description: %-12s";
  size_t len = snprintf(NULL, 0, fmt, op->addr, mnemonicstr, "", condbitsstr,
                        op->def.desc);

  char *str = malloc(len + 1);
  if (!str)
    return NULL;

  snprintf(str, len + 1, fmt, op->addr, mnemonicstr, "", condbitsstr,
           op->def.desc);
  return str;
}

// LUT

#define CBITS_NONE ADC_8080_DASM_CONDBITS_NONE
#define CBITS_CY ADC_8080_DASM_CONDBITS_CY
#define CBITS_SZACP ADC_8080_DASM_CONDBITS_SZACP
#define CBITS_ALL ADC_8080_DASM_CONDBITS_ALL

// clang-format off
static const adc_8080_dasm_opdef s_dasm_lut[256] = {
    {0x00,    "nop",         1,    CBITS_NONE,    "no operation"},
    {0x01,    "lxi b,%04x",  3,    CBITS_NONE,    "b = byte 3, c = byte 2"},
    {0x02,    "stax b",      1,    CBITS_NONE,    "(bc) = a"},
    {0x03,    "inx b",       1,    CBITS_NONE,    "bc++"},
    {0x04,    "inr b",       1,    CBITS_SZACP,   "b++"},
    {0x05,    "dcr b",       1,    CBITS_SZACP,   "b--"},
    {0x06,    "mvi b,%02x",  2,    CBITS_NONE,    "b = byte 2"},
    {0x07,    "rlc",         1,    CBITS_CY,      "a <<= 1; bit 0 = prev bit 7; cy = prev bit 7"},
    {0x08,    "nop",         1,    CBITS_NONE,    "no operation"},
    {0x09,    "dad b",       1,    CBITS_CY,      "hl += bc"},
    {0x0A,    "ldax b",      1,    CBITS_NONE,    "a = (bc)"},
    {0x0B,    "dcx b",       1,    CBITS_NONE,    "bc--"},
    {0x0C,    "inr c",       1,    CBITS_SZACP,   "c++"},
    {0x0D,    "dcr c",       1,    CBITS_SZACP,   "c--"},
    {0x0E,    "mvi c,%02x",  2,    CBITS_NONE,    "c = byte 2"},
    {0x0F,    "rrc",         1,    CBITS_CY,      "a >>= 1; bit 7 = prev bit 0; cy = prev bit 0"},
    {0x10,    "nop",         1,    CBITS_NONE,    "no operation"},
    {0x11,    "lxi d,%04x",  3,    CBITS_NONE,    "d = byte 3, e = byte 2"},
    {0x12,    "stax d",      1,    CBITS_NONE,    "(de) = a"},
    {0x13,    "inx d",       1,    CBITS_NONE,    "de++"},
    {0x14,    "inr d",       1,    CBITS_SZACP,   "d++"},
    {0x15,    "dcr d",       1,    CBITS_SZACP,   "d--"},
    {0x16,    "mvi d,%02x",  2,    CBITS_NONE,    "d = byte 2"},
    {0x17,    "ral",         1,    CBITS_CY,      "a <<= 1; bit 0 = prev cy; cy = prev bit 7"},
    {0x18,    "nop",         1,    CBITS_NONE,    "no operation"},
    {0x19,    "dad d",       1,    CBITS_CY,      "hl += de"},
    {0x1A,    "ldax d",      1,    CBITS_NONE,    "a = (de)"},
    {0x1B,    "dcx d",       1,    CBITS_NONE,    "de--"},
    {0x1C,    "inr e",       1,    CBITS_SZACP,   "e++"},
    {0x1D,    "dcr e",       1,    CBITS_SZACP,   "e--"},
    {0x1E,    "mvi e,%02x",  2,    CBITS_NONE,    "e = byte 2"},
    {0x1F,    "rar",         1,    CBITS_CY,      "a >>= 1; bit 7 = prev cy; cy = prev bit 0"},
    {0x20,    "nop",         1,    CBITS_NONE,    "no operation"},
    {0x21,    "lxi h,%04x",  3,    CBITS_NONE,    "h = byte 3, l = byte 2"},
    {0x22,    "shld (%04x)", 3,    CBITS_NONE,    "(adr+1) = h, (adr) = l"},
    {0x23,    "inx h",       1,    CBITS_NONE,    "hl++"},
    {0x24,    "inr h",       1,    CBITS_SZACP,   "h++"},
    {0x25,    "dcr h",       1,    CBITS_SZACP,   "h--"},
    {0x26,    "mvi h,%02x",  2,    CBITS_NONE,    "h = byte 2"},
    {0x27,    "daa",         1,    CBITS_NONE,    "todo"},
    {0x28,    "nop",         1,    CBITS_NONE,    "no operation"},
    {0x29,    "dad h",       1,    CBITS_CY,      "hl += de"},
    {0x2A,    "lhld (%04x)", 3,    CBITS_NONE,    "h = (adr+1), l = (adr)"},
    {0x2B,    "dcx h",       1,    CBITS_NONE,    "hl--"},
    {0x2C,    "inr l",       1,    CBITS_SZACP,   "l++"},
    {0x2D,    "dcr l",       1,    CBITS_SZACP,   "l--"},
    {0x2E,    "mvi l,%02x",  2,    CBITS_NONE,    "l = byte 2"},
    {0x2F,    "cma",         1,    CBITS_NONE,    "a = !a"},
    {0x30,    "nop",         1,    CBITS_NONE,    "no operation"},
    {0x31,    "lxi sp,%04x", 3,    CBITS_NONE,    "s = byte 3, p = byte 2"},
    {0x32,    "sta (%04x)",  3,    CBITS_NONE,    "(adr) = a"},
    {0x33,    "inx sp",      1,    CBITS_NONE,    "sp++"},
    {0x34,    "inr m",       1,    CBITS_SZACP,   "(hl)++"},
    {0x35,    "dcr m",       1,    CBITS_SZACP,   "(hl)--"},
    {0x36,    "mvi m,%02x",  2,    CBITS_NONE,    "(hl) = byte 2"},
    {0x37,    "stc",         1,    CBITS_CY,      "cy = 1"},
    {0x38,    "nop",         1,    CBITS_NONE,    "no operation"},
    {0x39,    "dad so",      1,    CBITS_CY,      "hl += sp"},
    {0x3A,    "lda (%04x)",  3,    CBITS_NONE,    "a = (adr)"},
    {0x3B,    "dcx sp",      1,    CBITS_NONE,    "sp--"},
    {0x3C,    "inr a",       1,    CBITS_SZACP,   "a++"},
    {0x3D,    "dcr a",       1,    CBITS_SZACP,   "a--"},
    {0x3E,    "mvi a,%02x",  2,    CBITS_NONE,    "a = byte 2"},
    {0x3F,    "cmc",         1,    CBITS_NONE,    "cy = !cy"},
    {0x40,    "mov b,b",     1,    CBITS_NONE,    "b = b"},
    {0x41,    "mov b,c",     1,    CBITS_NONE,    "b = c"},
    {0x42,    "mov b,d",     1,    CBITS_NONE,    "b = d"},
    {0x43,    "mov b,e",     1,    CBITS_NONE,    "b = e"},
    {0x44,    "mov b,h",     1,    CBITS_NONE,    "b = h"},
    {0x45,    "mov b,l",     1,    CBITS_NONE,    "b = l"},
    {0x46,    "mov b,m",     1,    CBITS_NONE,    "b = (hl)"},
    {0x47,    "mov b,a",     1,    CBITS_NONE,    "b = a"},
    {0x48,    "mov c,b",     1,    CBITS_NONE,    "c = b"},
    {0x49,    "mov c,c",     1,    CBITS_NONE,    "c = c"},
    {0x4A,    "mov c,d",     1,    CBITS_NONE,    "c = d"},
    {0x4B,    "mov c,e",     1,    CBITS_NONE,    "c = e"},
    {0x4C,    "mov c,h",     1,    CBITS_NONE,    "c = h"},
    {0x4D,    "mov c,l",     1,    CBITS_NONE,    "c = l"},
    {0x4E,    "mov c,m",     1,    CBITS_NONE,    "c = (hl)"},
    {0x4F,    "mov c,a",     1,    CBITS_NONE,    "c = a"},
    {0x50,    "mov d,b",     1,    CBITS_NONE,    "b = b"},
    {0x51,    "mov d,c",     1,    CBITS_NONE,    "d = c"},
    {0x52,    "mov d,d",     1,    CBITS_NONE,    "d = d"},
    {0x53,    "mov d,e",     1,    CBITS_NONE,    "d = e"},
    {0x54,    "mov d,h",     1,    CBITS_NONE,    "d = h"},
    {0x55,    "mov d,l",     1,    CBITS_NONE,    "d = l"},
    {0x56,    "mov d,m",     1,    CBITS_NONE,    "d = (hl)"},
    {0x57,    "mov d,a",     1,    CBITS_NONE,    "d = a"},
    {0x58,    "mov e,b",     1,    CBITS_NONE,    "e = b"},
    {0x59,    "mov e,c",     1,    CBITS_NONE,    "e = c"},
    {0x5A,    "mov e,d",     1,    CBITS_NONE,    "e = d"},
    {0x5B,    "mov e,e",     1,    CBITS_NONE,    "e = e"},
    {0x5C,    "mov e,h",     1,    CBITS_NONE,    "e = h"},
    {0x5D,    "mov e,l",     1,    CBITS_NONE,    "e = l"},
    {0x5E,    "mov e,m",     1,    CBITS_NONE,    "e = (hl)"},
    {0x5F,    "mov e,a",     1,    CBITS_NONE,    "e = a"},
    {0x60,    "mov h,b",     1,    CBITS_NONE,    "h = b"},
    {0x61,    "mov h,c",     1,    CBITS_NONE,    "h = c"},
    {0x62,    "mov h,d",     1,    CBITS_NONE,    "h = d"},
    {0x63,    "mov h,e",     1,    CBITS_NONE,    "h = e"},
    {0x64,    "mov h,h",     1,    CBITS_NONE,    "h = h"},
    {0x65,    "mov h,l",     1,    CBITS_NONE,    "h = l"},
    {0x66,    "mov h,m",     1,    CBITS_NONE,    "h = (hl)"},
    {0x67,    "mov h,a",     1,    CBITS_NONE,    "h = a"},
    {0x68,    "mov l,b",     1,    CBITS_NONE,    "l = b"},
    {0x69,    "mov l,c",     1,    CBITS_NONE,    "l = c"},
    {0x6A,    "mov l,d",     1,    CBITS_NONE,    "l = d"},
    {0x6B,    "mov l,e",     1,    CBITS_NONE,    "l = e"},
    {0x6C,    "mov l,h",     1,    CBITS_NONE,    "l = h"},
    {0x6D,    "mov l,l",     1,    CBITS_NONE,    "l = l"},
    {0x6E,    "mov l,m",     1,    CBITS_NONE,    "l = (hl)"},
    {0x6F,    "mov l,a",     1,    CBITS_NONE,    "l = a"},
    {0x70,    "mov m,b",     1,    CBITS_NONE,    "(hl) = b"},
    {0x71,    "mov m,c",     1,    CBITS_NONE,    "(hl) = c"},
    {0x72,    "mov m,d",     1,    CBITS_NONE,    "(hl) = d"},
    {0x73,    "mov m,e",     1,    CBITS_NONE,    "(hl) = e"},
    {0x74,    "mov m,h",     1,    CBITS_NONE,    "(hl) = h"},
    {0x75,    "mov m,l",     1,    CBITS_NONE,    "(hl) = l"},
    {0x76,    "hlt",         1,    CBITS_NONE,    "halt cpu"},
    {0x77,    "mov m,a",     1,    CBITS_NONE,    "(hl) = a"},
    {0x78,    "mov a,b",     1,    CBITS_NONE,    "a = b"},
    {0x79,    "mov a,c",     1,    CBITS_NONE,    "a = c"},
    {0x7A,    "mov a,d",     1,    CBITS_NONE,    "a = d"},
    {0x7B,    "mov a,e",     1,    CBITS_NONE,    "a = e"},
    {0x7C,    "mov a,h",     1,    CBITS_NONE,    "a = h"},
    {0x7D,    "mov a,l",     1,    CBITS_NONE,    "a = l"},
    {0x7E,    "mov a,m",     1,    CBITS_NONE,    "a = (hl)"},
    {0x7F,    "mov a,a",     1,    CBITS_NONE,    "a = a"},
    {0x80,    "add b",       1,    CBITS_ALL,     "a += b"},
    {0x81,    "add c",       1,    CBITS_ALL,     "a += c"},
    {0x82,    "add d",       1,    CBITS_ALL,     "a += d"},
    {0x83,    "add e",       1,    CBITS_ALL,     "a += e"},
    {0x84,    "add h",       1,    CBITS_ALL,     "a += h"},
    {0x85,    "add l",       1,    CBITS_ALL,     "a += l"},
    {0x86,    "add m",       1,    CBITS_ALL,     "a += (hl)"},
    {0x87,    "add a",       1,    CBITS_ALL,     "a += a"},
    {0x88,    "adc b",       1,    CBITS_ALL,     "a += b + cy"},
    {0x89,    "adc c",       1,    CBITS_ALL,     "a += c + cy"},
    {0x8A,    "adc d",       1,    CBITS_ALL,     "a += d + cy"},
    {0x8B,    "adc e",       1,    CBITS_ALL,     "a += e + cy"},
    {0x8C,    "adc h",       1,    CBITS_ALL,     "a += h + cy"},
    {0x8D,    "adc l",       1,    CBITS_ALL,     "a += l + cy"},
    {0x8E,    "adc m",       1,    CBITS_ALL,     "a += (hl) + cy"},
    {0x8F,    "adc a",       1,    CBITS_ALL,     "a += a + cy"},
    {0x90,    "sub b",       1,    CBITS_ALL,     "a -= b"},
    {0x91,    "sub c",       1,    CBITS_ALL,     "a -= c"},
    {0x92,    "sub d",       1,    CBITS_ALL,     "a -= d"},
    {0x93,    "sub e",       1,    CBITS_ALL,     "a -= e"},
    {0x94,    "sub h",       1,    CBITS_ALL,     "a -= h"},
    {0x95,    "sub l",       1,    CBITS_ALL,     "a -= l"},
    {0x96,    "sub m",       1,    CBITS_ALL,     "a -= (hl)"},
    {0x97,    "sub a",       1,    CBITS_ALL,     "a -= a"},
    {0x98,    "sbb b",       1,    CBITS_ALL,     "a -= b - cy"},
    {0x99,    "sbb c",       1,    CBITS_ALL,     "a -= c - cy"},
    {0x9A,    "sbb d",       1,    CBITS_ALL,     "a -= d - cy"},
    {0x9B,    "sbb e",       1,    CBITS_ALL,     "a -= e - cy"},
    {0x9C,    "sbb h",       1,    CBITS_ALL,     "a -= h - cy"},
    {0x9D,    "sbb l",       1,    CBITS_ALL,     "a -= l - cy"},
    {0x9E,    "sbb m",       1,    CBITS_ALL,     "a -= (hl) - cy"},
    {0x9F,    "sbb a",       1,    CBITS_ALL,     "a -= a - cy"},
    {0xA0,    "ana b",       1,    CBITS_ALL,     "a &= b"},
    {0xA1,    "ana c",       1,    CBITS_ALL,     "a &= c"},
    {0xA2,    "ana d",       1,    CBITS_ALL,     "a &= d"},
    {0xA3,    "ana e",       1,    CBITS_ALL,     "a &= e"},
    {0xA4,    "ana h",       1,    CBITS_ALL,     "a &= h"},
    {0xA5,    "ana l",       1,    CBITS_ALL,     "a &= l"},
    {0xA6,    "ana m",       1,    CBITS_ALL,     "a &= (hl)"},
    {0xA7,    "ana a",       1,    CBITS_ALL,     "a &= a"},
    {0xA8,    "xra b",       1,    CBITS_ALL,     "a ^= b"},
    {0xA9,    "xra c",       1,    CBITS_ALL,     "a ^= c"},
    {0xAA,    "xra d",       1,    CBITS_ALL,     "a ^= d"},
    {0xAB,    "xra e",       1,    CBITS_ALL,     "a ^= e"},
    {0xAC,    "xra h",       1,    CBITS_ALL,     "a ^= h"},
    {0xAD,    "xra l",       1,    CBITS_ALL,     "a ^= l"},
    {0xAE,    "xra m",       1,    CBITS_ALL,     "a ^= (hl)"},
    {0xAF,    "xra a",       1,    CBITS_ALL,     "a ^= a"},
    {0xB0,    "ora b",       1,    CBITS_ALL,     "a |= b"},
    {0xB1,    "ora c",       1,    CBITS_ALL,     "a |= c"},
    {0xB2,    "ora d",       1,    CBITS_ALL,     "a |= d"},
    {0xB3,    "ora e",       1,    CBITS_ALL,     "a |= e"},
    {0xB4,    "ora h",       1,    CBITS_ALL,     "a |= h"},
    {0xB5,    "ora l",       1,    CBITS_ALL,     "a |= l"},
    {0xB6,    "ora m",       1,    CBITS_ALL,     "a |= (hl)"},
    {0xB7,    "ora a",       1,    CBITS_ALL,     "a |= a"},
    {0xB8,    "cmp b",       1,    CBITS_ALL,     "a - b"},
    {0xB9,    "cmp c",       1,    CBITS_ALL,     "a - c"},
    {0xBA,    "cmp d",       1,    CBITS_ALL,     "a - d"},
    {0xBB,    "cmp e",       1,    CBITS_ALL,     "a - e"},
    {0xBC,    "cmp h",       1,    CBITS_ALL,     "a - h"},
    {0xBD,    "cmp l",       1,    CBITS_ALL,     "a - l"},
    {0xBE,    "cmp m",       1,    CBITS_ALL,     "a - (hl)"},
    {0xBF,    "cmp a",       1,    CBITS_ALL,     "a - a"},
    {0xC0,    "rnz",         1,    CBITS_NONE,    "if nz, ret"},
    {0xC1,    "pop b",       1,    CBITS_NONE,    "b = (sp+1); c = (sp); sp += 2"},
    {0xC2,    "jnz %04x",    3,    CBITS_NONE,    "if nz, pc = adr"},
    {0xC3,    "jmp %04x",    3,    CBITS_NONE,    "pc = adr"},
    {0xC4,    "cnz %04x",    3,    CBITS_NONE,    "if nz, call adr"},
    {0xC5,    "push b",      1,    CBITS_NONE,    "(sp-1) = b; (sp-2) = c; sp -= 2"},
    {0xC6,    "adi %02x",    2,    CBITS_ALL,     "a += byte"},
    {0xC7,    "rst 0",       1,    CBITS_NONE,    "call 0000"},
    {0xC8,    "rz",          1,    CBITS_NONE,    "if z, ret"},
    {0xC9,    "ret",         1,    CBITS_NONE,    "pc.lo = (sp); pc.hi = (sp+1); sp += 2"},
    {0xCA,    "jz %04x",     3,    CBITS_NONE,    "if z, pc = adr"},
    {0xCB,    "*jmp %04x",   3,    CBITS_NONE,    "pc = adr"},
    {0xCC,    "cz %04x",     3,    CBITS_NONE,    "if z, call adr"},
    {0xCD,    "call %04x",   3,    CBITS_NONE,    "(sp-1) = pc.hi; (sp-2) = pc.lo; sp -= 2; pc = adr"},
    {0xCE,    "aci %02x",    2,    CBITS_ALL,     "a += byte + cy"},
    {0xCF,    "rst 1",       1,    CBITS_NONE,    "call 0008"},
    {0xD0,    "rnc",         1,    CBITS_NONE,    "if ncy, ret"},
    {0xD1,    "pop d",       1,    CBITS_NONE,    "d = (sp+1); e = (sp); sp += 2"},
    {0xD2,    "jnc %04x",    3,    CBITS_NONE,    "if ncy, pc = adr"},
    {0xD3,    "out %02x",    2,    CBITS_NONE,    "device port byte = a"},
    {0xD4,    "cnc %04x",    3,    CBITS_NONE,    "if ncy, call adr"},
    {0xD5,    "push d",      1,    CBITS_NONE,    "(sp-1) = d; (sp-2) = e; sp -= 2"},
    {0xD6,    "sui %02x",    2,    CBITS_ALL,     "a -= byte"},
    {0xD7,    "rst 2",       1,    CBITS_NONE,    "call 0010"},
    {0xD8,    "rc",          1,    CBITS_NONE,    "if cy, ret"},
    {0xD9,    "*ret",        1,    CBITS_NONE,    "pc.lo = (sp); pc.hi = (sp+1); sp += 2"},
    {0xDA,    "jc %04x",     3,    CBITS_NONE,    "if cy, pc = adr"},
    {0xDB,    "in %02x",     2,    CBITS_NONE,    "a = device port byte"},
    {0xDC,    "cc %04x",     3,    CBITS_NONE,    "if cy, call adr"},
    {0xDD,    "*call %04x",  3,    CBITS_NONE,    "(sp-1) = pc.hi; (sp-2) = pc.lo; sp -= 2; pc = adr"},
    {0xDE,    "sbi %02x",    2,    CBITS_ALL,     "a -= byte - cy"},
    {0xDF,    "rst 3",       1,    CBITS_NONE,    "call 0018"},
    {0xE0,    "rpo",         1,    CBITS_NONE,    "if po, ret"},
    {0xE1,    "pop h",       1,    CBITS_NONE,    "h = (sp+1); l = (sp); sp += 2"},
    {0xE2,    "jpo %04x",    3,    CBITS_NONE,    "if po, pc = adr"},
    {0xE3,    "xhtl",        1,    CBITS_NONE,    "swap l,(sp); swap h,(sp+1)"},
    {0xE4,    "cpo %04x",    3,    CBITS_NONE,    "if po, call adr"},
    {0xE5,    "push h",      1,    CBITS_NONE,    "(sp-1) = h; (sp-2) = l; sp -= 2"},
    {0xE6,    "ani %02x",    2,    CBITS_ALL,     "a &= byte"},
    {0xE7,    "rst 4",       1,    CBITS_NONE,    "call 0020"},
    {0xE8,    "rpe",         1,    CBITS_NONE,    "if pe, ret"},
    {0xE9,    "pchl",        1,    CBITS_NONE,    "pc.hi = h; pc.lo = l"},
    {0xEA,    "jpe %04x",    3,    CBITS_NONE,    "if pe, pc = adr"},
    {0xEB,    "xhtl",        1,    CBITS_NONE,    "swap h,d; swap l,e"},
    {0xEC,    "cpe %04x",    3,    CBITS_NONE,    "if pe, call adr"},
    {0xED,    "*call %04x",  3,    CBITS_NONE,    "(sp-1) = pc.hi; (sp-2) = pc.lo; sp -= 2; pc = adr"},
    {0xEE,    "xri %02x",    2,    CBITS_ALL,     "a ^= byte"},
    {0xEF,    "rst 5",       1,    CBITS_NONE,    "call 0028"},
    {0xF0,    "rp",          1,    CBITS_NONE,    "if p, ret"},
    {0xF1,    "pop psw",     1,    CBITS_NONE,    "condbits = (sp); a = (sp+1); sp += 2"},
    {0xF2,    "jp %04x",     3,    CBITS_NONE,    "if p, pc = adr"},
    {0xF3,    "di",          1,    CBITS_NONE,    "disable interrupt flip-flop"},
    {0xF4,    "cp %04x",     3,    CBITS_NONE,    "if p, call adr"},
    {0xF5,    "push psw",    1,    CBITS_NONE,    "(sp-2) = condbits; (sp-1) = a; sp -= 2"},
    {0xF6,    "ori %02x",    2,    CBITS_ALL,     "a |= byte"},
    {0xF7,    "rst 6",       1,    CBITS_NONE,    "call 0030"},
    {0xF8,    "rm",          1,    CBITS_NONE,    "if m, ret"},
    {0xF9,    "sphl",        1,    CBITS_NONE,    "sp = hl"},
    {0xFA,    "jm %04x",     3,    CBITS_NONE,    "if m, pc = adr"},
    {0xFB,    "ei",          1,    CBITS_NONE,    "enable interrupt flip-flop"},
    {0xFC,    "cm %04x",     3,    CBITS_NONE,    "if m, call adr"},
    {0xFD,    "*call %04x",  3,    CBITS_NONE,    "(sp-1) = pc.hi; (sp-2) = pc.lo; sp -= 2; pc = adr"},
    {0xFE,    "cpi %02x",    2,    CBITS_ALL,     "a - byte"},
    {0xFF,    "rst 7",       1,    CBITS_NONE,    "call 0038"},
};
// clang-format on
