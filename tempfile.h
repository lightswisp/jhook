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
#include <stdbool.h>
#include "hooks.h"

extern bool  tempfile_create(void);
extern void  tempfile_remove(void);
extern char* tempfile_get_path(void);
extern char* tempfile_get_class_name(void);
extern bool  tempfile_generate_java_code(jvmtiEnv *jvmti, hook_t *hooks, size_t size);
