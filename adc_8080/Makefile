cc := $(CC)

build_dir := ./build

cpu_test_target := 8080_cpu_test
dasm_test_target := 8080_dasm_test

cpu_test_srcs :=  adc_8080_cpu.c 8080_cpu_test.c
dasm_test_srcs :=  adc_8080_dasm.c 8080_dasm_test.c

# String substitution for every C file to object file.
# For example, main.c -> ./build/main.c.o
cpu_test_objs := $(cpu_test_srcs:%=$(build_dir)/%.o)
dasm_test_objs := $(dasm_test_srcs:%=$(build_dir)/%.o)

# Compiler flags.
cflags := $(inc_flags) -MMD -MP -std=c99 -Wall -Wextra -pedantic -g -DDEBUG

all: cpu_test dasm_test
cpu_test: $(build_dir)/$(cpu_test_target)
dasm_test: $(build_dir)/$(dasm_test_target)

# The final build step
$(build_dir)/$(cpu_test_target): $(cpu_test_objs)
	$(cc) $(cpu_test_objs) -o $@

$(build_dir)/$(dasm_test_target): $(dasm_test_objs)
	$(cc) $(dasm_test_objs) -o $@

# Build step for C sources.
$(build_dir)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(cc) $(cflags) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(build_dir)
