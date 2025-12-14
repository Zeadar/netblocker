#pragma once

#define MONDAY    0b1000000
#define TUESDAY   0b0100000
#define WEDNESDAY 0b0010000
#define THURSDAY  0b0001000
#define FRIDAY    0b0000100
#define SATURDAY  0b0000010
#define SUNDAY    0b0000001

struct times {
  int start;
  int stop;
  int days;
};

int wait_for_wakeup();
struct times parse_config();
