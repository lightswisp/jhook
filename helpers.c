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
#include <jni.h>
#include <jvmti.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "logger.h"

void* safe_malloc(const char *f, size_t n){
  void* b = malloc(n);
  if(b == NULL){
    logger_fatal(f, "malloc failed");
    return NULL;
  }
  memset(b, 0, n);
  return b;
}

void* safe_realloc(const char *f, void *p, size_t n){
  void* b = realloc(p, n);
  if(b == NULL){
    logger_fatal(f, "realloc failed");
  }
  return b;
}

jint attach_current_thread(JavaVM *vm, void **penv, void *args){
  jint status;
  if((status = (*vm)->AttachCurrentThread(vm, penv, args)) != JNI_OK)
    logger_fatal(__func__, "failed to attach to jvm");
  return status;
}

jint get_java_vms(JavaVM **vm){
  jint r;
	jsize jvm_count;
	if ((r = JNI_GetCreatedJavaVMs(vm, 1, &jvm_count)) != JNI_OK) {
    logger_fatal(__func__, "failed to get jvms");
		return r;
	}
  logger_log(__func__,"jvm @ %p", *vm);
  return r;
}

jclass find_class(JNIEnv *env, const char *class_name){
  jclass tmp_class;
  tmp_class = (*env)->FindClass(env, class_name);
  if(tmp_class == NULL){
    logger_fatal(__func__, "failed to obtain class address");
    return NULL;
  }
  logger_log(__func__,"class address @ %p", tmp_class);
  return tmp_class;
}

jmethodID find_method(JNIEnv *env, const char *class_name, const char* method_name, const char *method_signature, bool is_static){
  jclass t_class;
  jmethodID t_method;
  t_class = find_class(env, class_name);

  if(t_class == NULL){
    logger_fatal(__func__, "failed to obtain class <%s> for method", class_name);
    return NULL;
  }

  if(is_static)
    t_method = (*env)->GetStaticMethodID(env, t_class, method_name, method_signature);
  else
    t_method = (*env)->GetMethodID(env, t_class, method_name, method_signature);

  if(t_method == NULL){
    logger_fatal(__func__, "failed to get <%s> method id", method_name);
    return NULL;
  }

  return t_method;
}

__Method resolve_jmethod_id(jmethodID mid){
  return *((__Method*)mid);
}
