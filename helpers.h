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
#include <stdint.h>
#include <jni.h>
#include <jvmti.h>

#include "defines.h"

extern void* safe_malloc(const char *f, size_t n);
extern void* safe_realloc(const char *f, void *p, size_t n);

#define SAFE_MALLOC(b, n, r) do { \
  b = safe_malloc(__func__, n);      \
  if(b == r) return r;               \
}while(0)

#define SAFE_REALLOC(b, p, n, r) do { \
  b = safe_realloc(__func__, p, n);   \
  if(b == r) return r;                \
}while(0)

extern jint         attach_current_thread(JavaVM *vm, void **penv, void *args);
extern jint         get_java_vms(JavaVM **vm);
extern jclass       find_class(JNIEnv *env, const char *class_name);
extern jmethodID    find_method(JNIEnv *env, const char *class_name, const char* method_name, const char *method_signature, bool is_static);
extern __Method     resolve_jmethod_id(jmethodID mid);

