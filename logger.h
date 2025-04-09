#pragma once
#define LOG_MAX_BUFFER_SIZE 1024
#define LOG_MAX_TIME_BUFFER_SIZE 128

extern void logger_log  (const char *func, const char *fmt, ...);
extern void logger_warn (const char *func, const char *fmt, ...);
extern void logger_fatal(const char *func, const char *fmt, ...);

