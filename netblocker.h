#pragma once

struct times {
  int start;
  int stop;
};

int wait_for_wakeup();
struct times parse_config();
