#include <stdbool.h>
#include <stdint.h>
#include <jni.h>
#include <jvmti.h>

#include "defines.h"
#include "logger.h"

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
