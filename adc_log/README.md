adc_log
========================

My simple C99 logging library.

# Usage

```c
#define ADC_LOG_IMPLEMENTATION
#include "adc_log.h"

int main() {
  adc_log_debug("An example debug log. %s", "Yay!");
  adc_log_info("An example info log");
  adc_log_warn("An example warn log. %d", 1000);
  adc_log_error("An example error log");

  return 0;
}
```
