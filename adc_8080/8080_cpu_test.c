#include "adc_8080_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMORY_TOTAL 0x10000

static void run_test(adc_8080_cpu *cpu, const char *filename,
                     uint64_t expected_cycles);

static bool s_test_complete;
static uint8_t *s_memory;

// Credit to superzazu for their 8080 cpu test setup which was used as a
// reference. https://github.com/superzazu/8080/blob/master/i8080_tests.c
// Test roms from: https://altairclone.com/downloads/cpu_tests/.
// BDOS system call reference: https://www.seasip.info/Cpm/bdos.html
int main() {
        printf("########## 8080 CPU test started!\n");

        s_memory = malloc(MEMORY_TOTAL);
        if (!s_memory) {
                fprintf(stderr, "Failed to malloc() memory!");
                return EXIT_FAILURE;
        }

        adc_8080_cpu cpu;
        run_test(&cpu, "roms/TST8080.COM", 4924LU);
        run_test(&cpu, "roms/CPUTEST.COM", 255653383LU);
        run_test(&cpu, "roms/8080PRE.COM", 7817LU);
        run_test(&cpu, "roms/8080EXM.COM", 23803381171LU);

        printf("\n########## 8080 CPU test finished!\n");

        free(s_memory);
        return EXIT_SUCCESS;
}

static uint8_t handle_memory_read(void *userdata, uint16_t addr);
static void handle_memory_write(void *userdata, uint16_t addr, uint8_t value);
static uint8_t handle_device_read(void *userdata, uint8_t device);
static void handle_device_write(void *userdata, uint8_t device, uint8_t output);

static void run_test(adc_8080_cpu *cpu, const char *filename,
                     uint64_t expected_cycles) {
        printf("\n##### Starting test '%s'\n\n", filename);
        s_test_complete = false;

        // Init the cpu.
        adc_8080_cpu_init(cpu);
        cpu->userdata = cpu;
        cpu->read_byte = handle_memory_read;
        cpu->write_byte = handle_memory_write;
        cpu->read_device = handle_device_read;
        cpu->write_device = handle_device_write;
        // Rom instructions start at address 0x100 (ORG 00100H).
        cpu->pc = 0x100;

        // Clear all the memory.
        memset(s_memory, 0, MEMORY_TOTAL);
        // Inject 'OUT 0,A' at 0x0000 to signal the test is complete.
        // BDOS 'function 0 P_TERMCPM' system call.
        s_memory[0x0000] = 0xD3;
        s_memory[0x0001] = 0x00;
        // Inject 'OUT 1,A' at 0x0005 to signal character output.
        // BDOS 'function 2 C_WRITE' and 'function 9 C_WRITESTR' system calls.
        s_memory[0x0005] = 0xD3;
        s_memory[0x0006] = 0x01;
        s_memory[0x0007] = 0xC9; // RET

        // Open the rom file and read into memory.
        FILE *file = fopen(filename, "rb");
        if (!file) {
                fprintf(stderr,
                        "\n\n##### Test '%s' failed!\n"
                        "Error: Failed to fopen() the rom file!\n",
                        filename);
                return;
        }
        fseek(file, 0, SEEK_END);
        size_t size = ftell(file);
        rewind(file);

        size_t bytes_read = fread(s_memory + 0x100, 1, size, file);
        if (bytes_read != size) {
                fprintf(
                    stderr,
                    "\n\n##### Test '%s' failed!\n"
                    "Error: Failed to read the rom file into memory! Read %zu "
                    "bytes, total is %zu bytes\n",
                    filename, bytes_read, size);
                return;
        }

        // Run the test.
        while (!s_test_complete)
                adc_8080_cpu_step(cpu);

        int64_t diff = llabs(expected_cycles - cpu->cycle_count);
        if (diff > 0) {
                fprintf(
                    stderr,
                    "\n\n##### Test '%s' failed!\n"
                    "Error: Cycles consumed does not match expected! Expected: "
                    "%lld, actual: "
                    "%lld\n",
                    filename, expected_cycles, cpu->cycle_count);
                return;
        }

        printf("\n\n##### Test '%s' passed!\n", filename);
}

static uint8_t handle_memory_read(void *userdata, uint16_t addr) {
        return s_memory[addr];
}

static void handle_memory_write(void *userdata, uint16_t addr, uint8_t value) {
        s_memory[addr] = value;
}

static uint8_t handle_device_read(void *userdata, uint8_t device) {
        return 0;
}

static void handle_device_write(void *userdata, uint8_t device,
                                uint8_t output) {
        adc_8080_cpu *cpu = (adc_8080_cpu *)userdata;

        if (device == 0) {
                s_test_complete = true;
                return;
        }

        if (device == 1) {
                uint8_t operation = cpu->rc;
                if (operation == 2) {
                        printf("%c", cpu->re);
                } else if (operation == 9) {
                        // Print chars starting from address DE until
                        // terminating '$' char.
                        uint16_t addr = (cpu->rd << 8) | cpu->re;
                        while (cpu->read_byte(userdata, addr) != '$') {
                                printf("%c", cpu->read_byte(userdata, addr++));
                        }
                }
                fflush(stdout);
        }
}
