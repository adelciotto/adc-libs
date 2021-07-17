cc := $(CC)
target_exec := vector_test
build_dir := ./build
srcs :=  vector_test.c

# String substitution for every C file to object file.
# For example, main.c -> ./build/main.c.o
objs := $(srcs:%=$(build_dir)/%.o)

# String substitution for every object file to dependency file.
# For example, ./build/main.c.o -> ./build.main.c.d
deps := $(objs:.o=.d)

# Compiler flags.
cflags := $(inc_flags) -MMD -MP -std=c99 -Wall -Wextra -pedantic -g -DDEBUG

default: $(build_dir)/$(target_exec)

# The final build step
$(build_dir)/$(target_exec): $(objs)
	$(cc) $(objs) -o $@

# Build step for C sources.
$(build_dir)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(cc) $(cflags) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(build_dir)

# Include the .d makefiles.
-include $(deps)
