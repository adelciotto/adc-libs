cc := $(CC)
target := log_test
build_dir := build

cflags := -MMD -MP -std=c99 -Wall -Wextra -pedantic -g -DDEBUG

srcs := log_test.c

objs := $(srcs:%=$(build_dir)/%.o)
deps := $(objs:.o=.d)

all: $(build_dir)/$(target)

$(build_dir)/$(target): $(objs)
	$(cc) $(objs) -o $@

$(build_dir)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(cc) $(cflags) -c $< -o $@

-include $(deps)

.PHONY: clean
clean:
	rm -rf $(build_dir)
