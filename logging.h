#pragma once
#include <Arduino.h>

#ifndef LOG_TAG
#define LOG_TAG "AQ"
#endif

#define LOGI(fmt, ...) do { Serial.printf("[INFO][" LOG_TAG "] " fmt "\n", ##__VA_ARGS__); } while (0)
#define LOGE(fmt, ...) do { Serial.printf("[ERR ][" LOG_TAG "] " fmt "\n",  ##__VA_ARGS__); } while (0)