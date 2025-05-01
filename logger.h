/* +----------------------------------------------------------------------+ 
 * |  Copyright (C) 2025 lightswisp                                       |
 * |                                                                      |
 * |  Everyone is permitted to copy and distribute verbatim or modified   |
 * |  copies of this license document, and changing it is allowed as long |
 * |  as the name is changed.                                             |
 * |                                                                      |
 * |         DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE                  |
 * |  TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION     |
 * |                                                                      |
 * |  0. You just DO WHAT THE FUCK YOU WANT TO.                           |
 * +----------------------------------------------------------------------+
 */
#pragma once
#define LOG_MAX_BUFFER_SIZE 1024
#define LOG_MAX_TIME_BUFFER_SIZE 128
#define COLOR_START(x) "\e["x"m"
#define COLOR_END      "\e[0m"
#define RED            "31"
#define GREEN          "32" 
#define YELLOW         "33"

extern void logger_log  (const char *func, const char *fmt, ...);
extern void logger_warn (const char *func, const char *fmt, ...);
extern void logger_fatal(const char *func, const char *fmt, ...);

