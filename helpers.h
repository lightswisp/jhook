#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <jni.h>
#include <jvmti.h>

#include "defines.h"

extern jint      attach_current_thread(JavaVM *vm, void **penv, void *args);
extern jint      get_java_vms(JavaVM **vm);
extern jclass    find_class(JNIEnv *env, const char *class_name);
extern jmethodID find_method(JNIEnv *env, const char *class_name, const char* method_name, const char *method_signature, bool is_static);
extern __Method  resolve_jmethod_id(jmethodID mid);
