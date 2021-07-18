#include "adc_argp.h"

// TODO: Write real unit tests!

int main(int argc, const char *argv[]) {
  int fullscreen = 0;
  char *configdir = "./config";
  unsigned int winscale = 1;

  adc_argp_option options[] = {
      ADC_ARGP_HELP(),
      ADC_ARGP_OPTION("fullscreen", "f", ADC_ARGP_TYPE_FLAG, &fullscreen,
                      "Enable fullscreen mode"),
      ADC_ARGP_OPTION("configdir", "d", ADC_ARGP_TYPE_STRING, &configdir,
                      "Set the config directory"),
      ADC_ARGP_OPTION("winscale", "s", ADC_ARGP_TYPE_UINT, &winscale,
                      "Set the window scale factor"),
  };
  // const char *argv[] = {"adc_argp_test", "--flag", "--str",
  // "helloworld",
  // "--uint",        "abc",    "--float"};

  adc_argp_parser *parser =
      adc_argp_new_parser(options, ADC_ARGP_COUNT(options));
  if (!parser)
    return -1;

  if (adc_argp_parse(parser, argc, argv) > 0)
    adc_argp_print_errors(parser, stderr);

  printf("fullscreen: %d\n", fullscreen);
  printf("configdir: %s\n", configdir);
  printf("winscale: %d\n", winscale);

  adc_argp_destroy_parser(&parser);
  return 0;
}
