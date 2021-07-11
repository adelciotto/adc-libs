// clang-format off
// adc_argp command-line argument parsing library by Anthony Del Ciotto.
//
// MIT License

// Copyright (c) [2021] [Anthony Del Ciotto]

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// clang-format on

#include "adc_argp.h"

#include <errno.h>  // For errno global
#include <stdlib.h> // For malloc, free
#include <string.h> // For strcmp

// For arithmetic range and underflow checks
#include <float.h>
#include <limits.h>
#include <math.h>

// Allow override of assert.
#ifndef ADC_ARGP_ASSERT
#include <assert.h>
#define ADC_ARGP_ASSERT(x) assert(x)
#endif

// Allow override of malloc and free.
#ifndef ADC_ARGP_MALLOC
#define ADC_ARGP_MALLOC(sz) malloc(sz)
#endif
#ifndef ADC_ARGP_FREE
#define ADC_ARGP_FREE(p) free(p)
#endif

// Allow override of max errors.
#ifndef ADC_ARGP_MAX_ERRORS
#define ADC_ARGP_MAX_ERRORS 32
#endif

typedef enum {
        ERROR_NONE = -1,
        ERROR_OPT_UNKNOWN,
        ERROR_ARG_MISSING,
        ERROR_ARG_INVALID,
        ERROR_ARG_INVALID_BOOL,
        ERROR_ARG_NEGATIVE_UINT,
        ERROR_ARG_OUT_OF_RANGE,
        ERROR_ARG_UNDERFLOW
} error_type;

typedef struct {
        error_type type;
        const adc_argp_option *opt;
        const char *argv;
} error;

struct adc_argp_parser {
        adc_argp_option *opts;
        unsigned int opts_len;
        // Statically allocate a fixed array of errors (32 by default). It's
        // unlikely that the parser will encounter more than this. The max can
        // always be overridden by defining ADC_ARGP_MAX_ERRORS.
        error errors[ADC_ARGP_MAX_ERRORS];
        unsigned int errors_len;
};

static int find_opt(const char *argv, adc_argp_option *opts, int len);
static int parse_bool(int *out, const char *argv);
static int parse_int(int *out, const char *argv);
static int parse_uint(unsigned int *out, const char *argv);
static int parse_float(float *out, const char *argv);
static int parse_double(double *out, const char *argv);

static void add_error(adc_argp_parser *parser, error_type type,
                      const adc_argp_option *opt, const char *argv);

adc_argp_parser *adc_argp_new_parser(adc_argp_option *opts,
                                     unsigned int opts_len) {
        ADC_ARGP_ASSERT(opts);

        adc_argp_parser *parser = ADC_ARGP_MALLOC(sizeof(*parser));
        if (!parser)
                return NULL;

        parser->opts = opts;
        parser->opts_len = opts_len;
        parser->errors_len = 0;
        return parser;
}

void adc_argp_destroy_parser(adc_argp_parser **parser) {
        if (*parser) {
                ADC_ARGP_FREE(*parser);
        }
        *parser = NULL;
}

#define ARGV_NEXT()                                                            \
        if (++i >= argc) {                                                     \
                add_error(parser, ERROR_ARG_MISSING, opt, argv[i - 1]);        \
                break;                                                         \
        }

int adc_argp_parse(adc_argp_parser *parser, int argc, const char *argv[]) {
        ADC_ARGP_ASSERT(parser);
        ADC_ARGP_ASSERT(argv);
        ADC_ARGP_ASSERT(parser->opts);

        // Do nothing if the user supplies an empty options table.
        if (parser->opts_len == 0)
                return 0;

        for (int i = 1; i < argc; i++) {
                int opt_index =
                    find_opt(argv[i], parser->opts, parser->opts_len);
                if (opt_index < 0) {
                        add_error(parser, ERROR_OPT_UNKNOWN, NULL, argv[i]);
                        continue;
                }

                adc_argp_option *opt = &parser->opts[opt_index];
                error_type err = ERROR_NONE;

                switch (opt->valtype) {
                case ADC_ARGP_TYPE_FLAG: *((int *)(opt->val)) = 1; break;
                case ADC_ARGP_TYPE_BOOL:
                        ARGV_NEXT();
                        err = parse_bool((int *)opt->val, argv[i]);
                        break;
                case ADC_ARGP_TYPE_STRING:
                        ARGV_NEXT();
                        // Dereference the pointer to the string and set it to
                        // point to the argv string.
                        *((char **)(opt->val)) = (char *)argv[i];
                        break;
                case ADC_ARGP_TYPE_INT:
                        ARGV_NEXT();
                        err = parse_int((int *)opt->val, argv[i]);
                        break;
                case ADC_ARGP_TYPE_UINT:
                        ARGV_NEXT();
                        err = parse_uint((unsigned int *)opt->val, argv[i]);
                        break;
                case ADC_ARGP_TYPE_FLOAT:
                        ARGV_NEXT();
                        err = parse_float((float *)opt->val, argv[i]);
                        break;
                case ADC_ARGP_TYPE_DOUBLE:
                        ARGV_NEXT();
                        err = parse_double((double *)opt->val, argv[i]);
                        break;
                }

                if (err != ERROR_NONE) {
                        add_error(parser, err, opt, argv[i]);
                }
        }

        return parser->errors_len;
}

static const char *argp_type_str(adc_argp_type type) {
        switch (type) {
        case ADC_ARGP_TYPE_FLAG: return "flag";
        case ADC_ARGP_TYPE_BOOL: return "bool";
        case ADC_ARGP_TYPE_STRING: return "string";
        case ADC_ARGP_TYPE_INT: return "int";
        case ADC_ARGP_TYPE_UINT: return "uint";
        case ADC_ARGP_TYPE_FLOAT: return "float";
        case ADC_ARGP_TYPE_DOUBLE: return "double";
        default: return "";
        }
}

void adc_argp_print_errors(adc_argp_parser *parser, FILE *stream) {
        ADC_ARGP_ASSERT(parser);
        ADC_ARGP_ASSERT(stream);

        fprintf(stream, "adc_argp_parse errors:\n");

        for (int i = 0; i < parser->errors_len; i++) {
                const error *err = &parser->errors[i];

                switch (err->type) {
                case ERROR_OPT_UNKNOWN:
                        ADC_ARGP_ASSERT(err->argv);
                        fprintf(stream, "Unknown option: '%s'\n", err->argv);
                        break;
                case ERROR_ARG_MISSING:
                        ADC_ARGP_ASSERT(err->opt);
                        fprintf(stream,
                                "Argument expected for the --%s option\n",
                                err->opt->name);
                        break;
                case ERROR_ARG_INVALID:
                        ADC_ARGP_ASSERT(err->opt);
                        ADC_ARGP_ASSERT(err->argv);
                        fprintf(
                            stream,
                            "Invalid %s with value '%s' for the --%s option\n",
                            argp_type_str(err->opt->valtype), err->argv,
                            err->opt->name);
                        break;
                case ERROR_ARG_INVALID_BOOL:
                        ADC_ARGP_ASSERT(err->opt);
                        ADC_ARGP_ASSERT(err->argv);
                        fprintf(
                            stream,
                            "Invalid bool with value '%s' for the --%s option, "
                            "expected 'true', 'false', '1' or '0'\n",
                            err->argv, err->opt->name);
                        break;
                case ERROR_ARG_NEGATIVE_UINT:
                        ADC_ARGP_ASSERT(err->opt);
                        ADC_ARGP_ASSERT(err->argv);
                        fprintf(stream,
                                "Negative uint with value '%s' for the --%s "
                                "option\n",
                                err->argv, err->opt->name);
                        break;
                case ERROR_ARG_OUT_OF_RANGE:
                        ADC_ARGP_ASSERT(err->opt);
                        ADC_ARGP_ASSERT(err->argv);
                        fprintf(stream,
                                "Out of range %s with value '%s' for the --%s "
                                "option\n",
                                argp_type_str(err->opt->valtype), err->argv,
                                err->opt->name);
                        break;
                case ERROR_ARG_UNDERFLOW:
                        ADC_ARGP_ASSERT(err->opt);
                        ADC_ARGP_ASSERT(err->argv);
                        fprintf(stream,
                                "Underflow has occurred in %s with value '%s' "
                                "for the --%s option\n",
                                argp_type_str(err->opt->valtype), err->argv,
                                err->opt->name);
                        break;
                }
        }
}

#define STRING_EQL(a, b) (strcmp(a, b) == 0)

static int string_has_prefix(const char *str, const char *prefix) {
        ADC_ARGP_ASSERT(str);
        ADC_ARGP_ASSERT(prefix);

        int i = 0;
        const char *strp = str;
        const char *prefixp = prefix;
        while (*prefixp != '\0') {
                // Return false if we reach the end of the string before the end
                // of the prefix.
                if (*strp == '\0')
                        return 0;
                // Return false when encountering the first non-match.
                if (*prefixp != *strp)
                        return 0;
                strp++;
                prefixp++;
        }
        return 1;
}

static int find_opt(const char *argv, adc_argp_option *opts, int opts_len) {
        ADC_ARGP_ASSERT(opts);
        ADC_ARGP_ASSERT(argv);

        // Options must begin with either '--' (long names) or '-' (short
        // names).
        int longname;
        if (string_has_prefix(argv, "--"))
                longname = 1;
        else if (string_has_prefix(argv, "-"))
                longname = 0;
        else
                return -1;

        for (int i = 0; i < opts_len; i++) {
                // +2 to skip '--'
                if (longname && STRING_EQL(argv + 2, opts[i].name))
                        return i;
                // +1 to skip '-'
                else if (STRING_EQL(argv + 1, opts[i].shortname))
                        return i;
        }
        return -1;
}

static int parse_bool(int *out, const char *argv) {
        ADC_ARGP_ASSERT(argv);

        // Accept 'true' and 'false' strings are valid bool args.
        if (STRING_EQL(argv, "true")) {
                *out = 1;
                return ERROR_NONE;
        }
        if (STRING_EQL(argv, "false")) {
                *out = 0;
                return ERROR_NONE;
        }

        int val = parse_int(out, argv);
        if (!(val == 1 || val == 0))
                return ERROR_ARG_INVALID_BOOL;

        *out = val;
        return ERROR_NONE;
}

static int parse_int(int *out, const char *argv) {
        ADC_ARGP_ASSERT(argv);

        char *endptr;
        long result = strtol(argv, &endptr, 0);

        if (endptr == argv)
                return ERROR_ARG_INVALID;
        if (result >= INT_MAX || result <= -INT_MAX)
                return ERROR_ARG_OUT_OF_RANGE;

        *out = (int)result;
        return ERROR_NONE;
}

static int parse_uint(unsigned int *out, const char *argv) {
        ADC_ARGP_ASSERT(argv);

        // Reject negative args for uint options.
        if (strchr(argv, '-'))
                return ERROR_ARG_NEGATIVE_UINT;

        char *endptr;
        unsigned long result = strtoul(argv, &endptr, 0);

        if (endptr == argv)
                return ERROR_ARG_INVALID;
        if (result >= UINT_MAX)
                return ERROR_ARG_OUT_OF_RANGE;

        *out = (unsigned int)result;
        return ERROR_NONE;
}

static int parse_float(float *out, const char *argv) {
        ADC_ARGP_ASSERT(argv);

        char *endptr;
        errno = 0;
        float result = strtof(argv, &endptr);

        if (endptr == argv)
                return ERROR_ARG_INVALID;
        if ((result == HUGE_VALF || result == -HUGE_VALF) && errno == ERANGE)
                return ERROR_ARG_OUT_OF_RANGE;
        if ((result <= FLT_MIN) && errno == ERANGE)
                return ERROR_ARG_UNDERFLOW;

        *out = result;
        return ERROR_NONE;
}

static int parse_double(double *out, const char *argv) {
        ADC_ARGP_ASSERT(argv);

        char *endptr;
        errno = 0;
        float result = strtof(argv, &endptr);

        if (endptr == argv)
                return ERROR_ARG_INVALID;
        if ((result == HUGE_VAL || result == -HUGE_VAL) && errno == ERANGE)
                return ERROR_ARG_OUT_OF_RANGE;
        if ((result <= DBL_MIN) && errno == ERANGE)
                return ERROR_ARG_UNDERFLOW;

        *out = result;
        return ERROR_NONE;
}

static void add_error(adc_argp_parser *parser, error_type type,
                      const adc_argp_option *opt, const char *argv) {
        ADC_ARGP_ASSERT(parser);

        if (parser->errors_len >= ADC_ARGP_MAX_ERRORS)
                return;

        parser->errors[parser->errors_len].type = type;
        parser->errors[parser->errors_len].opt = opt;
        parser->errors[parser->errors_len].argv = argv;
        parser->errors_len++;
}

