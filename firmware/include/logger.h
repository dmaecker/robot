#pragma once
#include <Arduino.h>

void logger_begin(uint16_t port = 4444);
void logger_log(const char* fmt, ...);

#define LOG(...) logger_log(__VA_ARGS__)