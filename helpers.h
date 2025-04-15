#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <jni.h>
#include <jvmti.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "logger.h"

#define SAFE_MALLOC(b, n, r) do { \
  b = safe_malloc(__func__, n);      \
  if(b == r) return r;               \
}while(0)

#define SAFE_REALLOC(b, p, n, r) do { \
  b = safe_realloc(__func__, p, n);   \
  if(b == r) return r;                \
}while(0)

inline void* safe_malloc(const char *f, size_t n){
  void* b = malloc(n);
  if(b == NULL){
    logger_fatal(f, "malloc failed");
    return NULL;
  }
  memset(b, 0, n);
  return b;
}

inline void *safe_realloc(const char *f, void *p, size_t n){
  void* b = realloc(p, n);
  if(b == NULL){
    logger_fatal(f, "realloc failed");
  }
  return b;
}

extern jint      attach_current_thread(JavaVM *vm, void **penv, void *args);
extern jint      get_java_vms(JavaVM **vm);
extern jclass    find_class(JNIEnv *env, const char *class_name);
extern jmethodID find_method(JNIEnv *env, const char *class_name, const char* method_name, const char *method_signature, bool is_static);
extern __Method  resolve_jmethod_id(jmethodID mid);
