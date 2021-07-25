#define ADC_LOG_IMPLEMENTATION
#include "adc_log.h"

static void example_logs() {
  adc_log_debug("An example debug log. %s", "Yay!");
  adc_log_info("An example info log");
  adc_log_warn("An example warn log. %d", 1000);
  adc_log_error("An example error log");
  fprintf(stderr, "\n");
}

static void example_cb(adc_log_msg *msg) {
  vprintf(msg->fmt, msg->args);
  printf("\n");
}

int main() {
  example_logs();

  adc_log_set_level(ADC_LOG_INFO);
  example_logs();

  adc_log_set_level(ADC_LOG_WARN);
  example_logs();

  adc_log_set_level(ADC_LOG_ERROR);
  example_logs();

  adc_log_add_callback(example_cb, NULL, ADC_LOG_INFO);
  adc_log_info("An example info log via callback");
  printf("\n");

  FILE *fp = fopen("log_test.txt", "a");
  if (fp) {
    adc_log_add_fp(fp, ADC_LOG_DEBUG);
    example_logs();
  }

  return 0;
}
