#pragma once
#include <Arduino.h>

inline void _logf(const char* level, const char* fmt, ...) {
  char buf[160];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  Serial.printf("[%s] %s\n", level, buf);
}

#define LOGI(...) _logf("INFO", __VA_ARGS__)
#define LOGW(...) _logf("WARN", __VA_ARGS__)
#define LOGE(...) _logf("ERR ", __VA_ARGS__)
